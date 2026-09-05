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
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
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
#define MOSD_FETCH_TIMEOUT_MS                                                  \
  5000 /* safety net: if a `mediactl status`/`art` child hasn't produced        \
        * EOF by this long (e.g. a stalled network fetch for album art),       \
        * give up on it and go back to idle rather than leaving the            \
        * fetch state machine stuck forever */

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

/* Forward declarations for functions defined later in this file */
static void mediaosdpaint(const MediaState *st, const char *artpath);
static int parsestate(const char *line, MediaState *st);

static Window mosdwin;
static Drw *mosddrw;
static int mosdvisible;
static struct timespec mosdshownat; /* last real (push) trigger */
static struct timespec mosdpolledat;
static char lastart[1024]; /* art path from the last real trigger, reused
                            * by the cheap poll refreshes in between */

/* Non-blocking fetch state machine -- see mosd_poll_child(). Replaces
 * what used to be a synchronous fork+wait (runargv_getline()) called
 * directly from mediaosdtrigger()/mediaosdtick(), which blocked dwm's
 * whole event loop on every media key press (and, when fetching album
 * art, potentially on a network round-trip) for as long as `mediactl`
 * took to run. */
typedef enum { MOSD_IDLE = 0, MOSD_FETCH_STATUS, MOSD_FETCH_ART } MosdStage;

static MosdStage mosdstage = MOSD_IDLE;
static pid_t mosdchildpid = -1;
static pid_t mosdzombiepid = -1;
static int mosdchildfd = -1;
static char mosdchildbuf[1200];
static size_t mosdchildlen;
static struct timespec mosdfetchstarted;
static int mosdpendingfetchart;  /* fetch art once the status fetch completes? */
static int mosdpendingistrigger; /* did this chain start from a real trigger
                                   * (vs. a background poll), i.e. should
                                   * completing it reset the shown/timeout
                                   * clock? */
static MediaState mosdpendingstate; /* status parsed mid-chain; held until
                                      * an art fetch (if any) completes */

/* Starts argv running in the background with its stdout piped back
 * through a non-blocking fd -- poll it via mosd_poll_child() on
 * subsequent ticks instead of waiting for it here. Returns 1 if the
 * child was started, 0 on a fork/pipe failure. */
static int mosd_start_fetch(const char *const argv[], MosdStage stage) {
  int pipefd[2];
  pid_t pid;
  int flags;

  if (!argv || !argv[0])
    return 0;
  if (pipe(pipefd) < 0)
    return 0;
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
    return 0;
  }
  close(pipefd[1]);
  flags = fcntl(pipefd[0], F_GETFL, 0);
  fcntl(pipefd[0], F_SETFL, flags | O_NONBLOCK);

  mosdchildpid = pid;
  mosdchildfd = pipefd[0];
  mosdchildlen = 0;
  mosdchildbuf[0] = '\0';
  mosdstage = stage;
  clock_gettime(CLOCK_MONOTONIC, &mosdfetchstarted);
  return 1;
}

/* Non-blocking opportunistic retry for a child whose stdout hit EOF
 * before it had actually exited yet (rare for these short-lived CLI
 * tools, but possible) -- avoids ever calling a blocking waitpid(). */
static void mosd_reap_zombie(void) {
  if (mosdzombiepid > 0 && waitpid(mosdzombiepid, NULL, WNOHANG) > 0)
    mosdzombiepid = -1;
}

/* Closes the pipe, reaps (or defers to mosd_reap_zombie()) the child,
 * strips trailing newlines, and returns the accumulated output. Never
 * blocks. */
static char *mosd_reap_fetch(void) {
  if (mosdchildfd >= 0) {
    close(mosdchildfd);
    mosdchildfd = -1;
  }
  if (mosdchildpid > 0) {
    if (waitpid(mosdchildpid, NULL, WNOHANG) <= 0)
      mosdzombiepid = mosdchildpid;
    mosdchildpid = -1;
  }
  mosdchildbuf[mosdchildlen] = '\0';
  while (mosdchildlen > 0 && (mosdchildbuf[mosdchildlen - 1] == '\n' ||
                              mosdchildbuf[mosdchildlen - 1] == '\r'))
    mosdchildbuf[--mosdchildlen] = '\0';
  return mosdchildbuf;
}

/* Paints (or, for an idle status, hides) using whatever the just-completed
 * fetch chain produced, and returns the state machine to idle. Only a
 * real (push) trigger's completion resets the shown/timeout clock --
 * a background poll refresh shouldn't restart the popup's countdown. */
static void mosd_finish_and_paint(void) {
  if (mosdpendingistrigger) {
    clock_gettime(CLOCK_MONOTONIC, &mosdshownat);
    mosdpolledat = mosdshownat;
  }
  mediaosdpaint(&mosdpendingstate, lastart);
  mosdstage = MOSD_IDLE;
}

/* Advances the fetch state machine by one non-blocking step. Safe to
 * call every tick while mosdstage != MOSD_IDLE; does nothing until
 * either the child's stdout hits EOF or MOSD_FETCH_TIMEOUT_MS elapses. */
static void mosd_poll_child(void) {
  static const char *const artargv[] = {"mediactl", "art", NULL};
  struct timespec now;
  long agems;
  ssize_t n;
  char *result;
  MosdStage finishedstage;

  if (mosdchildfd < 0) {
    mosdstage = MOSD_IDLE;
    return;
  }

  clock_gettime(CLOCK_MONOTONIC, &now);
  agems = (now.tv_sec - mosdfetchstarted.tv_sec) * 1000 +
          (now.tv_nsec - mosdfetchstarted.tv_nsec) / 1000000;
  if (agems >= MOSD_FETCH_TIMEOUT_MS) {
    /* Stalled child (e.g. a hung network fetch for album art) -- give
     * up on this fetch rather than leaving the state machine stuck.
     * SIGKILL since we're not going to wait around for a graceful
     * exit; mosd_reap_zombie() will pick up the corpse. */
    if (mosdchildpid > 0)
      kill(mosdchildpid, SIGKILL);
    mosd_reap_fetch();
    mosdstage = MOSD_IDLE;
    return;
  }

  while (mosdchildlen < sizeof(mosdchildbuf) - 1) {
    n = read(mosdchildfd, mosdchildbuf + mosdchildlen,
             sizeof(mosdchildbuf) - 1 - mosdchildlen);
    if (n > 0) {
      mosdchildlen += n;
      continue;
    }
    if (n == 0)
      break; /* EOF: child is done writing */
    if (errno == EAGAIN || errno == EWOULDBLOCK)
      return; /* nothing to read yet -- try again next tick */
    break;    /* real error: treat like EOF with whatever we have */
  }

  finishedstage = mosdstage;
  result = mosd_reap_fetch();

  if (finishedstage == MOSD_FETCH_STATUS) {
    if (!parsestate(result, &mosdpendingstate)) {
      mosdstage = MOSD_IDLE;
      return;
    }
    if (strcmp(mosdpendingstate.type, "idle") == 0) {
      if (mosdvisible) {
        XUnmapWindow(dpy, mosdwin);
        mosdvisible = 0;
      }
      mosdstage = MOSD_IDLE;
      return;
    }
    if (mosdpendingfetchart) {
      if (!mosd_start_fetch(artargv, MOSD_FETCH_ART))
        mosd_finish_and_paint(); /* couldn't start the art fetch --
                                   * paint with whatever lastart we
                                   * already have rather than dropping
                                   * the status update entirely */
      return;
    }
    mosd_finish_and_paint();
    return;
  }

  if (finishedstage == MOSD_FETCH_ART) {
    strncpy(lastart, result, sizeof(lastart) - 1);
    lastart[sizeof(lastart) - 1] = '\0';
    mosd_finish_and_paint();
    return;
  }

  mosdstage = MOSD_IDLE;
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

  pthread_mutex_lock(&imlib_mutex);
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
  pthread_mutex_unlock(&imlib_mutex);
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
  /* Don't leave an in-flight fetch's child (or a not-yet-reaped one)
   * behind on shutdown. */
  if (mosdchildpid > 0)
    kill(mosdchildpid, SIGKILL);
  if (mosdzombiepid > 0)
    waitpid(mosdzombiepid, NULL, 0);
  if (mosdchildpid > 0)
    waitpid(mosdchildpid, NULL, 0);
  if (mosdchildfd >= 0)
    close(mosdchildfd);
  if (mosddrw)
    drw_free(mosddrw);
  if (mosdwin)
    XDestroyWindow(dpy, mosdwin);
}

void mediaosdtrigger(const Arg *arg) {
  static const char *const statusargv[] = {"mediactl", "status", NULL};
  (void)arg;
  if (mosdstage != MOSD_IDLE)
    return; /* a fetch is already in flight -- let it finish instead of
              * overlapping a second `mediactl` child */
  mosdpendingfetchart = 1;
  mosdpendingistrigger = 1;
  mosd_start_fetch(statusargv, MOSD_FETCH_STATUS);
}

void mediaosdtick(void) {
  static const char *const statusargv[] = {"mediactl", "status", NULL};
  struct timespec now;
  long shownms, polledms;

  mosd_reap_zombie();

  /* Drive any in-flight fetch forward regardless of mosdvisible -- a
   * fresh trigger's chain hasn't painted (and therefore hasn't set
   * mosdvisible) yet. */
  if (mosdstage != MOSD_IDLE) {
    mosd_poll_child();
    return;
  }

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
    mosdpolledat = now; /* stamp now even though the result lands async
                          * later, so the poll cadence stays steady
                          * rather than drifting by each fetch's
                          * duration */
    mosdpendingfetchart = 0;
    mosdpendingistrigger = 0; /* background poll -- don't reset the
                                * shown/timeout clock when this lands */
    mosd_start_fetch(statusargv, MOSD_FETCH_STATUS);
  }
}
