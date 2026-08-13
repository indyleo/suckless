
/* See LICENSE file for copyright and license details.
 *
 * See osd.h for the public entry points. Owns a single small
 * override-redirect popup window + its own Drw context (kept separate
 * from the bar's, so this never has to fight drawbar() over pixmap size
 * or color-scheme state). osdtrigger() runs a change command, reads a
 * level back, and paints label + percentage bar into the popup; osdtick()
 * unmaps it again after OSD_TIMEOUT_MS. Both change and get commands are
 * argv-exec'd directly (no shell), since these fire on every repeat of a
 * held-down volume/brightness key and a fork+exec is cheap where
 * fork+sh+exec is not.
 */
#include <string.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#include "dwm.h"
#include "osd.h"
#include "statusbar.h"
#include "util.h"

extern const OsdItem osds[];
extern const int osdslen;
extern const char *fonts[];
extern const int fontslen;

#define OSD_W 260
#define OSD_WIN_H 44
#define OSD_MARGIN_BOTTOM 48
#define OSD_TIMEOUT_MS 1200
#define OSD_BARH 10
#define OSD_PAD 12

static Window osdwin;
static Drw *osddrw;
static int osdvisible;
static struct timespec osdshownat;

/* Runs argv, waits for it, discards its output -- used for the "change
 * the value" half of an OsdItem. */
static void runargv_wait(const char *const argv[]) {
  pid_t pid;

  if (!argv || !argv[0])
    return;
  if ((pid = fork()) == 0) {
    setsid();
    execvp(argv[0], (char *const *)argv);
    _exit(127);
  } else if (pid > 0) {
    waitpid(pid, NULL, 0);
  }
}

/* Runs argv, waits for it, returns its first line of stdout parsed as an
 * int, or -1 on any failure/empty output -- used for the "read the value
 * back" half. */
static int runargv_getint(const char *const argv[]) {
  int pipefd[2];
  pid_t pid;
  char buf[32] = "";
  ssize_t n;

  if (!argv || !argv[0])
    return -1;
  if (pipe(pipefd) < 0)
    return -1;
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
    return -1;
  }
  close(pipefd[1]);
  n = read(pipefd[0], buf, sizeof(buf) - 1);
  buf[n > 0 ? n : 0] = '\0';
  close(pipefd[0]);
  waitpid(pid, NULL, 0);
  if (n <= 0)
    return -1;
  return atoi(buf);
}

/* Paints label + level bar into osddrw and maps the popup if it isn't
 * already visible. level < 0 means "no numeric level" -- draws an empty
 * urgency-colored track instead of a fill. */
static void osdpaint(const char *label, int level) {
  int labelw, barx, barw, bary, fillw, lvl;

  /* OSD background */
  drw_setscheme(osddrw, scheme[SchemeNorm]);
  drw_rect(osddrw, 0, 0, OSD_W, OSD_WIN_H, 1, 1);

  /* Label */
  labelw = drw_fontset_getwidth(osddrw, label) + OSD_PAD;
  drw_setscheme(osddrw, scheme[SchemeNorm]);
  drw_text(osddrw, OSD_PAD, 0, labelw, OSD_WIN_H, 0, label, 0);

  /* Progress bar */
  barx = OSD_PAD + labelw;
  barw = OSD_W - barx - OSD_PAD;
  bary = (OSD_WIN_H - OSD_BARH) / 2;

  /* Track */
  drw_setscheme(osddrw, scheme[SchemeHid]);
  drw_rect(osddrw, barx, bary, barw, OSD_BARH, 1, 1);

  /* Fill */
  if (level >= 0) {
    lvl = level > 100 ? 100 : level;
    fillw = barw * lvl / 100;

    drw_setscheme(osddrw, scheme[SchemeSel]);
    if (fillw > 0)
      drw_rect(osddrw, barx, bary, fillw, OSD_BARH, 1, 1);
  }

  drw_map(osddrw, osdwin, 0, 0, OSD_W, OSD_WIN_H);

  if (!osdvisible) {
    XMapRaised(dpy, osdwin);
    osdvisible = 1;
  }

  clock_gettime(CLOCK_MONOTONIC, &osdshownat);
}

void osdsetup(void) {
  XSetWindowAttributes wa = {
      .override_redirect = True,
      .background_pixel = scheme[SchemeNorm][ColBg].pixel,
      .event_mask = ExposureMask,
  };
  XClassHint ch = {"dwm", "dwm"};
  int x = selmon->mx + (selmon->mw - OSD_W) / 2;
  int y = selmon->my + selmon->mh - OSD_WIN_H - OSD_MARGIN_BOTTOM;

  osdwin = XCreateWindow(dpy, root, x, y, OSD_W, OSD_WIN_H, 0,
                         DefaultDepth(dpy, screen), CopyFromParent,
                         DefaultVisual(dpy, screen),
                         CWOverrideRedirect | CWBackPixel | CWEventMask, &wa);
  XSetClassHint(dpy, osdwin, &ch);

  osddrw = drw_create(dpy, screen, osdwin, OSD_W, OSD_WIN_H);
  drw_fontset_create(osddrw, fonts, fontslen); /* non-fatal if this fails
                                                * a second time -- dwm
                                                * already died in setup()
                                                * if fonts don't load at
                                                * all */
  osdvisible = 0;
}

void osdcleanup(void) {
  if (osddrw)
    drw_free(osddrw);
  if (osdwin)
    XDestroyWindow(dpy, osdwin);
}

void osdtrigger(const Arg *arg) {
  int idx = arg->i;
  const OsdItem *o;
  int level;

  if (idx < 0 || idx >= osdslen)
    return;
  o = &osds[idx];

  runargv_wait(o->changecmd);
  level = o->getcmd ? runargv_getint(o->getcmd) : -1;
  osdpaint(o->label, level);

  if (o->blockidx >= 0)
    statusbar_refresh(&(Arg){.i = o->blockidx});
}

void osdtick(void) {
  struct timespec now;
  long elapsedms;

  if (!osdvisible)
    return;
  clock_gettime(CLOCK_MONOTONIC, &now);
  elapsedms = (now.tv_sec - osdshownat.tv_sec) * 1000 +
              (now.tv_nsec - osdshownat.tv_nsec) / 1000000;
  if (elapsedms >= OSD_TIMEOUT_MS) {
    XUnmapWindow(dpy, osdwin);
    osdvisible = 0;
  }
}
