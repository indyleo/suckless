/* See LICENSE file for copyright and license details.
 *
 * See wallpaper.h for the public entry points. Everything below is
 * private to the wallpaper subsystem: a worker thread loads/scales images
 * with Imlib2 off the main X connection, hands raw pixel buffers back
 * through wpqueue, and the main thread (in run()) turns those into
 * pixmaps and composites them onto the root window.
 */
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xft/Xft.h>
#include <Imlib2.h>
#include <dirent.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "dwm.h"
#include "util.h"
#include "wallpaper.h"

typedef struct {
  int num, mx, my, mw, mh;
  char path[2048];
} WallpaperJobSpec;

typedef struct WallpaperCacheEntry {
  char path[2048];
  int w, h;
  Pixmap pm;
  unsigned long lastused;
  struct WallpaperCacheEntry *next;
} WallpaperCacheEntry;

#define WALLPAPER_CACHE_MAX 64

extern const char *wallpaperdir; /* set in config.h, included once by dwm.c */

int wallpaperupdate = 0;
volatile int wallpaperready = 0;
static volatile int wpthreadrunning = 0;
pthread_mutex_t wplock = PTHREAD_MUTEX_INITIALIZER;
WallpaperResult *wpqueue = NULL;

static Pixmap currentwallpaper[32] = {0};
static char lastwallpaper[32][2048] = {{0}};
static int wprenderw[32] = {0};
static int wprenderh[32] = {0};
static WallpaperCacheEntry *wpcache = NULL;
static int wpcachecount = 0;
static unsigned long wpcacheclock = 0;
static Pixmap rootwallpaper = 0;

static void *wallpaperworker(void *arg) {
  WallpaperJobSpec *jobs = arg;

  for (int i = 0; jobs[i].num != -1; i++) {
    imlib_context_set_anti_alias(0);
    imlib_context_set_dither(0);
    imlib_context_set_blend(0);
    imlib_context_set_dither_mask(0);

    Imlib_Image img = imlib_load_image(jobs[i].path);
    if (!img) {
      fprintf(stderr, "dwm: failed to load wallpaper: %s\n", jobs[i].path);
      continue;
    }
    imlib_context_set_image(img);
    Imlib_Image scaled = imlib_create_cropped_scaled_image(
        0, 0, imlib_image_get_width(), imlib_image_get_height(), jobs[i].mw,
        jobs[i].mh);
    imlib_free_image();
    if (!scaled) {
      fprintf(stderr, "dwm: failed to scale wallpaper\n");
      continue;
    }

    /* copy raw pixels off the imlib image — no X calls in this thread */
    imlib_context_set_image(scaled);
    int w = imlib_image_get_width();
    int h = imlib_image_get_height();
    DATA32 *src = imlib_image_get_data_for_reading_only();
    uint32_t *buf = malloc((size_t)w * h * sizeof(uint32_t));
    if (!buf) {
      imlib_free_image();
      fprintf(stderr, "dwm: out of memory for wallpaper buffer\n");
      continue;
    }
    memcpy(buf, src, (size_t)w * h * sizeof(uint32_t));
    imlib_free_image();

    WallpaperResult *res = ecalloc(1, sizeof(WallpaperResult));
    res->monnum = jobs[i].num;
    res->data = buf;
    res->w = w;
    res->h = h;
    snprintf(res->path, sizeof(res->path), "%s", jobs[i].path);

    pthread_mutex_lock(&wplock);
    res->next = wpqueue;
    wpqueue = res;
    wallpaperready = 1;
    pthread_mutex_unlock(&wplock);
  }

  free(jobs);
  wpthreadrunning = 0;
  return NULL;
}

static void rebuildrootwallpaper(void) {
  if (rootwallpaper)
    XFreePixmap(dpy, rootwallpaper);
  rootwallpaper = XCreatePixmap(dpy, root, sw, sh, DefaultDepth(dpy, screen));
  GC gc = XCreateGC(dpy, root, 0, NULL);
  for (Monitor *m = mons; m; m = m->next)
    if (currentwallpaper[m->num])
      XCopyArea(dpy, currentwallpaper[m->num], rootwallpaper, gc, 0, 0, m->mw,
                m->mh, m->mx, m->my);
  XFreeGC(dpy, gc);
  Atom prop_root = XInternAtom(dpy, "_XROOTPMAP_ID", False);
  Atom prop_esetroot = XInternAtom(dpy, "ESETROOT_PMAP_ID", False);
  XChangeProperty(dpy, root, prop_root, XA_PIXMAP, 32, PropModeReplace,
                  (unsigned char *)&rootwallpaper, 1);
  XChangeProperty(dpy, root, prop_esetroot, XA_PIXMAP, 32, PropModeReplace,
                  (unsigned char *)&rootwallpaper, 1);
  XSetWindowBackgroundPixmap(dpy, root, rootwallpaper);
  XClearWindow(dpy, root);
  XSync(dpy, False);
}

static Pixmap wallpapercache_lookup(const char *path, int w, int h) {
  for (WallpaperCacheEntry *e = wpcache; e; e = e->next) {
    if (e->w == w && e->h == h && strcmp(e->path, path) == 0) {
      e->lastused = ++wpcacheclock;
      return e->pm;
    }
  }
  return None;
}

static int wallpapercache_ispinned(Pixmap pm) {
  for (Monitor *m = mons; m; m = m->next)
    if (currentwallpaper[m->num] == pm)
      return 1;
  return 0;
}

static void wallpapercache_evict_lru(void) {
  WallpaperCacheEntry *victim = NULL, *victimprev = NULL;
  WallpaperCacheEntry *e, *prev = NULL;

  /* find the least-recently-used entry that isn't a monitor's live wallpaper */
  for (e = wpcache; e; prev = e, e = e->next) {
    if (wallpapercache_ispinned(e->pm))
      continue;
    if (!victim || e->lastused < victim->lastused) {
      victim = e;
      victimprev = prev;
    }
  }
  if (!victim)
    return; /* every cached pixmap is currently in use; cache may exceed max */

  if (victimprev)
    victimprev->next = victim->next;
  else
    wpcache = victim->next;
  XFreePixmap(dpy, victim->pm);
  free(victim);
  wpcachecount--;
}

static void wallpapercache_store(const char *path, int w, int h, Pixmap pm) {
  if (wpcachecount >= WALLPAPER_CACHE_MAX)
    wallpapercache_evict_lru();
  WallpaperCacheEntry *e = ecalloc(1, sizeof(WallpaperCacheEntry));
  snprintf(e->path, sizeof(e->path), "%s", path);
  e->w = w;
  e->h = h;
  e->pm = pm;
  e->lastused = ++wpcacheclock;
  e->next = wpcache;
  wpcache = e;
  wpcachecount++;
}

void applywallpaperresult(WallpaperResult *res) {
  Monitor *m;
  for (m = mons; m; m = m->next)
    if (m->num == res->monnum)
      break;
  if (!m) {
    /* monitor was unplugged while the job was in flight */
    free(res->data);
    return;
  }

  /* build a Pixmap from the raw pixel buffer on the main thread/connection */
  XImage *xi =
      XCreateImage(dpy, DefaultVisual(dpy, screen), DefaultDepth(dpy, screen),
                   ZPixmap, 0, (char *)res->data, res->w, res->h, 32, 0);
  if (!xi) {
    free(res->data);
    fprintf(stderr, "dwm: XCreateImage failed\n");
    return;
  }

  Pixmap pm =
      XCreatePixmap(dpy, root, res->w, res->h, DefaultDepth(dpy, screen));
  GC gc = XCreateGC(dpy, pm, 0, NULL);
  XPutImage(dpy, pm, gc, xi, 0, 0, 0, 0, res->w, res->h);
  XFreeGC(dpy, gc);
  /* XDestroyImage would free res->data — null it out first since we manage it
   */
  xi->data = NULL;
  XDestroyImage(xi);
  free(res->data);
  res->data = NULL;

  /* cache owns the pixmap from here on; only the cache's own eviction frees it
   */
  wallpapercache_store(res->path, res->w, res->h, pm);
  currentwallpaper[m->num] = pm;
  wprenderw[m->num] = res->w;
  wprenderh[m->num] = res->h;

  GC gc2 = XCreateGC(dpy, root, 0, NULL);
  XCopyArea(dpy, pm, root, gc2, 0, 0, res->w, res->h, m->mx, m->my);
  XFreeGC(dpy, gc2);
  XSync(dpy, False);

  rebuildrootwallpaper();
}

/* Shared dispatcher: given a list of monitors and the wallpaper file each
 * should show, serve cache hits immediately and spawn one worker thread for
 * whatever's left. Used by both setrandomwallpaper() (new random pick per
 * monitor) and refreshdamagedwallpapers() (re-render the *same* image at a
 * new resolution after a hotplug/resize). */
static void dispatchwallpaperjobs(Monitor **list, const char **paths, int n) {
  if (n == 0)
    return;

  if (wpthreadrunning) {
    fprintf(stderr, "dwm: wallpaper change already in progress, skipping\n");
    return;
  }

  WallpaperJobSpec *jobs = ecalloc(n + 1, sizeof(WallpaperJobSpec));
  int idx = 0;
  int cachehits = 0;

  for (int i = 0; i < n; i++) {
    Monitor *m = list[i];
    Pixmap cached = wallpapercache_lookup(paths[i], m->mw, m->mh);
    if (cached != None) {
      /* already rendered at this exact resolution — skip the worker entirely */
      currentwallpaper[m->num] = cached;
      wprenderw[m->num] = m->mw;
      wprenderh[m->num] = m->mh;
      GC gc = XCreateGC(dpy, root, 0, NULL);
      XCopyArea(dpy, cached, root, gc, 0, 0, m->mw, m->mh, m->mx, m->my);
      XFreeGC(dpy, gc);
      cachehits++;
      continue;
    }

    snprintf(jobs[idx].path, sizeof(jobs[idx].path), "%s", paths[i]);
    jobs[idx].num = m->num;
    jobs[idx].mx = m->mx;
    jobs[idx].my = m->my;
    jobs[idx].mw = m->mw;
    jobs[idx].mh = m->mh;
    idx++;
  }
  jobs[idx].num = -1; /* sentinel */

  if (cachehits > 0) {
    XSync(dpy, False);
    rebuildrootwallpaper();
  }

  if (idx == 0) {
    /* every monitor was served from cache — nothing left for the worker */
    free(jobs);
    return;
  }

  wpthreadrunning = 1;
  pthread_t t;
  if (pthread_create(&t, NULL, wallpaperworker, jobs) != 0) {
    fprintf(stderr, "dwm: failed to spawn wallpaper thread\n");
    wpthreadrunning = 0;
    free(jobs);
    return;
  }
  pthread_detach(t);
}

/* Damage-aware refresh: called after monitor geometry changes (hotplug,
 * resolution change). Re-renders the *current* wallpaper image for any
 * monitor whose resolution doesn't match what's actually cached/displayed
 * — new monitors (never rendered) and monitors whose resolution changed.
 * Monitors that are unaffected are left completely untouched: no cache
 * lookup, no X calls, no thread spawn. */
void refreshdamagedwallpapers(void) {
  char fulldir[1024];
  const char *dir = wallpaperdir;
  if (dir[0] == '~') {
    const char *home = getenv("HOME");
    if (!home)
      return;
    snprintf(fulldir, sizeof(fulldir), "%s%s", home, dir + 1);
    dir = fulldir;
  }

  int nmon = 0;
  for (Monitor *m = mons; m; m = m->next)
    nmon++;
  if (nmon == 0)
    return;

  Monitor **list = ecalloc(nmon, sizeof(Monitor *));
  char **paths = ecalloc(nmon, sizeof(char *));
  int n = 0;

  for (Monitor *m = mons; m; m = m->next) {
    int damaged = (currentwallpaper[m->num] == None) ||
                  (wprenderw[m->num] != m->mw) || (wprenderh[m->num] != m->mh);
    if (!damaged)
      continue;

    char *path = ecalloc(1, 2048);
    if (lastwallpaper[m->num][0] != '\0') {
      /* re-render the same image this monitor was already showing */
      snprintf(path, 2048, "%s/%s", dir, lastwallpaper[m->num]);
    } else {
      /* brand-new monitor with no prior wallpaper — nothing to re-render,
       * fall back to a fresh random pick for this monitor only */
      DIR *d = opendir(dir);
      if (!d) {
        free(path);
        continue;
      }
      char *files[1024];
      int count = 0;
      struct dirent *entry;
      while ((entry = readdir(d)) != NULL && count < 1024) {
        if (entry->d_name[0] == '.')
          continue;
        const char *ext = strrchr(entry->d_name, '.');
        if (!ext)
          continue;
        if (strcasecmp(ext, ".jpg") && strcasecmp(ext, ".jpeg") &&
            strcasecmp(ext, ".png") && strcasecmp(ext, ".bmp") &&
            strcasecmp(ext, ".webp"))
          continue;
        files[count++] = strdup(entry->d_name);
      }
      closedir(d);
      if (count == 0) {
        free(path);
        for (int i = 0; i < count; i++)
          free(files[i]);
        continue;
      }
      srand(time(NULL) ^ (unsigned)m->num);
      int pick = rand() % count;
      snprintf(path, 2048, "%s/%s", dir, files[pick]);
      snprintf(lastwallpaper[m->num], sizeof(lastwallpaper[m->num]), "%s",
               files[pick]);
      for (int i = 0; i < count; i++)
        free(files[i]);
    }

    list[n] = m;
    paths[n] = path;
    n++;
  }

  if (n > 0)
    dispatchwallpaperjobs(list, (const char **)paths, n);

  for (int i = 0; i < n; i++)
    free(paths[i]);
  free(list);
  free(paths);
}

void setrandomwallpaper(void) {
  char fulldir[1024];
  const char *dir = wallpaperdir;
  if (dir[0] == '~') {
    const char *home = getenv("HOME");
    if (!home)
      return;
    snprintf(fulldir, sizeof(fulldir), "%s%s", home, dir + 1);
    dir = fulldir;
  }

  if (wpthreadrunning) {
    fprintf(stderr, "dwm: wallpaper change already in progress, skipping\n");
    return;
  }

  DIR *d = opendir(dir);
  if (!d) {
    fprintf(stderr, "dwm: cannot open wallpaper dir: %s\n", dir);
    return;
  }
  char *files[1024];
  int count = 0;
  struct dirent *entry;
  while ((entry = readdir(d)) != NULL && count < 1024) {
    if (entry->d_name[0] == '.')
      continue;
    const char *ext = strrchr(entry->d_name, '.');
    if (!ext)
      continue;
    if (strcasecmp(ext, ".jpg") && strcasecmp(ext, ".jpeg") &&
        strcasecmp(ext, ".png") && strcasecmp(ext, ".bmp") &&
        strcasecmp(ext, ".webp"))
      continue;
    files[count++] = strdup(entry->d_name);
  }
  closedir(d);
  if (count == 0) {
    fprintf(stderr, "dwm: no images found in %s\n", dir);
    return;
  }

  int nmon = 0;
  for (Monitor *m = mons; m; m = m->next)
    nmon++;

  Monitor **list = ecalloc(nmon, sizeof(Monitor *));
  char **paths = ecalloc(nmon, sizeof(char *));
  int n = 0;

  srand(time(NULL));
  for (Monitor *m = mons; m; m = m->next) {
    int pick;
    if (count == 1)
      pick = 0;
    else
      do {
        pick = rand() % count;
      } while (strcmp(files[pick], lastwallpaper[m->num]) == 0);

    char *path = ecalloc(1, 2048);
    snprintf(path, 2048, "%s/%s", dir, files[pick]);
    snprintf(lastwallpaper[m->num], sizeof(lastwallpaper[m->num]), "%s",
             files[pick]);

    list[n] = m;
    paths[n] = path;
    n++;
  }

  for (int i = 0; i < count; i++)
    free(files[i]);

  dispatchwallpaperjobs(list, (const char **)paths, n);

  for (int i = 0; i < n; i++)
    free(paths[i]);
  free(list);
  free(paths);
}

void nextwallpaper(const Arg *arg) {
  fprintf(stderr, "dwm: nextwallpaper called\n");
  setrandomwallpaper();
  fprintf(stderr, "dwm: nextwallpaper done\n");
}
