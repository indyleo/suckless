/* See LICENSE file for copyright and license details.
 *
 * See mediaosd.h for the public entry points and overall design. Owns a
 * single override-redirect popup window + its own Drw context, exactly
 * like osd.c, but paints album art (via Imlib2, straight onto the
 * popup's pixmap -- same technique wallpaper.c uses against the root
 * window) plus title/artist/album text and a percent-fill progress bar.
 *
 * Data comes from shelling out to `mediactl status` (all fields in one
 * call, tab-separated) and, on a real trigger only, `mediactl art`
 * (resolves mpris:artUrl to a local file path, downloading/caching if
 * needed). Both are argv-exec'd directly, no shell -- consistent with
 * osd.c's runargv_* helpers, extended here to capture a full line of
 * stdout instead of just an int.
 */
#include <Imlib2.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#include "dwm.h"
#include "drw.h"
#include "mediaosd.h"
#include "util.h"

extern const char *fonts[];
extern const int fontslen;

#define MOSD_W 460
#define MOSD_H                                                                 \
  122 /* +14 vs the first version, to fit a time-remaining                     \
       * row above the progress bar without crowding the                       \
       * title/artist lines */
#define MOSD_MARGIN_BOTTOM                                                     \
  110 /* taller offset than osd.c's volume popup                               \
       * (48) so the two don't overlap if a                                    \
       * volume/brightness key is hit while a                                  \
       * track is showing */
#define MOSD_PAD 14
#define MOSD_ART 80 /* album art thumbnail is MOSD_ART x MOSD_ART */
#define MOSD_BARH 6
#define MOSD_TIMEOUT_MS                                                        \
  3500 /* auto-hide this long after the last real                              \
        * trigger (push event), regardless of the                              \
        * poll refreshes in between */
#define MOSD_POLL_MS                                                           \
  1000 /* while visible, re-check `mediactl status`                            \
        * (cheap: no art re-fetch) this often so                               \
        * the progress bar keeps moving */

/* Same Nerd Font / Font Awesome codepoints as mediactl's ICON_* consts
 * (see the readonly ICON_PLAYING etc. lines near the top of mediactl) --
 * kept in sync by hand since those are bash string constants, not
 * something this C code can pull in directly. Update both places
 * together if you ever change mediactl's icon set. */
#define ICON_PLAYING "\uf04b"     /* nf-fa-play */
#define ICON_PAUSED "\uf04c"      /* nf-fa-pause */
#define ICON_STOPPED "\uf04d"     /* nf-fa-stop */
#define ICON_BROWSER "\U000f059f" /* nf-md-web */
#define ICON_SONG "\U000f0388"    /* nf-md-music */

typedef struct {
  char type[16];
  char status[16];
  char title[256];
  char artist[256];
  char album[256];
  int progress;
  char progresstime[32]; /* "M:SS / M:SS", empty if unavailable */
} MediaState;

static Window mosdwin;
static Drw *mosddrw;
static int mosdvisible;
static struct timespec mosdshownat; /* last real (push) trigger */
static struct timespec mosdpolledat;
static char lastart[1024]; /* art path from the last real trigger, reused
                            * by the cheap poll refreshes in between */
static pthread_mutex_t imliblock =
    PTHREAD_MUTEX_INITIALIZER; /* wallpaper.c also calls into Imlib2 from
                                * its worker thread; Imlib2's global
                                * context isn't safe to touch from two
                                * threads at once, so serialize on it */

/* Runs argv, waits for it, returns a pointer to a static buffer holding
 * its stdout with any trailing newline(s) stripped. Empty string on any
 * failure. Caller must copy out before the next call -- the buffer is
 * reused. */
static char *runargv_getline(const char *const argv[]) {
  static char buf[1200];
  int pipefd[2];
  pid_t pid;
  ssize_t n, total = 0;

  buf[0] = '\0';
  if (!argv || !argv[0])
    return buf;
  if (pipe(pipefd) < 0)
    return buf;
  if ((pid = fork()) == 0) {
    close(pipefd[0]);
    dup2(pipefd[1], STDOUT_FILENO);
    close(pipefd[1]);
    setsid();
    execvp(argv[0], (char *const *)argv);
    _exit(127);
  } else if (pid < 0) {
    close(pipefd[0]);
    close(pipefd[1]);
    return buf;
  }
  close(pipefd[1]);
  while (total < (ssize_t)sizeof(buf) - 1 &&
         (n = read(pipefd[0], buf + total, sizeof(buf) - 1 - total)) > 0)
    total += n;
  buf[total > 0 ? total : 0] = '\0';
  close(pipefd[0]);
  waitpid(pid, NULL, 0);

  while (total > 0 && (buf[total - 1] == '\n' || buf[total - 1] == '\r'))
    buf[--total] = '\0';

  return buf;
}

/* Splits a 9-field `mediactl status` line (type, player, status, title,
 * artist, album, art_url, progress, progress_time) into st. We only keep
 * the fields we actually render -- player and art_url are skipped here
 * since art is resolved separately via `mediactl art`. Returns 0 if the
 * line doesn't look like a valid status line. */
static int parsestate(const char *line, MediaState *st) {
  char buf[1200];
  char *fields[9];
  char *p, *tab;
  int i = 0;

  memset(st, 0, sizeof(*st));
  strncpy(buf, line, sizeof(buf) - 1);
  buf[sizeof(buf) - 1] = '\0';

  p = buf;
  while (i < 9 && p) {
    fields[i++] = p;
    if ((tab = strchr(p, '\t'))) {
      *tab = '\0';
      p = tab + 1;
    } else {
      p = NULL;
    }
  }
  if (i < 9)
    return 0;

  strncpy(st->type, fields[0], sizeof(st->type) - 1);
  strncpy(st->status, fields[2], sizeof(st->status) - 1);
  strncpy(st->title, fields[3], sizeof(st->title) - 1);
  strncpy(st->artist, fields[4], sizeof(st->artist) - 1);
  strncpy(st->album, fields[5], sizeof(st->album) - 1);
  st->progress = atoi(fields[7]);
  strncpy(st->progresstime, fields[8], sizeof(st->progresstime) - 1);
  return 1;
}

/* Draws a subtle placeholder track first (matches the empty-bar look
 * osd.c uses), then -- if path resolves to a loadable image -- renders
 * it scaled into that same box via Imlib2, straight onto mosddrw's
 * pixmap. */
static void mediaosd_drawart(const char *path, int x, int y, int w, int h) {
  Imlib_Image img;

  drw_setscheme(mosddrw, scheme[SchemeHid]);
  drw_rect(mosddrw, x, y, w, h, 1, 1);

  if (!path || !path[0])
    return;

  pthread_mutex_lock(&imliblock);
  imlib_context_set_display(dpy);
  imlib_context_set_visual(DefaultVisual(dpy, screen));
  imlib_context_set_colormap(DefaultColormap(dpy, screen));
  imlib_context_set_anti_alias(1);
  imlib_context_set_dither(1);
  imlib_context_set_blend(0);

  img = imlib_load_image(path);
  if (img) {
    imlib_context_set_image(img);
    imlib_context_set_drawable(mosddrw->drawable);
    imlib_render_image_on_drawable_at_size(x, y, w, h);
    imlib_free_image();
  }
  pthread_mutex_unlock(&imliblock);
}

static void mediaosdpaint(const MediaState *st, const char *artpath) {
  int artx, arty, textx, textw, lineh;
  int barx, barw, bary, fillw, pct;
  int timey, timeh, timew, timex;
  const char *sourceicon, *playicon;
  /* title/artist/album are each up to 255 chars (MediaState), so size
   * these for the worst case instead of leaving GCC to guess -- avoids
   * a spurious -Wformat-truncation warning even though the previous,
   * smaller buffers were never actually unsafe (snprintf truncates,
   * doesn't overflow). */
  char line1[16 + sizeof(((MediaState *)0)->title)];
  char line2[8 + sizeof(((MediaState *)0)->artist) +
             sizeof(((MediaState *)0)->album)];

  drw_setscheme(mosddrw, scheme[SchemeNorm]);
  drw_rect(mosddrw, 0, 0, MOSD_W, MOSD_H, 1, 1);

  artx = MOSD_PAD;
  arty = (MOSD_H - MOSD_ART) / 2;
  mediaosd_drawart(artpath, artx, arty, MOSD_ART, MOSD_ART);

  textx = artx + MOSD_ART + MOSD_PAD;
  textw = MOSD_W - textx - MOSD_PAD;
  lineh = MOSD_H / 4;

  sourceicon = strcmp(st->type, "browser") == 0 ? ICON_BROWSER : ICON_SONG;
  playicon = strcmp(st->status, "Playing") == 0 ? ICON_PLAYING : ICON_PAUSED;
  snprintf(line1, sizeof(line1), "%s %s %s", sourceicon, playicon, st->title);
  drw_setscheme(mosddrw, scheme[SchemeNorm]);
  drw_text(mosddrw, textx, MOSD_PAD, textw, lineh, 0, line1, 0);

  line2[0] = '\0';
  if (st->artist[0] && strcmp(st->artist, "Unknown") != 0) {
    int hasalbum = st->album[0] && strcmp(st->album, "Unknown") != 0 &&
                   strcmp(st->album, "[Unknown Album]") != 0;
    if (hasalbum)
      snprintf(line2, sizeof(line2), "%s \u2014 %s", st->artist, st->album);
    else
      snprintf(line2, sizeof(line2), "%s", st->artist);
  }
  drw_setscheme(mosddrw, scheme[SchemeHid]);
  drw_text(mosddrw, textx, MOSD_PAD + lineh, textw, lineh, 0, line2, 0);

  /* elapsed / total, right-aligned, in the gap between the artist line
   * and the progress bar. drw_text always left-aligns within the box
   * you give it, so we measure the string and shrink the box from the
   * left to fake right-alignment. */
  timeh = 16;
  timey = MOSD_PAD + lineh * 2;
  if (st->progresstime[0]) {
    timew = drw_fontset_getwidth(mosddrw, st->progresstime);
    timex = textx + textw - timew;
    if (timex < textx)
      timex = textx;
    drw_setscheme(mosddrw, scheme[SchemeHid]);
    drw_text(mosddrw, timex, timey, textx + textw - timex, timeh, 0,
             st->progresstime, 0);
  }

  barx = textx;
  barw = textw;
  bary = MOSD_H - MOSD_PAD - MOSD_BARH;

  drw_setscheme(mosddrw, scheme[SchemeHid]);
  drw_rect(mosddrw, barx, bary, barw, MOSD_BARH, 1, 1);

  pct = st->progress < 0 ? 0 : (st->progress > 100 ? 100 : st->progress);
  fillw = barw * pct / 100;
  drw_setscheme(mosddrw, scheme[SchemeSel]);
  if (fillw > 0)
    drw_rect(mosddrw, barx, bary, fillw, MOSD_BARH, 1, 1);

  drw_map(mosddrw, mosdwin, 0, 0, MOSD_W, MOSD_H);

  if (!mosdvisible) {
    XMapRaised(dpy, mosdwin);
    mosdvisible = 1;
  }
}

/* fetchart: 1 on a real (push) trigger -- resolves art via `mediactl
 * art` and remembers it in lastart; 0 on a background poll tick --
 * reuses lastart so we're not shelling out to curl/mediactl art every
 * second while the popup sits there. */
static void mediaosdrefresh(int fetchart) {
  static const char *const statusargv[] = {"mediactl", "status", NULL};
  static const char *const artargv[] = {"mediactl", "art", NULL};
  char linebuf[1200];
  MediaState st;

  strncpy(linebuf, runargv_getline(statusargv), sizeof(linebuf) - 1);
  linebuf[sizeof(linebuf) - 1] = '\0';

  if (!parsestate(linebuf, &st))
    return;

  if (strcmp(st.type, "idle") == 0) {
    if (mosdvisible) {
      XUnmapWindow(dpy, mosdwin);
      mosdvisible = 0;
    }
    return;
  }

  if (fetchart) {
    strncpy(lastart, runargv_getline(artargv), sizeof(lastart) - 1);
    lastart[sizeof(lastart) - 1] = '\0';
  }

  mediaosdpaint(&st, lastart);
}

void mediaosdsetup(void) {
  XSetWindowAttributes wa = {
      .override_redirect = True,
      .background_pixel = scheme[SchemeNorm][ColBg].pixel,
      .event_mask = ExposureMask,
  };
  XClassHint ch = {"dwm-mediaosd", "dwm-mediaosd"};
  int x = selmon->mx + (selmon->mw - MOSD_W) / 2;
  int y = selmon->my + selmon->mh - MOSD_H - MOSD_MARGIN_BOTTOM;

  mosdwin = XCreateWindow(dpy, root, x, y, MOSD_W, MOSD_H, 2,
                          DefaultDepth(dpy, screen), CopyFromParent,
                          DefaultVisual(dpy, screen),
                          CWOverrideRedirect | CWBackPixel | CWEventMask, &wa);
  XSetWindowBorder(dpy, mosdwin, scheme[SchemeSel][ColBorder].pixel);
  XSetClassHint(dpy, mosdwin, &ch);

  mosddrw = drw_create(dpy, screen, mosdwin, MOSD_W, MOSD_H);
  drw_fontset_create(mosddrw, fonts, fontslen);
  mosdvisible = 0;
}

void mediaosdcleanup(void) {
  if (mosddrw)
    drw_free(mosddrw);
  if (mosdwin)
    XDestroyWindow(dpy, mosdwin);
}

void mediaosdtrigger(const Arg *arg) {
  (void)arg;
  mediaosdrefresh(1);
  clock_gettime(CLOCK_MONOTONIC, &mosdshownat);
  mosdpolledat = mosdshownat;
}

void mediaosdtick(void) {
  struct timespec now;
  long shownms, polledms;

  if (!mosdvisible)
    return;

  clock_gettime(CLOCK_MONOTONIC, &now);

  shownms = (now.tv_sec - mosdshownat.tv_sec) * 1000 +
            (now.tv_nsec - mosdshownat.tv_nsec) / 1000000;
  if (shownms >= MOSD_TIMEOUT_MS) {
    XUnmapWindow(dpy, mosdwin);
    mosdvisible = 0;
    return;
  }

  polledms = (now.tv_sec - mosdpolledat.tv_sec) * 1000 +
             (now.tv_nsec - mosdpolledat.tv_nsec) / 1000000;
  if (polledms >= MOSD_POLL_MS) {
    mosdpolledat = now;
    mediaosdrefresh(0);
  }
}
