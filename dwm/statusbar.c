
/* See LICENSE file for copyright and license details.
 *
 * See statusbar.h for the public entry points. This replaces the
 * dwmblocks binary: statusblocks[] (config.h) lists shell commands, each
 * on its own optional interval, and this module runs them, glues the
 * output together using the exact same delimiter-byte convention
 * dwmblocks used, and pushes the result into dwm's stext via
 * setstatustext(). dwm.c's drawstatusbar()/buttonpress() are unmodified
 * and don't know or care that the text didn't come from an external
 * process.
 *
 * Click routing: dwm.c's buttonpress() already walks stext to work out
 * which delimiter-terminated segment was clicked and stores that in the
 * file-local `statussig`; sigstatusbar() (also dwm.c) now forwards that
 * straight into statusbar_handleclick() instead of signaling an external
 * pid.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "dwm.h"
#include "statusbar.h"
#include "util.h"

extern const StatusBlock statusblocks[];
extern const int statusblockslen;

static char blocktext[STATUSBAR_MAXBLOCKS][256];
static time_t lastrun[STATUSBAR_MAXBLOCKS];

/* Runs one block's command and stores its (icon-prefixed, trimmed)
 * output in blocktext[i]. button > 0 sets BLOCK_BUTTON for the command,
 * matching dwmblocks' click-command convention. */
static void runblock(int i, int button) {
  char shcmd[600];
  char out[200] = "";
  FILE *fp;
  size_t l;

  if (i < 0 || i >= statusblockslen || i >= STATUSBAR_MAXBLOCKS)
    return;

  if (button > 0)
    snprintf(shcmd, sizeof(shcmd), "BLOCK_BUTTON=%d %s", button,
             statusblocks[i].cmd);
  else
    snprintf(shcmd, sizeof(shcmd), "%s", statusblocks[i].cmd);

  if ((fp = popen(shcmd, "r"))) {
    if (fgets(out, sizeof(out), fp)) {
      l = strlen(out);
      while (l && (out[l - 1] == '\n' || out[l - 1] == '\r'))
        out[--l] = '\0';
    }
    pclose(fp);
  }

  snprintf(blocktext[i], sizeof(blocktext[i]), "%s%s",
           statusblocks[i].icon ? statusblocks[i].icon : "", out);
  lastrun[i] = time(NULL);
}

/* Concatenates blocktext[] into dwm's stext, separated by a literal space
 * and a trailing delimiter byte (value i+1) per block -- the same shape
 * dwmblocks produced, so dwm.c's existing status-bar parsing/click code
 * needs no changes. */
static void rebuild(void) {
  static char buf[1024];
  size_t off = 0;
  int i, n;

  for (i = 0; i < statusblockslen && i < STATUSBAR_MAXBLOCKS; i++) {
    if (off >= sizeof(buf) - 2)
      break;
    if (i > 0)
      buf[off++] = ' ';
    n = snprintf(buf + off, sizeof(buf) - off, "%s", blocktext[i]);
    if (n > 0)
      off += (size_t)n < sizeof(buf) - off ? (size_t)n : sizeof(buf) - off - 1;
    if (off < sizeof(buf) - 1)
      buf[off++] = (char)(i + 1);
  }
  buf[off] = '\0';
  setstatustext(buf);
}

void statusbar_init(void) {
  int i;
  for (i = 0; i < statusblockslen && i < STATUSBAR_MAXBLOCKS; i++) {
    lastrun[i] = 0;
    runblock(i, 0);
  }
  rebuild();
}

void statusbar_tick(void) {
  time_t now = time(NULL);
  int i, dirty = 0;

  for (i = 0; i < statusblockslen && i < STATUSBAR_MAXBLOCKS; i++) {
    if (statusblocks[i].interval > 0 &&
        (unsigned long)(now - lastrun[i]) >= statusblocks[i].interval) {
      runblock(i, 0);
      dirty = 1;
    }
  }
  if (dirty)
    rebuild();
}

void statusbar_handleclick(int statussig, int button) {
  int idx = statussig - 1;
  if (idx < 0 || idx >= statusblockslen)
    return;
  runblock(idx, button);
  rebuild();
}

void statusbar_refresh(const Arg *arg) {
  int idx = arg->i;
  int i;

  if (idx < 0) {
    for (i = 0; i < statusblockslen && i < STATUSBAR_MAXBLOCKS; i++)
      runblock(i, 0);
  } else {
    runblock(idx, 0);
  }
  rebuild();
}
