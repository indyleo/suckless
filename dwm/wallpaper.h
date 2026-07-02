/* See LICENSE file for copyright and license details.
 *
 * Async wallpaper loader: picks/loads/scales images off the main thread
 * via Imlib2, caches rendered pixmaps per (path, resolution), and rebuilds
 * the root window's composited wallpaper. See wallpaper.c for the details.
 */
#ifndef WALLPAPER_H
#define WALLPAPER_H

#include <pthread.h>
#include <stdint.h>

#include "dwm.h" /* Arg */

typedef struct WallpaperResult {
  int monnum, w, h;
  char path[2048];
  uint32_t *data; /* raw ARGB pixel buffer, mallocd by worker */
  struct WallpaperResult *next;
} WallpaperResult;

/* run()'s event loop drains these each tick: set wallpaperready and push
 * onto wpqueue (under wplock) when a worker thread finishes a render. */
extern int wallpaperupdate;
extern volatile int wallpaperready;
extern pthread_mutex_t wplock;
extern WallpaperResult *wpqueue;

void setrandomwallpaper(void);             /* called once from main() at startup */
void applywallpaperresult(WallpaperResult *res); /* called from run() to drain wpqueue */
void refreshdamagedwallpapers(void);       /* called after monitor hotplug/resize */
void nextwallpaper(const Arg *arg);        /* fifo/keybinding entry point */

#endif /* WALLPAPER_H */
