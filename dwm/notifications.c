/* See LICENSE file for copyright and license details.
 *
 * A standalone org.freedesktop.Notifications DBus server built into dwm.
 * This version adds Imlib2 support for rendering real images/icons in
 * popups and history, and fixes popup stacking above the history overlay.
 *
 * To use this, add -lImlib2 to your LIBS in config.mk:
 *   LIBS = -L/usr/lib -lX11 -lXft -lXrender -lfontconfig -lImlib2
 */
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xft/Xft.h>
#include <fontconfig/fontconfig.h>
#include <Imlib2.h>

#include <dbus/dbus.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/stat.h>
#include <pthread.h>

#include "dwm.h"
#include "drw.h"
#include "notifications.h"
#include "statusbar.h"
#include "ipc.h"
#include "util.h"

extern const char *fonts[];
extern const int fontslen;

#define NOTIF_MAX_POPUPS 5
#define NOTIF_HISTORY_LEN 25
#define NOTIF_W 340
#define NOTIF_PAD 12
#define NOTIF_MARGIN 10
#define NOTIF_GAP 8
#define NOTIF_BORDER 2
#define NOTIF_TIMEOUT_LOW_MS 4000
#define NOTIF_TIMEOUT_NORMAL_MS 6000

/* History overlay constants */
#define NOTIF_HIST_W 360
#define NOTIF_HIST_HEADER_H 26
#define NOTIF_HIST_FOOTER_H 26
#define NOTIF_HIST_ROW_H 68
#define NOTIF_HIST_MAX_ROWS 5
#define NOTIF_HIST_PAD 8
#define NOTIF_HIST_H (NOTIF_HIST_HEADER_H + NOTIF_HIST_MAX_ROWS * NOTIF_HIST_ROW_H \
                      + NOTIF_HIST_FOOTER_H + 2 * NOTIF_HIST_PAD)

typedef struct {
  int active;
  unsigned int id;
  Window win;
  Drw *drw;
  char appname[128];
  char summary[256];
  char body[256];
  int urgency;
  char defaultaction[64];
  struct timespec shownat;
  int expiremsafter;
  /* image support */
  Imlib_Image image;
  int img_w, img_h;
} Popup;

typedef struct {
  char appname[128];
  char summary[256];
  char body[256];
  int urgency;
  time_t time;
  /* image support */
  Imlib_Image image;
  int img_w, img_h;
} HistEntry;

static Popup popups[NOTIF_MAX_POPUPS];
static HistEntry history[NOTIF_HISTORY_LEN];
static int histhead = 0;
static int histcount = 0;
static int histscroll = 0;

/* History window */
static Window histwin = None;
static Drw *histdrw = NULL;
static int histvisible = 0;

static DBusConnection *dbusconn = NULL;
static int dbusactive = 0;
static unsigned int nextid = 1;
static int dndenabled = 0;
static unsigned int unreadcount = 0;
static int notifh = 0;

/* --- small helpers ------------------------------------------------- */

static int file_exists(const char *path) {
    if (!path || path[0] == '\0') return 0;
    return (access(path, R_OK) == 0);
}

/* Internal unlocked loader: assumes imlib_mutex is already held. */
static Imlib_Image load_image_from_path_unlocked(const char *path) {
    if (!file_exists(path)) return NULL;
    return imlib_load_image(path);
}

static Imlib_Image load_image_from_path(const char *path) {
    Imlib_Image img;

    pthread_mutex_lock(&imlib_mutex);
    img = load_image_from_path_unlocked(path);
    pthread_mutex_unlock(&imlib_mutex);
    return img;
}

static Imlib_Image load_image_from_icon_name(const char *name) {
    if (!name || name[0] == '\0') return NULL;

    if (strchr(name, '/')) {
        return load_image_from_path(name);
    }

    const char *dirs[] = {
        "/usr/share/icons/hicolor/48x48/apps/",
        "/usr/share/icons/hicolor/64x64/apps/",
        "/usr/share/icons/hicolor/128x128/apps/",
        "/usr/share/pixmaps/",
        NULL
    };
    char path[512];
    Imlib_Image img = NULL;

    pthread_mutex_lock(&imlib_mutex);
    for (int i = 0; dirs[i]; i++) {
        snprintf(path, sizeof(path), "%s%s.png", dirs[i], name);
        img = load_image_from_path_unlocked(path);
        if (img) break;

        snprintf(path, sizeof(path), "%s%s.svg", dirs[i], name);
        img = load_image_from_path_unlocked(path);
        if (img) break;
    }
    pthread_mutex_unlock(&imlib_mutex);

    return img;
}

static Imlib_Image load_image_from_data(int width, int height, int rowstride,
                                        int has_alpha, int channels,
                                        const unsigned char *data) {
    if (width <= 0 || height <= 0 || !data) return NULL;

    pthread_mutex_lock(&imlib_mutex);

    Imlib_Image img = imlib_create_image(width, height);
    if (!img) {
        pthread_mutex_unlock(&imlib_mutex);
        return NULL;
    }

    imlib_context_set_image(img);
    imlib_image_set_has_alpha(has_alpha ? 1 : 0);
    DATA32 *pixels = imlib_image_get_data();
    if (!pixels) {
        imlib_free_image();
        pthread_mutex_unlock(&imlib_mutex);
        return NULL;
    }

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int offset = y * rowstride + x * channels;
            unsigned char r = data[offset];
            unsigned char g = data[offset + 1];
            unsigned char b = data[offset + 2];
            unsigned char a = has_alpha ? data[offset + 3] : 255;
            pixels[y * width + x] = (a << 24) | (r << 16) | (g << 8) | b;
        }
    }
    imlib_image_put_back_data(pixels);
    pthread_mutex_unlock(&imlib_mutex);

    return img;
}

static void notif_free_image(Imlib_Image img) {
    if (!img) return;

    pthread_mutex_lock(&imlib_mutex);
    imlib_context_set_image(img);
    imlib_free_image();
    pthread_mutex_unlock(&imlib_mutex);
}

static Imlib_Image clone_image(Imlib_Image src) {
    Imlib_Image img;

    if (!src) return NULL;

    pthread_mutex_lock(&imlib_mutex);
    imlib_context_set_image(src);
    img = imlib_clone_image();
    pthread_mutex_unlock(&imlib_mutex);

    return img;
}

static void pushhistory(const char *appname, const char *summary,
                         const char *body, int urgency,
                         Imlib_Image image, int img_w, int img_h) {
    HistEntry *h = &history[histhead];
    if (h->image) notif_free_image(h->image);
    snprintf(h->appname, sizeof(h->appname), "%s", appname ? appname : "");
    snprintf(h->summary, sizeof(h->summary), "%s", summary ? summary : "");
    snprintf(h->body, sizeof(h->body), "%s", body ? body : "");
    h->urgency = urgency;
    h->time = time(NULL);
    h->image = image;
    h->img_w = img_w;
    h->img_h = img_h;
    histhead = (histhead + 1) % NOTIF_HISTORY_LEN;
    if (histcount < NOTIF_HISTORY_LEN)
        histcount++;
}

static void remove_history_offset(int off) {
    if (off < 0 || off >= histcount) return;
    int oldest = (histhead - histcount + NOTIF_HISTORY_LEN) % NOTIF_HISTORY_LEN;
    int idx = (oldest + off) % NOTIF_HISTORY_LEN;
    if (history[idx].image) notif_free_image(history[idx].image);
    history[idx].image = NULL;
    for (int i = off; i < histcount - 1; i++) {
        int dst = (oldest + i) % NOTIF_HISTORY_LEN;
        int src = (oldest + i + 1) % NOTIF_HISTORY_LEN;
        history[dst] = history[src];
    }
    histcount--;
    histhead = (oldest + histcount) % NOTIF_HISTORY_LEN;
}

static void notif_updateblock(void) {
  char buf[32];

  if (notifblockidx < 0)
    return;

  if (dndenabled)
    snprintf(buf, sizeof(buf), "\uf1f6");
  else if (unreadcount > 0)
    snprintf(buf, sizeof(buf), "\uf0f3 %u", unreadcount > 99 ? 99 : unreadcount);
  else
    snprintf(buf, sizeof(buf), "\uf0a2");

  statusbar_setblock(notifblockidx, buf);
}

/* --- popup layout / painting --------------------------------------- */

static void notif_relayout(void) {
  int i, x, y;
  int bar_offset = 0;

  if (!selmon)
    return;

  x = selmon->mx + selmon->mw - NOTIF_W - NOTIF_MARGIN - 2 * NOTIF_BORDER;

  if (selmon->by == selmon->my)
    bar_offset = selmon->wy - selmon->my;

  y = selmon->my + bar_offset + NOTIF_MARGIN;

  for (i = 0; i < NOTIF_MAX_POPUPS; i++) {
    if (!popups[i].active)
      continue;
    XMoveWindow(dpy, popups[i].win, x, y);
    y += notifh + NOTIF_GAP;
  }
}

static void notif_paint(int slot) {
  Popup *p = &popups[slot];
  Drw *d = p->drw;
  int line1y, line2y, lineh;
  unsigned int bordercolor;

  lineh = notifh / 2;

  drw_setscheme(d, scheme[SchemeNorm]);
  drw_rect(d, 0, 0, NOTIF_W, notifh, 1, 1);

  line1y = 0;
  line2y = lineh;

  int img_area_x = NOTIF_PAD;
  int img_area_y = (notifh - 36) / 2;
  int img_area_w = 36;
  int img_area_h = 36;
  int text_left = img_area_x + img_area_w + 8;
  int text_width = NOTIF_W - text_left - NOTIF_PAD;

  if (p->image && d->drawable) {
    pthread_mutex_lock(&imlib_mutex);
    imlib_context_set_drawable(d->drawable);
    imlib_context_set_visual(DefaultVisual(dpy, d->screen));
    imlib_context_set_colormap(DefaultColormap(dpy, d->screen));
    imlib_context_set_image(p->image);
    imlib_render_image_part_on_drawable_at_size(0, 0,
        imlib_image_get_width(), imlib_image_get_height(),
        img_area_x, img_area_y, img_area_w, img_area_h);
    pthread_mutex_unlock(&imlib_mutex);
  }

  const char *sum = p->summary[0] ? p->summary : p->appname;
  drw_setscheme(d, scheme[SchemeNorm]);
  int sum_w = drw_fontset_getwidth(d, sum);
  int sum_x = text_left + (text_width - sum_w) / 2;
  drw_text(d, sum_x, line1y, sum_w, lineh, 0, sum, 0);

  drw_setscheme(d, scheme[SchemeHid]);
  int body_w = drw_fontset_getwidth(d, p->body);
  int body_x = text_left + (text_width - body_w) / 2;
  drw_text(d, body_x, line2y, body_w, lineh, 0, p->body, 0);

  drw_map(d, p->win, 0, 0, NOTIF_W, notifh);

  bordercolor = p->urgency >= 2 ? scheme[SchemeUrg][ColBorder].pixel
                : p->urgency == 0 ? scheme[SchemeHid][ColBorder].pixel
                                  : scheme[SchemeSel][ColBorder].pixel;
  XSetWindowBorder(dpy, p->win, bordercolor);
}

/* --- popup lifecycle -------------------------------------------------*/

static int find_slot_by_id(unsigned int id) {
  int i;
  if (!id)
    return -1;
  for (i = 0; i < NOTIF_MAX_POPUPS; i++)
    if (popups[i].active && popups[i].id == id)
      return i;
  return -1;
}

static int find_free_slot(void) {
  int i;
  for (i = 0; i < NOTIF_MAX_POPUPS; i++)
    if (!popups[i].active)
      return i;
  return -1;
}

static int find_slot_by_win(Window w) {
  int i;
  for (i = 0; i < NOTIF_MAX_POPUPS; i++)
    if (popups[i].win == w)
      return i;
  return -1;
}

static void notif_close_popup(int slot, unsigned int reason) {
  Popup *p = &popups[slot];
  DBusMessage *sig;

  if (!p->active)
    return;

  if (dbusactive) {
    sig = dbus_message_new_signal("/org/freedesktop/Notifications",
                                   "org.freedesktop.Notifications",
                                   "NotificationClosed");
    if (sig) {
      dbus_message_append_args(sig, DBUS_TYPE_UINT32, &p->id,
                                DBUS_TYPE_UINT32, &reason, DBUS_TYPE_INVALID);
      dbus_connection_send(dbusconn, sig, NULL);
      dbus_message_unref(sig);
    }
  }

  if (p->image) {
    notif_free_image(p->image);
    p->image = NULL;
  }

  p->active = 0;
  p->id = 0;
  XUnmapWindow(dpy, p->win);
  notif_relayout();
}

static void fill_popup(int slot, unsigned int id, const char *appname,
                        const char *summary, const char *body, int urgency,
                        const char *action, int expire_timeout,
                        Imlib_Image image, int img_w, int img_h) {
  Popup *p = &popups[slot];

  p->id = id;
  snprintf(p->appname, sizeof(p->appname), "%s", appname ? appname : "");
  snprintf(p->summary, sizeof(p->summary), "%s", summary ? summary : "");
  snprintf(p->body, sizeof(p->body), "%s", body ? body : "");
  p->urgency = urgency;
  snprintf(p->defaultaction, sizeof(p->defaultaction), "%s", action ? action : "");
  p->image = image;
  p->img_w = img_w;
  p->img_h = img_h;

  if (expire_timeout == 0)
    p->expiremsafter = -1;
  else if (expire_timeout > 0)
    p->expiremsafter = expire_timeout;
  else
    p->expiremsafter = urgency >= 2 ? -1
                      : urgency == 0 ? NOTIF_TIMEOUT_LOW_MS
                                     : NOTIF_TIMEOUT_NORMAL_MS;

  clock_gettime(CLOCK_MONOTONIC, &p->shownat);

  if (!p->active) {
    p->active = 1;
    XMapRaised(dpy, p->win);
    if (histwin != None)
        XRaiseWindow(dpy, p->win);
  }
  notif_paint(slot);
}

/* --- history overlay ------------------------------------------------- */

static const char *notif_app_icon(const char *appname) {
  if (!appname)
    return "\uf0f3";
  if (strstr(appname, "firefox") || strstr(appname, "chromium") ||
      strstr(appname, "chrome") || strstr(appname, "brave"))
    return "\uf269";
  if (strstr(appname, "discord") || strstr(appname, "signal") ||
      strstr(appname, "telegram") || strstr(appname, "slack"))
    return "\uf392";
  if (strstr(appname, "spotify") || strstr(appname, "rhythmbox"))
    return "\uf1bc";
  if (strstr(appname, "terminal") || strstr(appname, "kitty") ||
      strstr(appname, "alacritty"))
    return "\uf120";
  return "\uf0f3";
}

static void notif_hist_paint(void) {
  if (!histdrw || !histvisible) return;

  Drw *d = histdrw;
  int y;
  int textw;
  unsigned int bordercolor;

  drw_setscheme(d, scheme[SchemeNorm]);
  drw_rect(d, 0, 0, NOTIF_HIST_W, NOTIF_HIST_H, 1, 1);

  y = NOTIF_HIST_PAD;
  const char *title = "Notifications";
  drw_setscheme(d, scheme[SchemeSel]);
  textw = drw_fontset_getwidth(d, title);
  drw_text(d, (NOTIF_HIST_W - textw) / 2, y, textw, NOTIF_HIST_HEADER_H, 0, title, 0);

  const char *cleartxt = "Clear";
  textw = drw_fontset_getwidth(d, cleartxt);
  int clear_x = NOTIF_HIST_W - textw - NOTIF_HIST_PAD - 8;
  int clear_w = textw + 8;
  drw_setscheme(d, scheme[SchemeUrg]);
  drw_rect(d, clear_x - 4, y, clear_w, NOTIF_HIST_HEADER_H, 0, 0);
  drw_text(d, clear_x, y, textw, NOTIF_HIST_HEADER_H, 0, cleartxt, 1);

  y += NOTIF_HIST_HEADER_H + NOTIF_HIST_PAD;

  for (int i = 0; i < NOTIF_HIST_MAX_ROWS && i + histscroll < histcount; i++) {
    int idx = (histhead - 1 - (i + histscroll) + NOTIF_HISTORY_LEN) % NOTIF_HISTORY_LEN;
    HistEntry *h = &history[idx];

    drw_setscheme(d, scheme[SchemeNorm]);
    drw_rect(d, NOTIF_HIST_PAD, y, NOTIF_HIST_W - 2*NOTIF_HIST_PAD,
             NOTIF_HIST_ROW_H - 4, 0, 0);

    bordercolor = h->urgency >= 2 ? scheme[SchemeUrg][ColBorder].pixel
                  : h->urgency == 0 ? scheme[SchemeHid][ColBorder].pixel
                                    : scheme[SchemeSel][ColBorder].pixel;
    XSetForeground(dpy, d->gc, bordercolor);
    XDrawRectangle(dpy, d->drawable, d->gc,
                   NOTIF_HIST_PAD, y,
                   NOTIF_HIST_W - 2*NOTIF_HIST_PAD, NOTIF_HIST_ROW_H - 4);

    int img_x = NOTIF_HIST_PAD + 8;
    int img_y = y + (NOTIF_HIST_ROW_H - 4 - 36) / 2;
    int img_size = 36;

    if (h->image && d->drawable) {
      pthread_mutex_lock(&imlib_mutex);
      imlib_context_set_drawable(d->drawable);
      imlib_context_set_visual(DefaultVisual(dpy, d->screen));
      imlib_context_set_colormap(DefaultColormap(dpy, d->screen));
      imlib_context_set_image(h->image);
      imlib_render_image_part_on_drawable_at_size(0, 0,
          imlib_image_get_width(), imlib_image_get_height(),
          img_x, img_y, img_size, img_size);
      pthread_mutex_unlock(&imlib_mutex);
    } else {
      const char *icon = notif_app_icon(h->appname);
      drw_setscheme(d, scheme[SchemeNorm]);
      int icon_w = drw_fontset_getwidth(d, icon);
      drw_text(d, img_x, img_y + (img_size - d->fonts->h) / 2,
               icon_w, d->fonts->h, 0, icon, 0);
    }

    int text_left = img_x + img_size + 10;
    int text_width = NOTIF_HIST_W - text_left - NOTIF_HIST_PAD - 4;

    drw_setscheme(d, scheme[SchemeNorm]);
    int summary_w = drw_fontset_getwidth(d, h->summary);
    int summary_x = text_left + (text_width - summary_w) / 2;
    drw_text(d, summary_x, y + 4, summary_w, d->fonts->h, 0, h->summary, 0);

    drw_setscheme(d, scheme[SchemeHid]);
    int body_w = drw_fontset_getwidth(d, h->body);
    int body_x = text_left + (text_width - body_w) / 2;
    drw_text(d, body_x, y + 4 + d->fonts->h + 2, body_w, d->fonts->h, 0, h->body, 0);

    y += NOTIF_HIST_ROW_H;
  }

  y = NOTIF_HIST_H - NOTIF_HIST_FOOTER_H - NOTIF_HIST_PAD;
  char foot[128];
  snprintf(foot, sizeof(foot), "DND: %s", dndenabled ? "ON" : "OFF");
  textw = drw_fontset_getwidth(d, foot);
  drw_setscheme(d, scheme[SchemeSel]);
  drw_text(d, (NOTIF_HIST_W - textw) / 2, y, textw, NOTIF_HIST_FOOTER_H, 0, foot, 0);

  drw_map(d, histwin, 0, 0, NOTIF_HIST_W, NOTIF_HIST_H);
}

static void notif_hist_toggle(void) {
  int bar_offset = 0;

  if (!histwin) return;

  histvisible = !histvisible;
  if (histvisible) {
    if (selmon) {
      if (selmon->by == selmon->my)
        bar_offset = selmon->wy - selmon->my;
    }

    XMoveResizeWindow(dpy, histwin,
                      selmon->mx + selmon->mw - NOTIF_HIST_W - NOTIF_MARGIN,
                      selmon->my + bar_offset + NOTIF_MARGIN,
                      NOTIF_HIST_W, NOTIF_HIST_H);
    XMapRaised(dpy, histwin);
    notif_hist_paint();
  } else {
    XUnmapWindow(dpy, histwin);
  }
}

static int notif_hist_click(unsigned int button, int y) {
  if (!histvisible || !histwin) return 0;

  if (button == Button3) {
    for (int i = 0; i < NOTIF_HISTORY_LEN; i++) {
      if (history[i].image) notif_free_image(history[i].image);
      history[i].image = NULL;
    }
    histcount = 0;
    histhead = 0;
    histscroll = 0;
    notif_hist_paint();
    return 1;
  }

  if (button == Button1) {
    int header_bottom = NOTIF_HIST_PAD + NOTIF_HIST_HEADER_H;
    if (y >= NOTIF_HIST_PAD && y <= header_bottom) {
      for (int i = 0; i < NOTIF_HISTORY_LEN; i++) {
        if (history[i].image) notif_free_image(history[i].image);
        history[i].image = NULL;
      }
      histcount = 0;
      histhead = 0;
      histscroll = 0;
      notif_hist_paint();
      return 1;
    }

    int footer_top = NOTIF_HIST_H - NOTIF_HIST_FOOTER_H - NOTIF_HIST_PAD;
    if (y >= footer_top) {
      dndenabled = !dndenabled;
      notif_hist_paint();
      notif_updateblock();
      return 1;
    }

    int rows_top = NOTIF_HIST_PAD + NOTIF_HIST_HEADER_H + NOTIF_HIST_PAD;
    int rows_bottom = rows_top + NOTIF_HIST_MAX_ROWS * NOTIF_HIST_ROW_H;
    if (y >= rows_top && y < rows_bottom) {
      int rel_y = y - rows_top;
      int row = rel_y / NOTIF_HIST_ROW_H;
      int global_off = histscroll + row;
      if (global_off < histcount) {
        remove_history_offset(global_off);
        if (histscroll > 0 && histscroll >= histcount)
          histscroll = histcount > 0 ? histcount - 1 : 0;
        notif_hist_paint();
      }
      return 1;
    }
  }
  return 0;
}

/* --- dbus message parsing helpers ----------------------------------- */

static int iter_str(DBusMessageIter *it, const char **out) {
  if (dbus_message_iter_get_arg_type(it) != DBUS_TYPE_STRING)
    return 0;
  dbus_message_iter_get_basic(it, out);
  return 1;
}

static int iter_next_str(DBusMessageIter *it, const char **out) {
  dbus_message_iter_next(it);
  return iter_str(it, out);
}

static int iter_next_u32(DBusMessageIter *it, dbus_uint32_t *out) {
  dbus_message_iter_next(it);
  if (dbus_message_iter_get_arg_type(it) != DBUS_TYPE_UINT32)
    return 0;
  dbus_message_iter_get_basic(it, out);
  return 1;
}

static void send_error(DBusMessage *msg, const char *name, const char *desc) {
  DBusMessage *err;
  if (dbus_message_get_no_reply(msg))
    return;
  err = dbus_message_new_error(msg, name, desc);
  if (!err)
    return;
  dbus_connection_send(dbusconn, err, NULL);
  dbus_message_unref(err);
}

/* --- method implementations ------------------------------------------*/

static void handle_notify(DBusMessage *msg) {
  DBusMessageIter args;
  const char *appname = "", *appicon = "", *summary = "", *body = "";
  dbus_uint32_t replaces_id = 0;
  dbus_int32_t expire_timeout = -1;
  int urgency = 1;
  char actionkey[64] = "";
  DBusMessage *reply;
  dbus_uint32_t id;
  int slot;

  Imlib_Image loaded_img = NULL;
  int loaded_img_w = 0, loaded_img_h = 0;
  const char *image_path = NULL;
  int has_image_data = 0;
  int raw_w = 0, raw_h = 0, raw_rowstride = 0, raw_alpha = 0, raw_channels = 0;
  const unsigned char *raw_data = NULL;
  int raw_data_len = 0;

  if (!dbus_message_iter_init(msg, &args) || !iter_str(&args, &appname) ||
      !iter_next_u32(&args, &replaces_id) || !iter_next_str(&args, &appicon) ||
      !iter_next_str(&args, &summary) || !iter_next_str(&args, &body)) {
    send_error(msg, DBUS_ERROR_INVALID_ARGS, "malformed Notify call");
    return;
  }

  dbus_message_iter_next(&args); /* actions array */
  if (dbus_message_iter_get_arg_type(&args) == DBUS_TYPE_ARRAY) {
    DBusMessageIter actit;
    int first = 1;
    dbus_message_iter_recurse(&args, &actit);
    while (dbus_message_iter_get_arg_type(&actit) == DBUS_TYPE_STRING) {
      const char *key;
      dbus_message_iter_get_basic(&actit, &key);
      dbus_message_iter_next(&actit);
      if (dbus_message_iter_get_arg_type(&actit) == DBUS_TYPE_STRING)
        dbus_message_iter_next(&actit);
      if (first) {
        snprintf(actionkey, sizeof(actionkey), "%s", key);
        first = 0;
      }
    }
  }

  dbus_message_iter_next(&args); /* hints a{sv} */
  if (dbus_message_iter_get_arg_type(&args) == DBUS_TYPE_ARRAY) {
    DBusMessageIter hit;
    dbus_message_iter_recurse(&args, &hit);
    while (dbus_message_iter_get_arg_type(&hit) == DBUS_TYPE_DICT_ENTRY) {
      DBusMessageIter entry, variant;
      const char *key = NULL;

      dbus_message_iter_recurse(&hit, &entry);

      /* Only process if the dictionary key is safely identified as a string */
      if (dbus_message_iter_get_arg_type(&entry) == DBUS_TYPE_STRING) {
        dbus_message_iter_get_basic(&entry, &key);
        dbus_message_iter_next(&entry);

        /* Ensure the value is a variant */
        if (dbus_message_iter_get_arg_type(&entry) == DBUS_TYPE_VARIANT) {
          dbus_message_iter_recurse(&entry, &variant);
          int val_type = dbus_message_iter_get_arg_type(&variant);

          /* Apply safe guards for each expected hint type */
          if (key && strcmp(key, "urgency") == 0 && val_type == DBUS_TYPE_BYTE) {
            unsigned char v;
            dbus_message_iter_get_basic(&variant, &v);
            urgency = v;
          }
          else if (key && strcmp(key, "image-path") == 0 && val_type == DBUS_TYPE_STRING) {
            dbus_message_iter_get_basic(&variant, &image_path);
          }
          else if (key && strcmp(key, "image-data") == 0 && val_type == DBUS_TYPE_STRUCT) {
            DBusMessageIter struc;
            dbus_message_iter_recurse(&variant, &struc);
            if (dbus_message_iter_get_arg_type(&struc) == DBUS_TYPE_INT32) {
              dbus_int32_t val;
              dbus_message_iter_get_basic(&struc, &val);
              raw_w = val;
              dbus_message_iter_next(&struc);
            }
            if (dbus_message_iter_get_arg_type(&struc) == DBUS_TYPE_INT32) {
              dbus_int32_t val;
              dbus_message_iter_get_basic(&struc, &val);
              raw_h = val;
              dbus_message_iter_next(&struc);
            }
            if (dbus_message_iter_get_arg_type(&struc) == DBUS_TYPE_INT32) {
              dbus_int32_t val;
              dbus_message_iter_get_basic(&struc, &val);
              raw_rowstride = val;
              dbus_message_iter_next(&struc);
            }
            if (dbus_message_iter_get_arg_type(&struc) == DBUS_TYPE_BOOLEAN) {
              dbus_bool_t val;
              dbus_message_iter_get_basic(&struc, &val);
              raw_alpha = val;
              dbus_message_iter_next(&struc);
            }
            if (dbus_message_iter_get_arg_type(&struc) == DBUS_TYPE_INT32) {
              dbus_int32_t val;
              dbus_message_iter_get_basic(&struc, &val);
              dbus_message_iter_next(&struc);
            }
            if (dbus_message_iter_get_arg_type(&struc) == DBUS_TYPE_INT32) {
              dbus_int32_t val;
              dbus_message_iter_get_basic(&struc, &val);
              raw_channels = val;
              dbus_message_iter_next(&struc);
            }
            if (dbus_message_iter_get_arg_type(&struc) == DBUS_TYPE_ARRAY) {
              DBusMessageIter arr;
              dbus_message_iter_recurse(&struc, &arr);
              if (dbus_message_iter_get_arg_type(&arr) == DBUS_TYPE_BYTE) {
                dbus_message_iter_get_fixed_array(&arr, &raw_data, &raw_data_len);
                has_image_data = 1;
              }
            }
          }
        }
      }
      dbus_message_iter_next(&hit);
    }
  }

  dbus_message_iter_next(&args); /* expire_timeout INT32 */
  if (dbus_message_iter_get_arg_type(&args) == DBUS_TYPE_INT32)
    dbus_message_iter_get_basic(&args, &expire_timeout);

  /* Load image if available */
  if (has_image_data && raw_data && raw_w > 0 && raw_h > 0) {
    loaded_img = load_image_from_data(raw_w, raw_h, raw_rowstride,
                                      raw_alpha, raw_channels, raw_data);
    if (loaded_img) {
      pthread_mutex_lock(&imlib_mutex);
      imlib_context_set_image(loaded_img);
      loaded_img_w = imlib_image_get_width();
      loaded_img_h = imlib_image_get_height();
      pthread_mutex_unlock(&imlib_mutex);
    }
  } else if (image_path) {
    loaded_img = load_image_from_path(image_path);
    if (loaded_img) {
      pthread_mutex_lock(&imlib_mutex);
      imlib_context_set_image(loaded_img);
      loaded_img_w = imlib_image_get_width();
      loaded_img_h = imlib_image_get_height();
      pthread_mutex_unlock(&imlib_mutex);
    }
  } else if (appicon && appicon[0]) {
    loaded_img = load_image_from_icon_name(appicon);
    if (loaded_img) {
      pthread_mutex_lock(&imlib_mutex);
      imlib_context_set_image(loaded_img);
      loaded_img_w = imlib_image_get_width();
      loaded_img_h = imlib_image_get_height();
      pthread_mutex_unlock(&imlib_mutex);
    }
  }

  if (replaces_id && find_slot_by_id(replaces_id) >= 0)
    id = replaces_id;
  else {
    id = nextid++;
    if (nextid == 0)
      nextid = 1;
  }

  pushhistory(appname, summary, body, urgency, loaded_img, loaded_img_w, loaded_img_h);

  if (!dndenabled) {
    slot = find_slot_by_id(replaces_id);
    if (slot < 0)
      slot = find_free_slot();
    if (slot >= 0) {
      Imlib_Image popup_img = NULL;
      if (loaded_img) {
        popup_img = clone_image(loaded_img);
      }
      fill_popup(slot, id, appname, summary, body, urgency, actionkey,
                 expire_timeout, popup_img, loaded_img_w, loaded_img_h);
    }
    notif_relayout();
    unreadcount++;
  }
  notif_updateblock();

  reply = dbus_message_new_method_return(msg);
  if (reply) {
    dbus_message_append_args(reply, DBUS_TYPE_UINT32, &id, DBUS_TYPE_INVALID);
    dbus_connection_send(dbusconn, reply, NULL);
    dbus_message_unref(reply);
  }
}

static void handle_close_notification(DBusMessage *msg) {
  DBusMessageIter args;
  dbus_uint32_t id;
  int slot;
  DBusMessage *reply;

  if (!dbus_message_iter_init(msg, &args) ||
      dbus_message_iter_get_arg_type(&args) != DBUS_TYPE_UINT32) {
    send_error(msg, DBUS_ERROR_INVALID_ARGS, "CloseNotification needs a UINT32 id");
    return;
  }
  dbus_message_iter_get_basic(&args, &id);

  if ((slot = find_slot_by_id(id)) >= 0)
    notif_close_popup(slot, 3);

  if (!dbus_message_get_no_reply(msg)) {
    reply = dbus_message_new_method_return(msg);
    if (reply) {
      dbus_connection_send(dbusconn, reply, NULL);
      dbus_message_unref(reply);
    }
  }
}

static void handle_get_capabilities(DBusMessage *msg) {
  DBusMessage *reply = dbus_message_new_method_return(msg);
  DBusMessageIter iter, arr;
  static const char *caps[] = {"body", "actions", "persistence"};
  size_t i;

  if (!reply)
    return;
  dbus_message_iter_init_append(reply, &iter);
  dbus_message_iter_open_container(&iter, DBUS_TYPE_ARRAY, "s", &arr);
  for (i = 0; i < sizeof(caps) / sizeof(caps[0]); i++)
    dbus_message_iter_append_basic(&arr, DBUS_TYPE_STRING, &caps[i]);
  dbus_message_iter_close_container(&iter, &arr);
  dbus_connection_send(dbusconn, reply, NULL);
  dbus_message_unref(reply);
}

static void handle_get_server_information(DBusMessage *msg) {
  DBusMessage *reply = dbus_message_new_method_return(msg);
  const char *name = "dwm-notify", *vendor = "dwm", *version = "1.0",
             *specver = "1.2";

  if (!reply)
    return;
  dbus_message_append_args(reply, DBUS_TYPE_STRING, &name, DBUS_TYPE_STRING,
                            &vendor, DBUS_TYPE_STRING, &version,
                            DBUS_TYPE_STRING, &specver, DBUS_TYPE_INVALID);
  dbus_connection_send(dbusconn, reply, NULL);
  dbus_message_unref(reply);
}

static const char *introspect_xml =
    "<!DOCTYPE node PUBLIC \"-//freedesktop//DTD D-BUS Object Introspection 1.0//EN\"\n"
    " \"http://www.freedesktop.org/standards/dbus/1.0/introspect.dtd\">\n"
    "<node>\n"
    "  <interface name=\"org.freedesktop.Notifications\">\n"
    "    <method name=\"Notify\">\n"
    "      <arg direction=\"in\" type=\"s\"/><arg direction=\"in\" type=\"u\"/>\n"
    "      <arg direction=\"in\" type=\"s\"/><arg direction=\"in\" type=\"s\"/>\n"
    "      <arg direction=\"in\" type=\"s\"/><arg direction=\"in\" type=\"as\"/>\n"
    "      <arg direction=\"in\" type=\"a{sv}\"/><arg direction=\"in\" type=\"i\"/>\n"
    "      <arg direction=\"out\" type=\"u\"/>\n"
    "    </method>\n"
    "    <method name=\"CloseNotification\"><arg direction=\"in\" type=\"u\"/></method>\n"
    "    <method name=\"GetCapabilities\"><arg direction=\"out\" type=\"as\"/></method>\n"
    "    <method name=\"GetServerInformation\">\n"
    "      <arg direction=\"out\" type=\"s\"/><arg direction=\"out\" type=\"s\"/>\n"
    "      <arg direction=\"out\" type=\"s\"/><arg direction=\"out\" type=\"s\"/>\n"
    "    </method>\n"
    "    <signal name=\"NotificationClosed\"><arg type=\"u\"/><arg type=\"u\"/></signal>\n"
    "    <signal name=\"ActionInvoked\"><arg type=\"u\"/><arg type=\"s\"/></signal>\n"
    "  </interface>\n"
    "</node>\n";

static void handle_introspect(DBusMessage *msg) {
  DBusMessage *reply = dbus_message_new_method_return(msg);
  if (!reply)
    return;
  dbus_message_append_args(reply, DBUS_TYPE_STRING, &introspect_xml,
                            DBUS_TYPE_INVALID);
  dbus_connection_send(dbusconn, reply, NULL);
  dbus_message_unref(reply);
}

static void notif_handle_message(DBusMessage *msg) {
  const char *iface, *member;

  if (dbus_message_get_type(msg) != DBUS_MESSAGE_TYPE_METHOD_CALL)
    return;

  iface = dbus_message_get_interface(msg);
  member = dbus_message_get_member(msg);
  if (!member)
    return;

  if (iface && !strcmp(iface, "org.freedesktop.Notifications")) {
    if (!strcmp(member, "Notify")) {
      handle_notify(msg);
      return;
    } else if (!strcmp(member, "CloseNotification")) {
      handle_close_notification(msg);
      return;
    } else if (!strcmp(member, "GetCapabilities")) {
      handle_get_capabilities(msg);
      return;
    } else if (!strcmp(member, "GetServerInformation")) {
      handle_get_server_information(msg);
      return;
    }
  } else if (iface && !strcmp(iface, "org.freedesktop.DBus.Introspectable") &&
             !strcmp(member, "Introspect")) {
    handle_introspect(msg);
    return;
  }

  send_error(msg, DBUS_ERROR_UNKNOWN_METHOD, "not implemented by dwm-notify");
}

/* --- public entry points --------------------------------------------*/

void notifsetup(void) {
  DBusError err;
  int i;
  int ret;

  dbus_error_init(&err);
  dbusconn = dbus_bus_get(DBUS_BUS_SESSION, &err);
  if (dbus_error_is_set(&err) || !dbusconn) {
    fprintf(stderr, "dwm: notifications: could not connect to session bus: %s\n",
            err.message ? err.message : "unknown error");
    dbus_error_free(&err);
    dbusactive = 0;
  } else {
    dbus_connection_set_exit_on_disconnect(dbusconn, FALSE);
    ret = dbus_bus_request_name(dbusconn, "org.freedesktop.Notifications",
                            DBUS_NAME_FLAG_DO_NOT_QUEUE |
                            DBUS_NAME_FLAG_REPLACE_EXISTING,
                            &err);
    if (dbus_error_is_set(&err) || ret != DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER) {
      fprintf(stderr, "dwm: notifications: could not become "
                       "org.freedesktop.Notifications (another daemon "
                       "already running?), disabling\n");
      dbus_error_free(&err);
      dbusactive = 0;
    } else {
      dbusactive = 1;
    }
  }

  for (i = 0; i < NOTIF_MAX_POPUPS; i++) {
    XSetWindowAttributes wa = {
        .override_redirect = True,
        .background_pixel = scheme[SchemeNorm][ColBg].pixel,
        .event_mask = ExposureMask | ButtonPressMask,
    };
    XClassHint ch = {"dwm-notification", "dwm-notification"};

    popups[i].win =
        XCreateWindow(dpy, root, 0, 0, NOTIF_W, notifh > 0 ? notifh : 1,
                      NOTIF_BORDER, DefaultDepth(dpy, screen), CopyFromParent,
                      DefaultVisual(dpy, screen),
                      CWOverrideRedirect | CWBackPixel | CWEventMask, &wa);
    XSetClassHint(dpy, popups[i].win, &ch);
    popups[i].drw = drw_create(dpy, screen, popups[i].win, NOTIF_W,
                               notifh > 0 ? notifh : 1);
    drw_fontset_create(popups[i].drw, fonts, fontslen);

    if (notifh == 0 && popups[i].drw->fonts)
      notifh = 2 * (popups[i].drw->fonts->h + 2) + 2 * NOTIF_PAD;

    popups[i].active = 0;
    popups[i].id = 0;
    popups[i].image = NULL;
  }

  if (notifh > 0)
    for (i = 0; i < NOTIF_MAX_POPUPS; i++) {
      XResizeWindow(dpy, popups[i].win, NOTIF_W, notifh);
      drw_resize(popups[i].drw, NOTIF_W, notifh);
    }

  XSetWindowAttributes hwa = {
      .override_redirect = True,
      .background_pixel = scheme[SchemeNorm][ColBg].pixel,
      .event_mask = ExposureMask | ButtonPressMask,
  };
  XClassHint hch = {"dwm-notif-history", "dwm-notif-history"};
  histwin = XCreateWindow(dpy, root, 0, 0, NOTIF_HIST_W, NOTIF_HIST_H,
                          NOTIF_BORDER, DefaultDepth(dpy, screen),
                          CopyFromParent, DefaultVisual(dpy, screen),
                          CWOverrideRedirect | CWBackPixel | CWEventMask, &hwa);
  XSetClassHint(dpy, histwin, &hch);
  histdrw = drw_create(dpy, screen, histwin, NOTIF_HIST_W, NOTIF_HIST_H);
  drw_fontset_create(histdrw, fonts, fontslen);

  for (i = 0; i < NOTIF_HISTORY_LEN; i++) {
    history[i].image = NULL;
  }

  notif_updateblock();
}

void notifcleanup(void) {
  int i;
  for (i = 0; i < NOTIF_MAX_POPUPS; i++) {
    if (popups[i].drw)
      drw_free(popups[i].drw);
    if (popups[i].win)
      XDestroyWindow(dpy, popups[i].win);
  }
  for (i = 0; i < NOTIF_HISTORY_LEN; i++) {
    if (history[i].image) notif_free_image(history[i].image);
    history[i].image = NULL;
  }
  if (histdrw)
    drw_free(histdrw);
  if (histwin)
    XDestroyWindow(dpy, histwin);
  if (dbusconn)
    dbus_connection_unref(dbusconn);
}

void notiftick(void) {
  DBusMessage *msg;
  int i;
  struct timespec now;

  if (dbusactive && dbusconn) {
    dbus_connection_read_write_dispatch(dbusconn, 0);
    while ((msg = dbus_connection_pop_message(dbusconn))) {
      notif_handle_message(msg);
      dbus_message_unref(msg);
    }
  }

  clock_gettime(CLOCK_MONOTONIC, &now);
  for (i = 0; i < NOTIF_MAX_POPUPS; i++) {
    long elapsedms;
    if (!popups[i].active || popups[i].expiremsafter < 0)
      continue;
    elapsedms = (now.tv_sec - popups[i].shownat.tv_sec) * 1000 +
                (now.tv_nsec - popups[i].shownat.tv_nsec) / 1000000;
    if (elapsedms >= popups[i].expiremsafter)
      notif_close_popup(i, 1);
  }
}

int notif_win_click(Window w, unsigned int button, int y) {
  if (w == histwin)
    return notif_hist_click(button, y);

  int slot = find_slot_by_win(w);
  Popup *p;
  DBusMessage *sig;

  if (slot < 0)
    return 0;
  p = &popups[slot];
  if (!p->active)
    return 1;

  if (button == Button1 && p->defaultaction[0] && dbusactive) {
    sig = dbus_message_new_signal("/org/freedesktop/Notifications",
                                   "org.freedesktop.Notifications",
                                   "ActionInvoked");
    if (sig) {
      const char *action = p->defaultaction;
      dbus_message_append_args(sig, DBUS_TYPE_UINT32, &p->id, DBUS_TYPE_STRING,
                                &action, DBUS_TYPE_INVALID);
      dbus_connection_send(dbusconn, sig, NULL);
      dbus_message_unref(sig);
    }
  }
  notif_close_popup(slot, 2);
  return 1;
}

int notif_win_expose(Window w) {
  if (w == histwin) {
    if (histvisible)
      notif_hist_paint();
    return 1;
  }

  int slot = find_slot_by_win(w);
  if (slot < 0)
    return 0;
  if (popups[slot].active)
    notif_paint(slot);
  return 1;
}

void notif_blockclick(unsigned int button) {
  int i;

  if (button == Button1) {
    notif_hist_toggle();
  } else if (button == Button2) {
    for (i = 0; i < NOTIF_MAX_POPUPS; i++)
      if (popups[i].active)
        notif_close_popup(i, 2);
    unreadcount = 0;
  } else if (button == Button3) {
    dndenabled = !dndenabled;
  }
  notif_updateblock();
}

void notif_dnd(const Arg *arg) {
  (void)arg;
  dndenabled = !dndenabled;
  notif_updateblock();
}

void notif_dismissall(const Arg *arg) {
  int i;
  (void)arg;
  for (i = 0; i < NOTIF_MAX_POPUPS; i++)
    if (popups[i].active)
      notif_close_popup(i, 2);
  unreadcount = 0;
  notif_updateblock();
}

void notif_clearhistory(const Arg *arg) {
  (void)arg;
  for (int i = 0; i < NOTIF_HISTORY_LEN; i++) {
    if (history[i].image) notif_free_image(history[i].image);
    history[i].image = NULL;
  }
  histcount = 0;
  histhead = 0;
}

void notif_dumphistory(const Arg *arg) {
  char buf[NOTIF_HISTORY_LEN * 340 + 64];
  char discard[512];
  size_t off = 0;
  int i, n, idx;
  (void)arg;

  if (fiforeplyfd < 0)
    return;

  while (read(fiforeplyfd, discard, sizeof(discard)) > 0)
    ;

  for (i = 0; i < histcount && off < sizeof(buf) - 1; i++) {
    char timebuf[32];
    struct tm tmv;
    idx = (histhead - 1 - i + NOTIF_HISTORY_LEN) % NOTIF_HISTORY_LEN;
    localtime_r(&history[idx].time, &tmv);
    strftime(timebuf, sizeof(timebuf), "%H:%M:%S", &tmv);
    n = snprintf(buf + off, sizeof(buf) - off, "[%s] %s: %s - %s\n", timebuf,
                 history[idx].appname, history[idx].summary,
                 history[idx].body);
    if (n > 0)
      off += (size_t)n < sizeof(buf) - off ? (size_t)n : sizeof(buf) - off - 1;
  }
  write(fiforeplyfd, buf, off);
}
