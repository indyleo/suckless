
/* See LICENSE file for copyright and license details.
 *
 * A small on-screen-display popup for volume/brightness/mic-style
 * "change a level, show it briefly" keybindings. See osd.c.
 */
#ifndef OSD_H
#define OSD_H

#include "dwm.h" /* Arg */

typedef struct {
  const char *label;             /* short prefix shown in the popup, e.g.
                                   * "VOL" / "BRI" / "MIC" */
  const char *const *changecmd;  /* NULL-terminated argv run first (raise/
                                   * lower/toggle whatever this controls),
                                   * or NULL to just query + display */
  const char *const *getcmd;     /* NULL-terminated argv run afterwards;
                                   * its stdout is parsed as an integer
                                   * 0-100 for the level bar, or NULL to
                                   * skip the bar and just flash the
                                   * label (e.g. a plain toggle with no
                                   * numeric level) */
  int blockidx; /* statusblocks[] index (config.h) to refresh immediately
                 * afterward so the bar doesn't lag behind the OSD, or -1
                 * if no status-bar block mirrors this control */
} OsdItem;

void osdsetup(void);   /* called once from setup(), after updatebars() */
void osdcleanup(void); /* called once from cleanup() */
void osdtrigger(const Arg *arg); /* arg->i = index into osds[] (config.h);
                                   * bind directly in keys[] */
void osdtick(void); /* called every run() loop iteration; handles the
                      * popup's auto-hide timer */

#endif /* OSD_H */
