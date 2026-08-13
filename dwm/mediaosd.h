/* See LICENSE file for copyright and license details.
 *
 * A "Now Playing" OSD popup: album art thumbnail, title/artist/album,
 * and a live progress bar for whatever `mediactl status` reports as the
 * active player. Sibling to osd.c (same override-redirect popup + own
 * Drw pattern) but driven by two different triggers instead of a
 * keybinding alone:
 *
 *   - push:  medianotify calls `mediactl status` on every real player
 *            event (track/artist/play-pause change) and, when the
 *            displayed state actually changed, writes "mediaosd 0" to
 *            dwm's fifo (config.h: fifopath) in addition to its usual
 *            dwmblocks signal. That lands here via ipc.c -> mediaosdtrigger().
 *   - pull:  while the popup is visible, mediaosdtick() (called every
 *            run() iteration, like osdtick()) re-runs `mediactl status`
 *            every MOSD_POLL_MS to keep the progress bar moving, without
 *            re-fetching artwork.
 *
 * See mediaosd.c for the implementation and the two #define blocks
 * (MOSD_* geometry/timing) if you want to retune size, position, or
 * timeouts.
 */
#ifndef MEDIAOSD_H
#define MEDIAOSD_H

#include "dwm.h" /* Arg */

void mediaosdsetup(void);   /* called once from setup(), after updatebars() */
void mediaosdcleanup(void); /* called once from cleanup() */
void mediaosdtrigger(const Arg *arg); /* arg is unused (kept so this fits
                                       * the same fifocmd/keys[] function
                                       * signature as osdtrigger); bind
                                       * to the fifo "mediaosd" command
                                       * and/or a keybinding for manual
                                       * testing */
void mediaosdtick(void); /* called every run() loop iteration; handles
                          * auto-hide and the while-visible progress
                          * poll */

#endif /* MEDIAOSD_H */
