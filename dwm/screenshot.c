/* See LICENSE file for copyright and license details.
 *
 * See screenshot.h for the public entry points.
 */
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/cursorfont.h>
#include <Imlib2.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#include "dwm.h"
#include "screenshot.h"
#include "util.h"

static void selectregion(int *rx, int *ry, int *rw, int *rh);

static void screenshotpath(char *buf, size_t len) {
  const char *home = getenv("HOME");
  char dir[1024];
  struct stat st;
  time_t t;
  struct tm tmv;
  char ts[64];

  snprintf(dir, sizeof dir, "%s/Pictures/Screenshots", home ? home : ".");
  if (stat(dir, &st) != 0)
    mkdir(dir, 0755); /* -p not needed, Pictures/ should already exist */

  t = time(NULL);
  localtime_r(&t, &tmv);
  strftime(ts, sizeof ts, "%Y-%m-%d_%H-%M-%S", &tmv);
  snprintf(buf, len, "%s/%s.png", dir, ts);
}

static void copytoclip(const char *path) {
  if (fork() == 0) {
    setsid();
    execlp("xclip", "xclip", "-selection", "clipboard", "-t", "image/png", "-i",
           path, NULL);
    _exit(1);
  }
}

static void notifyshot(const char *path) {
  const char *base = strrchr(path, '/');
  char msg[2048 + 32];

  snprintf(msg, sizeof msg, "Saved to: %s", base ? base + 1 : path);
  if (fork() == 0) {
    setsid();
    execlp("notify-send", "notify-send", "-i", path, "Screenshot captured", msg,
           NULL);
    _exit(1);
  }
}

void takescreenshot(const Arg *arg) {
  Imlib_Image full, out;
  int x = 0, y = 0, w = sw, h = sh;
  char path[2048];
  Client *c;

  switch (arg->i) {
  case ShotSelect: {
    int rx, ry, rw, rh;
    selectregion(&rx, &ry, &rw, &rh);
    if (rw < 2 || rh < 2)
      return; /* cancelled, or just a click with no drag */
    x = MAX(0, rx);
    y = MAX(0, ry);
    w = MIN(rw, sw - x);
    h = MIN(rh, sh - y);
    break;
  }
  case ShotScreen:
    x = selmon->mx;
    y = selmon->my;
    w = selmon->mw;
    h = selmon->mh;
    break;
  case ShotWindow:
    if (!(c = selmon->sel))
      return;
    x = c->x;
    y = c->y;
    w = c->w + 2 * c->bw;
    h = c->h + 2 * c->bw;
    break;
  case ShotFull:
  default:
    break; /* whole root, all monitors */
  }

  imlib_context_set_display(dpy);
  imlib_context_set_visual(DefaultVisual(dpy, screen));
  imlib_context_set_colormap(DefaultColormap(dpy, screen));
  imlib_context_set_drawable(root);

  full = imlib_create_image_from_drawable(0, 0, 0, sw, sh, 1);
  if (!full)
    return;

  imlib_context_set_image(full);
  out = imlib_create_cropped_image(x, y, w, h);
  imlib_free_image(); /* frees 'full', context still points at it */

  screenshotpath(path, sizeof path);
  imlib_context_set_image(out);
  imlib_image_set_format("png");
  imlib_save_image(path);
  imlib_free_image();

  copytoclip(path);
  notifyshot(path);
}

static void copytextclip(const char *text) {
  int fd[2];

  if (pipe(fd) < 0)
    return;
  if (fork() == 0) {
    setsid();
    dup2(fd[0], STDIN_FILENO);
    close(fd[0]);
    close(fd[1]);
    execlp("xclip", "xclip", "-selection", "clipboard", NULL);
    _exit(1);
  }
  close(fd[0]);
  write(fd[1], text, strlen(text));
  close(fd[1]);
}

static void notifycolor(const char *hex) {
  if (fork() == 0) {
    setsid();
    execlp("notify-send", "notify-send", "Color Picked", hex, NULL);
    _exit(1);
  }
}

void pickcolor(const Arg *arg) {
  Cursor cur;
  XEvent ev;
  XImage *img;
  XColor color;
  int x, y;
  char hex[8];

  cur = XCreateFontCursor(dpy, XC_crosshair);
  if (XGrabPointer(dpy, root, False, ButtonPressMask, GrabModeAsync,
                   GrabModeAsync, root, cur, CurrentTime) != GrabSuccess) {
    XFreeCursor(dpy, cur);
    return;
  }

  do {
    XMaskEvent(dpy, ButtonPressMask, &ev);
  } while (ev.type != ButtonPress);
  x = ev.xbutton.x_root;
  y = ev.xbutton.y_root;

  XUngrabPointer(dpy, CurrentTime);
  XFreeCursor(dpy, cur);

  if (!(img = XGetImage(dpy, root, x, y, 1, 1, AllPlanes, ZPixmap)))
    return;
  color.pixel = XGetPixel(img, 0, 0);
  XDestroyImage(img);
  XQueryColor(dpy, DefaultColormap(dpy, screen), &color);

  snprintf(hex, sizeof hex, "#%02X%02X%02X", color.red >> 8, color.green >> 8,
           color.blue >> 8);

  copytextclip(hex);
  notifycolor(hex);
}

static void selectregion(int *rx, int *ry, int *rw, int *rh) {
  XEvent ev;
  Cursor cur;
  Window borders[4]; /* top, bottom, left, right */
  XSetWindowAttributes swa;
  int i, startx, starty, curx, cury, ox, oy, ow, oh;

  *rw = 0;
  cur = XCreateFontCursor(dpy, XC_crosshair);
  if (XGrabPointer(dpy, root, False,
                   ButtonPressMask | ButtonReleaseMask | PointerMotionMask,
                   GrabModeAsync, GrabModeAsync, root, cur,
                   CurrentTime) != GrabSuccess) {
    XFreeCursor(dpy, cur);
    return;
  }

  swa.override_redirect = True;
  swa.background_pixel = scheme[SchemeSel][ColBorder].pixel;
  swa.save_under = True;
  for (i = 0; i < 4; i++) {
    borders[i] =
        XCreateWindow(dpy, root, -10, -10, 1, 1, 0, DefaultDepth(dpy, screen),
                      CopyFromParent, DefaultVisual(dpy, screen),
                      CWOverrideRedirect | CWBackPixel | CWSaveUnder, &swa);
    XMapRaised(dpy, borders[i]);
  }

  do {
    XMaskEvent(dpy, ButtonPressMask, &ev);
  } while (ev.type != ButtonPress);
  startx = curx = ev.xbutton.x_root;
  starty = cury = ev.xbutton.y_root;

  for (;;) {
    XMaskEvent(dpy, PointerMotionMask | ButtonReleaseMask, &ev);
    curx = (ev.type == MotionNotify) ? ev.xmotion.x_root : ev.xbutton.x_root;
    cury = (ev.type == MotionNotify) ? ev.xmotion.y_root : ev.xbutton.y_root;

    ox = MIN(startx, curx);
    oy = MIN(starty, cury);
    ow = MAX(abs(curx - startx), 1);
    oh = MAX(abs(cury - starty), 1);

    XMoveResizeWindow(dpy, borders[0], ox, oy, ow, 2); /* top */
    XMoveResizeWindow(dpy, borders[1], ox, oy + MAX(oh - 2, 0), ow,
                      2);                              /* bottom */
    XMoveResizeWindow(dpy, borders[2], ox, oy, 2, oh); /* left */
    XMoveResizeWindow(dpy, borders[3], ox + MAX(ow - 2, 0), oy, 2,
                      oh); /* right */

    if (ev.type == ButtonRelease)
      break;
  }

  for (i = 0; i < 4; i++)
    XDestroyWindow(dpy, borders[i]);

  XUngrabPointer(dpy, CurrentTime);
  XFreeCursor(dpy, cur);
  XSync(dpy, False); /* let the destroys + resulting Expose repaints land before
                        we grab pixels */

  *rx = MIN(startx, curx);
  *ry = MIN(starty, cury);
  *rw = abs(curx - startx);
  *rh = abs(cury - starty);
}
