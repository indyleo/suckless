/* See LICENSE file for copyright and license details.
 *
 * See ipc.h for the public entry points.
 */
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "dwm.h"
#include "ipc.h"
#include "screenshot.h"
#include "util.h"
#include "wallpaper.h"

extern const char *fifopath;      /* set in config.h, included once by dwm.c */
extern const char *fiforeplypath; /* set in config.h, included once by dwm.c */

int fifofd = -1;
int fiforeplyfd = -1;

typedef struct {
  const char *cmd;
  void (*func)(const Arg *);
  int argtype; /* 0 = none, 1 = int, 2 = uint, 3 = float */
} FifoCmd;

/* tag index → bitmask wrappers */
static void fifoviewtag(const Arg *arg) { view(&((Arg){.ui = 1u << arg->i})); }
static void fifotagtag(const Arg *arg) { tag(&((Arg){.ui = 1u << arg->i})); }
static void fifotoggletag(const Arg *arg) {
  toggleview(&((Arg){.ui = 1u << arg->i}));
}
static void fifotogglewintag(const Arg *arg) {
  toggletag(&((Arg){.ui = 1u << arg->i}));
}
/* scratchpad index → ui wrapper */
static void fifotogglescratch(const Arg *arg) {
  togglescratch(&((Arg){.ui = (unsigned int)arg->i}));
}

/* query side: writes "mon=.. tags=.. layout=.. urgent=.. title=.." to
 * fiforeplypath so a status bar / script can read dwm's state back out
 * instead of only being able to push commands in. */
static void fifostate(const Arg *arg) {
  char buf[512], discard[512];
  Client *c;
  unsigned int urg = 0, nvis = 0;
  int len;

  if (fiforeplyfd < 0)
    return;

  for (c = selmon->clients; c; c = c->next) {
    if (c->isurgent)
      urg |= c->tags;
    if (ISVISIBLE(c))
      nvis++;
  }

  /* drain any unread previous reply so the pipe buffer can't grow
   * unbounded if nothing is reading it */
  while (read(fiforeplyfd, discard, sizeof(discard)) > 0)
    ;

  len = snprintf(buf, sizeof(buf),
                 "mon=%d tags=%u layout=%s clients=%u urgent=%u title=%s\n",
                 selmon->num, selmon->tagset[selmon->seltags],
                 selmon->lt[selmon->sellt]->name, nvis, urg,
                 selmon->sel ? selmon->sel->name : "");
  if (len > 0)
    write(fiforeplyfd, buf, (size_t)len);
}

static FifoCmd fifocmds[] = {
    /* cmd                function              argtype */
    /* tag/view */
    {"view", fifoviewtag, 1},           /* view 0-4          */
    {"tag", fifotagtag, 1},             /* tag 0-4           */
    {"toggleview", fifotoggletag, 1},   /* toggleview 0-4    */
    {"toggletag", fifotogglewintag, 1}, /* toggletag 0-4     */
    /* layout */
    {"setmfact", setmfact, 3},     /* setmfact 0.6      */
    {"incnmaster", incnmaster, 1}, /* incnmaster 1/-1   */
    {"cyclelayout", cyclelayout, 1}, /* cyclelayout 1/-1  */
    {"zoom", zoom, 0},
    {"togglefloating", togglefloating, 0},
    {"togglefullscreen", fullscreen, 0},
    /* focus / stack */
    {"focusstackvis", focusstackvis, 1}, /* focusstackvis 1/-1 */
    {"focusmon", focusmon, 1},           /* focusmon 1/-1     */
    {"tagmon", tagmon, 1},               /* tagmon 1/-1       */
    {"switchcol", switchcol, 0},
    /* window visibility */
    {"show", show, 0},
    {"hide", hide, 0},
    {"showall", showall, 0},
    {"togglewin", toggleselwin, 0}, /* toggles the focused client */
    {"killclient", killclient, 0},
    /* scratchpads */
    {"togglescratch", fifotogglescratch, 1}, /* togglescratch 0-7 */
    {"hideallscratch", hideallscratchpads, 0},
    /* bar */
    {"togglebar", togglebar, 0},
    /* wallpaper */
    {"nextwallpaper", nextwallpaper, 0},
    /* screenshots */
    {"screenshot", takescreenshot,
     1}, /* screenshot 0=full 1=screen 2=window 3=select */
    {"colorpicker", pickcolor, 0}, /* colorpicker */
    /* query */
    {"state", fifostate, 0}, /* writes state line to fiforeplypath */
    /* session */
    {"quit", quit, 0}, /* quit 1 = restart  */
};

void readfifo(void) {
  static char buf[256];
  static size_t buflen = 0;
  ssize_t n;
  char *nl;
  Arg arg;
  unsigned int i;

  n = read(fifofd, buf + buflen, sizeof(buf) - buflen - 1);
  if (n <= 0)
    return;
  buflen += n;
  buf[buflen] = '\0';

  while ((nl = strchr(buf, '\n'))) {
    *nl = '\0';
    char cmd[64], param[64];
    int items = sscanf(buf, "%63s %63s", cmd, param);

    for (i = 0; i < LENGTH(fifocmds); i++) {
      if (strcmp(cmd, fifocmds[i].cmd) != 0)
        continue;
      arg = (Arg){0};
      if (fifocmds[i].argtype == 1 && items == 2)
        arg.i = atoi(param);
      else if (fifocmds[i].argtype == 2 && items == 2)
        arg.ui = (unsigned int)atoi(param);
      else if (fifocmds[i].argtype == 3 && items == 2)
        arg.f = atof(param);
      fifocmds[i].func(&arg);
      break;
    }
    if (i == LENGTH(fifocmds))
      fprintf(stderr, "dwm: unknown fifo command '%s'\n", cmd);

    /* shift remaining buffer down past this line */
    size_t consumed = (nl - buf) + 1;
    memmove(buf, nl + 1, buflen - consumed + 1);
    buflen -= consumed;
  }
}

void setupfifo(void) {
  struct stat st;
  if (stat(fifopath, &st) != 0) {
    if (mkfifo(fifopath, 0600) != 0) {
      fprintf(stderr, "dwm: could not create fifo %s\n", fifopath);
      return;
    }
  }
  /* O_RDWR (not O_RDONLY) so we never block waiting for a writer,
   * and never get POLLHUP'd when the last writer closes */
  fifofd = open(fifopath, O_RDWR | O_NONBLOCK);
  if (fifofd < 0)
    fprintf(stderr, "dwm: could not open fifo %s\n", fifopath);

  if (stat(fiforeplypath, &st) != 0) {
    if (mkfifo(fiforeplypath, 0600) != 0) {
      fprintf(stderr, "dwm: could not create fifo %s\n", fiforeplypath);
      return;
    }
  }
  /* same O_RDWR trick: keeps a write() from the `state` command from
   * blocking or SIGPIPE'ing even if nothing is currently reading */
  fiforeplyfd = open(fiforeplypath, O_RDWR | O_NONBLOCK);
  if (fiforeplyfd < 0)
    fprintf(stderr, "dwm: could not open fifo %s\n", fiforeplypath);
}