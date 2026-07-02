/* See LICENSE file for copyright and license details.
 *
 * Screenshot capture (full/screen/window/region) and an eyedropper-style
 * color picker, both backed by Imlib2 + xclip/notify-send.
 */
#ifndef SCREENSHOT_H
#define SCREENSHOT_H

#include "dwm.h" /* Arg */

enum { ShotFull, ShotScreen, ShotWindow, ShotSelect }; /* screenshot modes,
                                                           passed as arg->i */

void takescreenshot(const Arg *arg);
void pickcolor(const Arg *arg);

#endif /* SCREENSHOT_H */
