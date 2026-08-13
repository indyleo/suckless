
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
extern const char *statusdelim;    /* visible separator printed between
                                    * blocks, e.g. " | " -- mirrors
                                    * dwmblocks-async's DELIMITER */
extern const int statusmaxlen;     /* max Unicode codepoints kept per
                                    * block's trimmed output -- mirrors
                                    * MAX_BLOCK_OUTPUT_LENGTH */
extern const int statusclickable;  /* 0 disables click-routing entirely
                                    * (no delimiter bytes are embedded, so
                                    * buttonpress()'s scan never finds one
                                    * and sigstatusbar() no-ops) -- mirrors
                                    * CLICKABLE_BLOCKS */
extern const int statusleaddelim;  /* 1 = also print statusdelim before
                                    * the first block -- mirrors
                                    * LEADING_DELIMITER */
extern const int statustraildelim; /* 1 = also print statusdelim after
                                    * the last block -- mirrors
                                    * TRAILING_DELIMITER */

static char blocktext[STATUSBAR_MAXBLOCKS][256];
static time_t lastrun[STATUSBAR_MAXBLOCKS];

/* Truncates s in place to at most n Unicode codepoints (not bytes),
 * without splitting a multi-byte UTF-8 sequence -- continuation bytes
 * are 10xxxxxx (0x80-0xBF), so only bytes that *aren't* continuation
 * bytes count as the start of a new codepoint. */
static void utf8truncate(char *s, int n) {
  int cp = 0;
  unsigned char *p = (unsigned char *)s;

  if (n < 0)
    return;
  while (*p) {
    if ((*p & 0xC0) != 0x80) { /* start of a new codepoint */
      if (cp == n) {
        *p = '\0';
        return;
      }
      cp++;
    }
    p++;
  }
}

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
  utf8truncate(blocktext[i], statusmaxlen);
  lastrun[i] = time(NULL);
}

/* Concatenates blocktext[] into dwm's stext, separated by statusdelim
 * (visible) and -- when statusclickable is set -- a trailing delimiter
 * byte (value i+1, invisible, stripped from display) per block, so
 * dwm.c's existing status-bar parsing/click code needs no changes. */
static void rebuild(void) {
  static char buf[1024];
  size_t off = 0;
  int i, n;

  if (statusleaddelim) {
    n = snprintf(buf + off, sizeof(buf) - off, "%s", statusdelim);
    if (n > 0)
      off += (size_t)n < sizeof(buf) - off ? (size_t)n : sizeof(buf) - off - 1;
  }

  for (i = 0; i < statusblockslen && i < STATUSBAR_MAXBLOCKS; i++) {
    if (off >= sizeof(buf) - 2)
      break;
    if (i > 0) {
      n = snprintf(buf + off, sizeof(buf) - off, "%s", statusdelim);
      if (n > 0)
        off +=
            (size_t)n < sizeof(buf) - off ? (size_t)n : sizeof(buf) - off - 1;
    }
    n = snprintf(buf + off, sizeof(buf) - off, "%s", blocktext[i]);
    if (n > 0)
      off += (size_t)n < sizeof(buf) - off ? (size_t)n : sizeof(buf) - off - 1;
    if (statusclickable && off < sizeof(buf) - 1)
      buf[off++] = (char)(i + 1);
  }

  if (statustraildelim && off < sizeof(buf) - 1) {
    n = snprintf(buf + off, sizeof(buf) - off, "%s", statusdelim);
    if (n > 0)
      off += (size_t)n < sizeof(buf) - off ? (size_t)n : sizeof(buf) - off - 1;
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
  /* statussig starts at 1, so index 0 is statussig - 1 */
  int blockidx = statussig - 1;

  /* Use your existing length variable since sizeof() fails on incomplete types
   */
  if (blockidx < 0 || blockidx >= statusblockslen) {
    return;
  }

  /* Execute the block command based on the index and button pressed */
  runblock(blockidx, button);

  /* Optional: Only rebuild if the click actually updates the block's state */
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
