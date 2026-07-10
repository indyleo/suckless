/* See LICENSE file for copyright and license details.
 *
 * Shared surface between dwm.c and the modules split out of it
 * (wallpaper.c, ipc.c, screenshot.c). dwm.c still owns every definition
 * here; this header just gives the other translation units something to
 * build against. Keep it minimal — only add something if more than one
 * .c file needs it.
 */
#ifndef DWM_H
#define DWM_H

#include <sys/types.h> /* pid_t */
#include <X11/Xlib.h>
#include <X11/Xft/Xft.h> /* drw.h assumes Xft's types are already visible */
#include "drw.h"          /* Clr, for the `scheme` global */

typedef union {
  int i;
  unsigned int ui;
  float f;
  const void *v;
} Arg;

typedef struct Monitor Monitor;
typedef struct Client Client;
struct Client {
  char name[256];
  float mina, maxa;
  int x, y, w, h;
  int oldx, oldy, oldw, oldh;
  int basew, baseh, incw, inch, maxw, maxh, minw, minh, hintsvalid;
  int bw, oldbw;
  unsigned int tags;
  int isfixed, isfloating, isurgent, neverfocus, oldstate, isfullscreen,
      issteam, isterminal, noswallow;
  pid_t pid;
  Client *next;
  Client *snext;
  Client *swallowing;
  Monitor *mon;
  Window win;
};

typedef struct {
  const char *symbol; /* shown in the bar */
  const char *name;   /* plain-text name, e.g. for the fifo state reply */
  void (*arrange)(Monitor *);
} Layout;

typedef struct Pertag Pertag;
struct Monitor {
  char ltsymbol[16];
  float mfact;
  int nmaster;
  int num;
  int by;             /* bar geometry */
  int btw;            /* width of tasks portion of bar */
  int bt;             /* number of tasks */
  int mx, my, mw, mh; /* screen size */
  int wx, wy, ww, wh; /* window area  */
  unsigned int seltags;
  unsigned int sellt;
  unsigned int tagset[2];
  int showbar;
  int topbar;
  int hidsel;
  Client *clients;
  Client *sel;
  Client *stack;
  Monitor *next;
  Window barwin;
  const Layout *lt[2];
  Pertag *pertag;
  int stw;
};

enum {
  SchemeNorm,
  SchemeSel,
  SchemeHid,
  SchemeUrg
}; /* color schemes, indexes `scheme` */

#define ISVISIBLE(C) ((C->tags & C->mon->tagset[C->mon->seltags]))

/* global state, defined in dwm.c */
extern Display *dpy;
extern Window root;
extern int screen;
extern int sw, sh; /* X display screen geometry width, height */
extern Monitor *mons, *selmon;
extern Clr **scheme;

/* dwm.c functions the fifo command table (ipc.c) and movestack.c dispatch
 * into */
void arrange(Monitor *m);
void view(const Arg *arg);
void tag(const Arg *arg);
void toggleview(const Arg *arg);
void toggletag(const Arg *arg);
void setmfact(const Arg *arg);
void incnmaster(const Arg *arg);
void zoom(const Arg *arg);
void togglefloating(const Arg *arg);
void fullscreen(const Arg *arg);
void focusstackvis(const Arg *arg);
void focusmon(const Arg *arg);
void tagmon(const Arg *arg);
void show(const Arg *arg);
void hide(const Arg *arg);
void showall(const Arg *arg);
void killclient(const Arg *arg);
void togglescratch(const Arg *arg);
void hideallscratchpads(const Arg *arg);
void togglebar(const Arg *arg);
void switchcol(const Arg *arg);
void cyclelayout(const Arg *arg);
void toggleselwin(const Arg *arg);
void quit(const Arg *arg);

#endif /* DWM_H */