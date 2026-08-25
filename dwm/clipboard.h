/* See LICENSE file for copyright and license details.
 *
 * A clipboard history manager built into dwm, replacing the external
 * "clip daemon" + "clip select" script pair (see autostart.sh / the
 * old MODKEY|SHIFTKEY+c binding in config.h). Watches the CLIPBOARD
 * selection via XFixes, keeps a capped history of entries (newest
 * first, with pinning so favorites survive trimming) persisted to
 * disk, and offers a dmenu-based picker to copy an old entry back
 * onto the clipboard.
 *
 * dwm never becomes the clipboard's *long-term* owner itself -- it
 * only watches (via XFixes) and, when you pick an entry, hands the
 * text to `xclip` the same way screenshot.c already does for images
 * (see copytextclip() there). That means no ICCCM selection-owner
 * protocol to implement here: dwm is a passive reader, xclip is the
 * writer, exactly like today, just with history now.
 *
 * External deps: dmenu (already a dep, see config.h's dmenucmd) and
 * xclip (already a dep, see screenshot.c / README's Runtime section).
 */
#ifndef CLIPBOARD_H
#define CLIPBOARD_H

#include "dwm.h" /* Arg */

void clipboardsetup(void);   /* called once from setup(), only if
                              * XFixesQueryExtension() succeeded */
void clipboardcleanup(void); /* called once from cleanup() */

/* Wired into dwm.c's handler[] table under SelectionNotify -- this is
 * the reply to the XConvertSelection request clipboardfixesnotify()
 * makes below. */
void clipboardselectionnotify(XEvent *e);

/* Called from run()'s event loop for the XFixesSelectionNotify
 * extension event, the same way rrscreenchangenotify() is dispatched
 * for RRScreenChangeNotify -- see dwm.c's run(). */
void clipboardfixesnotify(XEvent *e);

void clippick(const Arg *arg);  /* dmenu picker: choose a history
                                 * entry, copy it back onto CLIPBOARD */
void clippin(const Arg *arg);   /* pin/unpin the most recently copied
                                 * entry so it survives history
                                 * trimming (arg is unused) */
void clipclear(const Arg *arg); /* clear all unpinned history (arg is
                                 * unused; pinned entries are kept) */

#endif /* CLIPBOARD_H */
