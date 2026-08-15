/* See LICENSE file for copyright and license details.
 *
 * A small on-screen-display popup for volume/brightness/mic-style
 * "change a level, show it briefly" keybindings. See osd.c.
 */
#ifndef OSD_H
#define OSD_H

#include "dwm.h" /* Arg */

/* Optional fast path for an OsdItem: called (after changecmd already ran)
 * instead of getcmd+statusbar_refresh(). Must fill *level with 0-100 (or
 * -1 for "no numeric level") for the popup's bar, and -- if the item has
 * a blockidx -- fill text[] with a ready-to-display "icon label" string
 * for that statusblocks[] entry. Returns 0 on success. Returning -1
 * (e.g. because the tool it needs isn't installed) falls back to the
 * legacy getcmd/statusbar_refresh path for that trigger, so a missing
 * fastget dependency degrades gracefully instead of breaking the OSD. */
typedef int (*OsdFastGet)(int *level, char *text, size_t textsz);

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
                                   * numeric level). Ignored when fastget
                                   * is set and succeeds; kept as the
                                   * fallback path otherwise. */
  int blockidx; /* statusblocks[] index (config.h) to refresh immediately
                 * afterward so the bar doesn't lag behind the OSD, or -1
                 * if no status-bar block mirrors this control */
  OsdFastGet fastget; /* see above; NULL = always use getcmd (unchanged
                        * behavior, safe default for any item that
                        * doesn't opt in) */
} OsdItem;

/* Fast paths for the three controls that have one -- see osd.c. Each
 * replaces a `sysstats <thing>_raw` fork (itself wrapping another tool)
 * for the OSD's own level readout, AND the `popen("sh -c sysstats
 * <thing>")` fork that statusbar_refresh() would otherwise trigger, with
 * either a single direct exec (volume/mic: `wpctl get-volume` -- no
 * shell, no bash script in between) or zero forks at all (brightness:
 * a plain sysfs read). Icon/text tiers are ported 1:1 from sysstats'
 * vvolume()/mmicrophone()/bbrightness(). */
int osd_vol_fastget(int *level, char *text, size_t textsz);
int osd_mic_fastget(int *level, char *text, size_t textsz);
int osd_bri_fastget(int *level, char *text, size_t textsz);

void osdsetup(void);   /* called once from setup(), after updatebars() */
void osdcleanup(void); /* called once from cleanup() */
void osdtrigger(const Arg *arg); /* arg->i = index into osds[] (config.h);
                                   * bind directly in keys[] */
void osdtick(void); /* called every run() loop iteration; handles the
                      * popup's auto-hide timer */

#endif /* OSD_H */