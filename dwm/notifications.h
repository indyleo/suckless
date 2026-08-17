/* See LICENSE file for copyright and license details.
 *
 * A standalone org.freedesktop.Notifications DBus server built into dwm,
 * ... (existing comments)
 */
#ifndef NOTIFICATIONS_H
#define NOTIFICATIONS_H

#include <X11/Xlib.h>
#include "dwm.h" /* Arg */

extern const int notifblockidx;

void notifsetup(void);
void notifcleanup(void);
void notiftick(void);

/* Changed signature: now takes the click's Y coordinate so the history
 * window can determine which row was clicked. */
int notif_win_click(Window w, unsigned int button, int y);
int notif_win_expose(Window w);

void notif_blockclick(unsigned int button);

void notif_dnd(const Arg *arg);
void notif_dismissall(const Arg *arg);
void notif_clearhistory(const Arg *arg);
void notif_dumphistory(const Arg *arg);

#endif /* NOTIFICATIONS_H */
