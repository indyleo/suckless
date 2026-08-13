
/* See LICENSE file for copyright and license details.
 *
 * Built-in status bar blocks -- replaces dwmblocks. See statusbar.c.
 */
#ifndef STATUSBAR_H
#define STATUSBAR_H

#include "dwm.h" /* Arg */

typedef struct {
  const char *icon; /* short prefix prepended to this block's output, e.g.
                      * "" or "VOL " -- purely cosmetic, can be "" */
  const char *cmd;  /* shell command, run via popen("sh -c ...") on a
                      * timer and/or click. Only its first line of stdout
                      * is used. May itself emit dwmblocks-style
                      * ^c#rrggbb^ / ^b#rrggbb^ / ^d^ / ^f+N^ / ^r,..^
                      * color codes -- drawstatusbar() in dwm.c already
                      * parses those and is untouched by this module. On
                      * click, BLOCK_BUTTON is set in the command's
                      * environment to the mouse button number (1-5),
                      * exactly like dwmblocks did. */
  unsigned int interval; /* seconds between automatic reruns; 0 = only on
                           * click or an explicit statusbar_refresh() call
                           * (fifo `statusblock N`, or triggered from
                           * osd.c after a volume/brightness change) */
} StatusBlock;

/* delimiter bytes between blocks are literal values 1..31 (anything
 * < ' ' is stripped from the *displayed* text by drawstatusbar(), but
 * still walked by dwm.c's buttonpress() to figure out which block was
 * clicked) -- that caps this at 31 blocks, which is far more than any
 * status bar should realistically show anyway. */
#define STATUSBAR_MAXBLOCKS 31

void statusbar_init(void); /* called once from setup(), after updatebars() */
void statusbar_tick(void); /* called every run() loop iteration; cheap --
                             * only does work for blocks whose interval
                             * has actually elapsed */
void statusbar_handleclick(int statussig,
                            int button); /* called from dwm.c's
                                          * sigstatusbar(); statussig is
                                          * the delimiter byte dwm.c's own
                                          * buttonpress() already resolved
                                          * from the click position */
void statusbar_refresh(
    const Arg *arg); /* arg->i = block index to rerun immediately, or -1
                       * for all blocks. fifo command `statusblock N` and
                       * osd.c both call this. */

#endif /* STATUSBAR_H */
