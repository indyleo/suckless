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
#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
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

/* Runs argv, waits for it, returns its first line of stdout (trailing
 * newline stripped) in buf, or -1 on any failure/empty output. Sibling
 * to runargv_getint() above but for output that isn't a bare integer,
 * e.g. wpctl's "Volume: 0.45 [MUTED]". */
static int runargv_getline(const char *const argv[], char *buf, size_t bufsz) {
  int pipefd[2];
  pid_t pid;
  ssize_t n;

  if (!argv || !argv[0] || bufsz == 0)
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
  n = read(pipefd[0], buf, bufsz - 1);
  buf[n > 0 ? n : 0] = '\0';
  close(pipefd[0]);
  waitpid(pid, NULL, 0);
  if (n <= 0)
    return -1;
  while (n > 0 && (buf[n - 1] == '\n' || buf[n - 1] == '\r'))
    buf[--n] = '\0';
  return 0;
}

/* --- Fast paths for VOL/MIC/BRI ---------------------------------------
 * See osd.h for the OsdFastGet contract. Icon/text tiers below are
 * ported 1:1 from sysstats' vvolume()/mmicrophone()/bbrightness() --
 * same codepoints, verified against that script rather than guessed. */

static const char *volicon(int pct, int muted) {
  if (muted || pct <= 0) return "󰝟";
  if (pct >= 100)        return "󰶬";
  if (pct >= 75)         return "󰕾";
  if (pct >= 25)         return "󰖀";
  return "󰕿";
}

static const char *micicon(int pct, int muted) {
  return (!muted && pct > 0) ? "" : "";
}

static const char *brighticon(int pct) {
  if (pct >= 100) return "󰛨";
  if (pct >= 80)  return "󱩖"; /* sysstats itself maps both the
                                       * >=90 and >=80 tiers to this same
                                       * glyph -- kept as-is for fidelity,
                                       * not a transcription slip here */
  if (pct >= 70)  return "󱩕";
  if (pct >= 60)  return "󱩔";
  if (pct >= 50)  return "󱩓";
  if (pct >= 40)  return "󱩒";
  if (pct >= 30)  return "󱩑";
  if (pct >= 20)  return "󱩐";
  if (pct >= 10)  return "󱩏";
  return "󱩎";
}

/* Direct `wpctl get-volume TARGET`, no shell, no sysstats/pamixer in
 * between -- replaces both the OSD's own getcmd fork AND the
 * statusbar_refresh() shell fork with this one exec. Parses wpctl's own
 * "Volume: 0.NN" / "Volume: 0.NN [MUTED]" output directly. */
static int volquery_fastget(const char *target, int *level, char *text, size_t textsz) {
  const char *const argv[] = {"wpctl", "get-volume", target, NULL};
  char line[64];
  double volf;
  int muted, pct;

  if (runargv_getline(argv, line, sizeof(line)) < 0)
    return -1;
  if (sscanf(line, "Volume: %lf", &volf) != 1)
    return -1;

  muted = strstr(line, "MUTED") != NULL;
  pct = (int)(volf * 100.0 + 0.5);
  if (pct > 100) pct = 100;
  if (pct < 0) pct = 0;

  *level = pct;
  if (text && textsz > 0) {
    if (muted || pct == 0)
      snprintf(text, textsz, "%s Muted", volicon(pct, muted));
    else
      snprintf(text, textsz, "%s %d%%", volicon(pct, muted), pct);
  }
  return 0;
}

int osd_vol_fastget(int *level, char *text, size_t textsz) {
  return volquery_fastget("@DEFAULT_AUDIO_SINK@", level, text, textsz);
}

int osd_mic_fastget(int *level, char *text, size_t textsz) {
  char line[64];
  const char *const argv[] = {"wpctl", "get-volume", "@DEFAULT_AUDIO_SOURCE@", NULL};
  double volf;
  int muted, pct;

  if (runargv_getline(argv, line, sizeof(line)) < 0)
    return -1;
  if (sscanf(line, "Volume: %lf", &volf) != 1)
    return -1;

  muted = strstr(line, "MUTED") != NULL;
  pct = (int)(volf * 100.0 + 0.5);
  if (pct > 100) pct = 100;
  if (pct < 0) pct = 0;

  *level = pct;
  if (text && textsz > 0) {
    if (!muted && pct > 0)
      snprintf(text, textsz, "%s %d%%", micicon(pct, muted), pct);
    else
      snprintf(text, textsz, "%s Muted", micicon(pct, muted));
  }
  return 0;
}

/* Backlight device is discovered once (first entry under
 * /sys/class/backlight, same "pick whatever's there" logic as sysstats'
 * `find -mindepth 1 -maxdepth 1 -print -quit`) and cached for the life
 * of the process -- brightness reads after that are two fopen()+fscanf()
 * calls, no fork at all. If no backlight device is ever found (desktop
 * with no panel), this permanently returns -1 and the legacy
 * getcmd/statusbar_refresh path is used instead -- which itself just
 * shows "N/A" via sysstats, so nothing is lost. */
static const char *find_backlight_dir(void) {
  static char dir[512] = "";
  static int tried = 0;
  DIR *d;
  struct dirent *de;

  if (tried)
    return dir[0] ? dir : NULL;
  tried = 1;
  if ((d = opendir("/sys/class/backlight"))) {
    while ((de = readdir(d))) {
      if (de->d_name[0] == '.')
        continue;
      snprintf(dir, sizeof(dir), "/sys/class/backlight/%s", de->d_name);
      break;
    }
    closedir(d);
  }
  return dir[0] ? dir : NULL;
}

int osd_bri_fastget(int *level, char *text, size_t textsz) {
  const char *dir;
  char path[560];
  FILE *f;
  long cur = -1, max = -1;
  int pct;

  if (!(dir = find_backlight_dir()))
    return -1;

  snprintf(path, sizeof(path), "%s/brightness", dir);
  if ((f = fopen(path, "r"))) {
    if (fscanf(f, "%ld", &cur) != 1) cur = -1;
    fclose(f);
  }
  snprintf(path, sizeof(path), "%s/max_brightness", dir);
  if ((f = fopen(path, "r"))) {
    if (fscanf(f, "%ld", &max) != 1) max = -1;
    fclose(f);
  }
  if (cur < 0 || max <= 0)
    return -1;

  /* round-half-up, matching sysstats' bbrightness() exactly */
  pct = (int)((cur * 100 + max / 2) / max);
  if (pct > 100) pct = 100;
  if (pct < 0) pct = 0;

  *level = pct;
  if (text && textsz > 0)
    snprintf(text, textsz, "%s %d%%", brighticon(pct), pct);
  return 0;
}

/* Discovers the keyboard backlight device under /sys/class/leds. Mirrors
 * find_kbd_device() in sysctl and _kbd_backlight_dir() in sysstats --
 * all three need to agree on the same device or the OSD popup can end
 * up unable to *read* a level that sysctl/sysstats can still *write*
 * (this used to only match "kbd_backlight"/"kbd_illum" substrings,
 * missing e.g. "asus::kbd" which the shell scripts' broader kbd
 * keyboard* match already handled). Caches the result. */
static const char *find_kbd_backlight_dir(void) {
  static char dir[512] = "";
  static int tried = 0;
  DIR *d;
  struct dirent *de;
  char path[560];
  struct stat st;

  if (tried)
    return dir[0] ? dir : NULL;
  tried = 1;
  if ((d = opendir("/sys/class/leds"))) {
    while ((de = readdir(d))) {
      if (de->d_name[0] == '.')
        continue;
      if (strstr(de->d_name, "kbd") || strstr(de->d_name, "keyboard")) {
        snprintf(path, sizeof(path), "/sys/class/leds/%s/brightness", de->d_name);
        if (stat(path, &st) != 0)
          continue;
        snprintf(path, sizeof(path), "/sys/class/leds/%s/max_brightness", de->d_name);
        if (stat(path, &st) != 0)
          continue;
        snprintf(dir, sizeof(dir), "/sys/class/leds/%s", de->d_name);
        break;
      }
    }
    closedir(d);
  }

  if (!dir[0]) {
    /* Same fallback name list as the shell scripts, for identical
     * behavior when the substring scan above finds nothing. */
    static const char *fallback[] = {"kbd_backlight", "platform::kbd_backlight",
                                      "tpacpi::kbd_backlight", "dell::kbd_backlight"};
    size_t i;
    for (i = 0; i < sizeof(fallback) / sizeof(fallback[0]); i++) {
      snprintf(path, sizeof(path), "/sys/class/leds/%s/brightness", fallback[i]);
      if (stat(path, &st) != 0)
        continue;
      snprintf(dir, sizeof(dir), "/sys/class/leds/%s", fallback[i]);
      break;
    }
  }

  return dir[0] ? dir : NULL;
}

int osd_kbd_fastget(int *level, char *text, size_t textsz) {
  const char *dir;
  char path[560];
  FILE *f;
  long cur = -1, max = -1;
  int pct;

  if (!(dir = find_kbd_backlight_dir()))
    return -1;

  snprintf(path, sizeof(path), "%s/brightness", dir);
  if ((f = fopen(path, "r"))) {
    if (fscanf(f, "%ld", &cur) != 1) cur = -1;
    fclose(f);
  }
  snprintf(path, sizeof(path), "%s/max_brightness", dir);
  if ((f = fopen(path, "r"))) {
    if (fscanf(f, "%ld", &max) != 1) max = -1;
    fclose(f);
  }
  if (cur < 0 || max <= 0)
    return -1;

  /* round-half-up */
  pct = (int)((cur * 100 + max / 2) / max);
  if (pct > 100) pct = 100;
  if (pct < 0) pct = 0;

  *level = pct;
  if (text && textsz > 0)
    snprintf(text, textsz, "󰌌 %d%%", pct); /* Using a generic Nerd Font keyboard icon */
  return 0;
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
  XClassHint ch = {"dwm-osd", "dwm-osd"};
  int x = selmon->mx + (selmon->mw - OSD_W) / 2;
  int y = selmon->my + selmon->mh - OSD_WIN_H - OSD_MARGIN_BOTTOM;

  osdwin = XCreateWindow(dpy, root, x, y, OSD_W, OSD_WIN_H, 2,
                         DefaultDepth(dpy, screen), CopyFromParent,
                         DefaultVisual(dpy, screen),
                         CWOverrideRedirect | CWBackPixel | CWEventMask, &wa);
  XSetWindowBorder(dpy, osdwin, scheme[SchemeSel][ColBorder].pixel);
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
  int level = -1;
  char blocktext[128] = "";
  int fastok = 0;

  if (idx < 0 || idx >= osdslen)
    return;
  o = &osds[idx];

  runargv_wait(o->changecmd);

  if (o->fastget)
    fastok = o->fastget(&level, blocktext, sizeof(blocktext)) == 0;

  if (!fastok)
    level = o->getcmd ? runargv_getint(o->getcmd) : -1;

  osdpaint(o->label, level);

  if (o->blockidx >= 0) {
    if (fastok && blocktext[0])
      statusbar_setblock(o->blockidx, blocktext);
    else
      statusbar_refresh(&(Arg){.i = o->blockidx});
  }
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
