/* See LICENSE file for copyright and license details.
 *
 * See clipboard.h for the public entry points and the overall design
 * (passive XFixes watcher + xclip as the writer, no selection-owner
 * protocol implemented here).
 */
#include <X11/Xatom.h>
#include <X11/extensions/Xfixes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#include "clipboard.h"
#include "dwm.h"
#include "util.h"

/* Unpinned entries kept before the oldest is evicted; pinned entries
 * get their own (larger) cap and are never evicted by *unpinned*
 * growth. Both are generous for a personal history, not a leak. */
#define CLIP_MAX_HISTORY 200
#define CLIP_MAX_PINNED 100
/* Absurdly large clips (a whole log file fat-fingered into a copy) get
 * truncated at this many bytes rather than bloating history forever. */
#define CLIP_MAX_ENTRY (256 * 1024)
/* How much of an entry is shown per dmenu line. */
#define CLIP_PREVIEW_LEN 100

typedef struct ClipEntry {
  char *text;
  size_t len;
  time_t ts;
  int pinned;
  struct ClipEntry *next;
} ClipEntry;

static ClipEntry *history = NULL; /* unpinned, newest first */
static ClipEntry *pinned = NULL;  /* pinned, newest first */
static int historycount = 0;
static int pinnedcount = 0;
/* The single most recently *captured or picked* entry, whichever list
 * it's currently sitting in: pushed to the head of `history` when a
 * new copy comes in, but also reassigned to an arbitrary (possibly
 * not-head) entry by clippick() when the user picks something from
 * history. clippin() relocates it between lists without invalidating
 * this pointer either way -- see cliplistmove()'s comment. */
static ClipEntry *lastentry = NULL;

static Atom clipboardatom;
static Atom utf8atom;
static Atom clipdwmprop;
static Window clipwin = None;
static int clipboardactive = 0;

/* ---- small local helpers, same shape as the ones screenshot.c /
 * osd.c / mediaosd.c each keep a private copy of ------------------- */

static void *erealloc(void *p, size_t n) {
  void *r = realloc(p, n);
  if (!r)
    die("realloc:");
  return r;
}

static void mkdir_p(const char *path) {
  char buf[1024];
  struct stat st;
  char *p;

  snprintf(buf, sizeof buf, "%s", path);
  for (p = buf + 1; *p; p++) {
    if (*p == '/') {
      *p = '\0';
      if (stat(buf, &st) != 0)
        mkdir(buf, 0755);
      *p = '/';
    }
  }
  if (stat(buf, &st) != 0)
    mkdir(buf, 0755);
}

static int clippath(char *buf, size_t bufsz) {
  const char *xdg = getenv("XDG_CACHE_HOME");
  const char *home = getenv("HOME");
  char dir[900];

  if (xdg && *xdg)
    snprintf(dir, sizeof dir, "%s/dwm", xdg);
  else if (home && *home)
    snprintf(dir, sizeof dir, "%s/.cache/dwm", home);
  else
    return 0;
  mkdir_p(dir);
  snprintf(buf, bufsz, "%s/clipboard_history", dir);
  return 1;
}

/* Writes `input` to argv's stdin (may be NULL/0-length), then reads
 * one line of its stdout into buf. Returns 0 if a non-empty line was
 * read, -1 otherwise (spawn failure, or the picker was cancelled with
 * no output -- e.g. dmenu closed via Escape). Same fork/pipe shape as
 * screenshot.c's copytextclip() and osd.c's runargv_getline(), just
 * bidirectional. */
static int runargv_io(const char *const argv[], const char *input,
                       size_t inputlen, char *buf, size_t bufsz) {
  int inpipe[2], outpipe[2];
  pid_t pid;
  ssize_t n;
  size_t total = 0;

  if (pipe(inpipe) < 0)
    return -1;
  if (pipe(outpipe) < 0) {
    close(inpipe[0]);
    close(inpipe[1]);
    return -1;
  }
  if ((pid = fork()) == 0) {
    setsid();
    dup2(inpipe[0], STDIN_FILENO);
    dup2(outpipe[1], STDOUT_FILENO);
    close(inpipe[0]);
    close(inpipe[1]);
    close(outpipe[0]);
    close(outpipe[1]);
    execvp(argv[0], (char *const *)argv);
    _exit(1);
  }
  close(inpipe[0]);
  close(outpipe[1]);
  if (pid < 0) {
    close(inpipe[1]);
    close(outpipe[0]);
    return -1;
  }

  if (input && inputlen) {
    size_t written = 0;
    ssize_t w;
    while (written < inputlen) {
      w = write(inpipe[1], input + written, inputlen - written);
      if (w <= 0)
        break;
      written += (size_t)w;
    }
  }
  close(inpipe[1]); /* EOF, so dmenu knows the list is complete */

  while (total < bufsz - 1 &&
         (n = read(outpipe[0], buf + total, bufsz - 1 - total)) > 0)
    total += (size_t)n;
  close(outpipe[0]);
  waitpid(pid, NULL, 0);

  buf[total] = '\0';
  if (total > 0 && buf[total - 1] == '\n')
    buf[total - 1] = '\0';
  return total > 0 ? 0 : -1;
}

/* Same fork/pipe shape as screenshot.c's copytextclip(); kept as its
 * own copy here (rather than shared) since that one takes a
 * NUL-terminated string and this one needs an explicit length. */
static void copytextclip(const char *text, size_t len) {
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
  if (write(fd[1], text, len) < 0)
    ; /* best effort, same as screenshot.c's copytextclip */
  close(fd[1]);
}

/* ---- persistence --------------------------------------------------
 * Format is deliberately dumb: one record per entry,
 *   "<P|H> <unix-ts> <byte-len>\n<raw bytes><\n>"
 * Length-prefixing (rather than escaping) means entries can contain
 * anything -- embedded newlines, NULs in the middle, whatever the X
 * selection handed us -- with no encoding step. */

static void savehistory(void) {
  char path[1024];
  FILE *f;
  ClipEntry *e;

  if (!clippath(path, sizeof path))
    return;
  if (!(f = fopen(path, "w")))
    return;
  for (e = pinned; e; e = e->next) {
    fprintf(f, "P %ld %zu\n", (long)e->ts, e->len);
    fwrite(e->text, 1, e->len, f);
    fputc('\n', f);
  }
  for (e = history; e; e = e->next) {
    fprintf(f, "H %ld %zu\n", (long)e->ts, e->len);
    fwrite(e->text, 1, e->len, f);
    fputc('\n', f);
  }
  fclose(f);
}

static void loadhistory(void) {
  char path[1024];
  FILE *f;
  char kind;
  long ts;
  size_t len;
  char *buf;
  ClipEntry *e, *tail;

  if (!clippath(path, sizeof path))
    return;
  if (!(f = fopen(path, "r")))
    return; /* no history yet, not an error */

  while (fscanf(f, " %c %ld %zu\n", &kind, &ts, &len) == 3) {
    if (len > CLIP_MAX_ENTRY)
      break; /* corrupt/oversized record: bail rather than guess */
    buf = ecalloc(1, len + 1);
    if (len && fread(buf, 1, len, f) != len) {
      free(buf);
      break;
    }
    fgetc(f); /* consume the trailing '\n' separator */

    e = ecalloc(1, sizeof(ClipEntry));
    e->text = buf;
    e->len = len;
    e->ts = ts;
    e->pinned = (kind == 'P');
    e->next = NULL;

    if (e->pinned) {
      if (pinned) {
        for (tail = pinned; tail->next; tail = tail->next)
          ;
        tail->next = e;
      } else {
        pinned = e;
      }
      pinnedcount++;
    } else {
      if (history) {
        for (tail = history; tail->next; tail = tail->next)
          ;
        tail->next = e;
      } else {
        history = e;
      }
      historycount++;
    }
  }
  fclose(f);

  if (history && (!pinned || history->ts >= pinned->ts))
    lastentry = history;
  else
    lastentry = pinned;
}

/* ---- capping -------------------------------------------------------
 * Both lists are singly linked and newest-first, so evicting the
 * oldest entry means walking to the second-to-last node. Fine at
 * these caps (a couple hundred entries, done rarely). */

static void cliptrim(void) {
  ClipEntry *e, *prev;

  while (historycount > CLIP_MAX_HISTORY && history) {
    if (!history->next) {
      if (history == lastentry)
        lastentry = NULL;
      free(history->text);
      free(history);
      history = NULL;
      historycount = 0;
      break;
    }
    for (prev = history; prev->next->next; prev = prev->next)
      ;
    e = prev->next;
    prev->next = NULL;
    if (e == lastentry)
      lastentry = NULL; /* the oldest is never the most recent, but
                         * stay defensive */
    free(e->text);
    free(e);
    historycount--;
  }

  while (pinnedcount > CLIP_MAX_PINNED && pinned) {
    if (!pinned->next) {
      if (pinned == lastentry)
        lastentry = NULL;
      free(pinned->text);
      free(pinned);
      pinned = NULL;
      pinnedcount = 0;
      break;
    }
    for (prev = pinned; prev->next->next; prev = prev->next)
      ;
    e = prev->next;
    prev->next = NULL;
    if (e == lastentry)
      lastentry = NULL;
    free(e->text);
    free(e);
    pinnedcount--;
  }
}

static void clipboardpush(const unsigned char *data, unsigned long nitems) {
  ClipEntry *e;
  size_t len = nitems;

  if (len == 0)
    return;
  if (len > CLIP_MAX_ENTRY)
    len = CLIP_MAX_ENTRY;
  /* Dedup against whatever we most recently captured -- this is what
   * keeps xclip re-asserting ownership of an entry we just picked
   * from history from spawning a duplicate at the top of the list
   * (see clippick()/copytextclip() below). */
  if (lastentry && lastentry->len == len &&
      memcmp(lastentry->text, data, len) == 0)
    return;

  e = ecalloc(1, sizeof(ClipEntry));
  e->text = ecalloc(1, len + 1);
  memcpy(e->text, data, len);
  e->text[len] = '\0';
  e->len = len;
  e->ts = time(NULL);
  e->pinned = 0;
  e->next = history;
  history = e;
  historycount++;
  lastentry = e;

  cliptrim();
  savehistory();
}

/* ---- X plumbing ----------------------------------------------------- */

void clipboardsetup(void) {
  clipboardatom = XInternAtom(dpy, "CLIPBOARD", False);
  utf8atom = XInternAtom(dpy, "UTF8_STRING", False);
  clipdwmprop = XInternAtom(dpy, "DWM_CLIP_SELECTION", False);

  clipwin = XCreateSimpleWindow(dpy, root, -1, -1, 1, 1, 0, 0, 0);

  XFixesSelectSelectionInput(dpy, root, clipboardatom,
                              XFixesSetSelectionOwnerNotifyMask |
                                  XFixesSelectionWindowDestroyNotifyMask |
                                  XFixesSelectionClientCloseNotifyMask);

  loadhistory();
  clipboardactive = 1;

  /* Seed history with whatever's already on the clipboard at startup,
   * the same way a live change would. */
  if (XGetSelectionOwner(dpy, clipboardatom) != None)
    XConvertSelection(dpy, clipboardatom, utf8atom, clipdwmprop, clipwin,
                       CurrentTime);
}

void clipboardcleanup(void) {
  ClipEntry *e, *next;

  if (!clipboardactive)
    return;
  savehistory();
  for (e = history; e; e = next) {
    next = e->next;
    free(e->text);
    free(e);
  }
  for (e = pinned; e; e = next) {
    next = e->next;
    free(e->text);
    free(e);
  }
  history = pinned = NULL;
  lastentry = NULL;
  if (clipwin != None)
    XDestroyWindow(dpy, clipwin);
  clipboardactive = 0;
}

void clipboardfixesnotify(XEvent *e) {
  XFixesSelectionNotifyEvent *xfe = (XFixesSelectionNotifyEvent *)e;

  if (!clipboardactive || xfe->selection != clipboardatom)
    return;
  if (xfe->subtype != XFixesSetSelectionOwnerNotify || xfe->owner == None)
    return;
  XConvertSelection(dpy, clipboardatom, utf8atom, clipdwmprop, clipwin,
                     xfe->timestamp);
}

void clipboardselectionnotify(XEvent *e) {
  XSelectionEvent *se = &e->xselection;
  Atom type;
  int format;
  unsigned long nitems, after;
  unsigned char *data = NULL;

  if (!clipboardactive || se->requestor != clipwin || se->property == None)
    return;
  if (XGetWindowProperty(dpy, clipwin, clipdwmprop, 0, CLIP_MAX_ENTRY / 4,
                         True /* delete the property once read */,
                         AnyPropertyType, &type, &format, &nitems, &after,
                         &data) != Success ||
      !data)
    return;
  clipboardpush(data, nitems);
  XFree(data);
}

/* ---- dmenu picker ---------------------------------------------------
 * Each line is "<index> <pin-marker><preview>\n"; index maps back into
 * a parallel array built in the same emission order (pinned first,
 * then history), so parsing the chosen line's leading integer is
 * enough to recover the full (untruncated) entry. */

static void clipappendline(char **menu, size_t *menulen, size_t *menucap,
                            ClipEntry **index, size_t *n, ClipEntry *e,
                            int ispinned) {
  char preview[CLIP_PREVIEW_LEN + 1];
  size_t i, pl = 0;
  char line[CLIP_PREVIEW_LEN + 32];
  int linelen;

  /* Byte-truncated, not UTF-8-aware -- a multi-byte codepoint can get
   * cut mid-sequence in the preview. Cosmetic only: the full original
   * text is still what gets copied back, this only affects how the
   * last character or two of a long preview line renders in dmenu. */
  for (i = 0; i < e->len && pl < CLIP_PREVIEW_LEN; i++) {
    unsigned char c = (unsigned char)e->text[i];
    preview[pl++] = (c == '\n' || c == '\r' || c == '\t') ? ' ' : (char)c;
  }
  preview[pl] = '\0';

  linelen = snprintf(line, sizeof line, "%3zu %s%s\n", *n,
                     ispinned ? "\xE2\x98\x85 " : "  ", preview);
  if (linelen < 0)
    return;

  if (*menulen + (size_t)linelen + 1 > *menucap) {
    *menucap = (*menulen + (size_t)linelen + 1) * 2;
    *menu = erealloc(*menu, *menucap);
  }
  memcpy(*menu + *menulen, line, (size_t)linelen);
  *menulen += (size_t)linelen;

  index[*n] = e;
  (*n)++;
}

void clippick(const Arg *arg) {
  /* Tune these flags to match whatever styling your dmenu_run wrapper
   * normally passes (e.g. -nb/-nf/-fn) if you want the picker to look
   * consistent with the rest of your dmenu-driven scripts. */
  const char *const dmenuargv[] = {"dmenu", "-l", "20",
                                   "-p", "Clipboard History", NULL};
  ClipEntry **index;
  size_t cap, n = 0, menulen = 0, menucap = 4096;
  char *menu;
  ClipEntry *e;
  char reply[32];
  int id;

  if (!clipboardactive)
    return;

  cap = (size_t)pinnedcount + (size_t)historycount;
  if (cap == 0)
    return;

  index = ecalloc(cap, sizeof(ClipEntry *));
  menu = ecalloc(1, menucap);

  for (e = pinned; e; e = e->next)
    clipappendline(&menu, &menulen, &menucap, index, &n, e, 1);
  for (e = history; e; e = e->next)
    clipappendline(&menu, &menulen, &menucap, index, &n, e, 0);

  if (runargv_io(dmenuargv, menu, menulen, reply, sizeof reply) == 0 &&
      sscanf(reply, "%d", &id) == 1 && id >= 0 && (size_t)id < n) {
    copytextclip(index[id]->text, index[id]->len);
    /* Update lastentry immediately so the XFixesSelectionNotify that
     * xclip's ownership-take triggers a moment from now dedupes clean
     * against what we just set, instead of appearing to be a new
     * copy. */
    lastentry = index[id];
  }

  free(menu);
  free(index);
}

/* Unlinks `e` from `*from` (wherever it sits in that singly linked
 * list) and re-inserts it at the head of `*to`, flipping its pinned
 * flag and updating both counts. No-ops if `e` isn't actually in
 * `*from`.
 *
 * This used to assume `e` was always the head of `*from` -- true for
 * a freshly-pushed entry, but clippick() can set `lastentry` to any
 * entry in the list (whichever one the user picked from dmenu, not
 * necessarily the newest). With the old head-only check, pinning
 * (Mod+Ctrl+c) after picking anything but the very newest history
 * entry silently did nothing. This walks the list to find and unlink
 * `e` properly instead. */
static void cliplistmove(ClipEntry **from, ClipEntry **to, ClipEntry *e,
                          int *fromcount, int *tocount) {
  ClipEntry *prev;

  if (*from == e) {
    *from = e->next;
  } else {
    for (prev = *from; prev && prev->next != e; prev = prev->next)
      ;
    if (!prev)
      return; /* e isn't in *from at all */
    prev->next = e->next;
  }
  e->next = *to;
  *to = e;
  e->pinned = !e->pinned;
  (*fromcount)--;
  (*tocount)++;
}

void clippin(const Arg *arg) {
  if (!clipboardactive || !lastentry)
    return;
  if (lastentry->pinned)
    cliplistmove(&pinned, &history, lastentry, &pinnedcount, &historycount);
  else
    cliplistmove(&history, &pinned, lastentry, &historycount, &pinnedcount);
  cliptrim();
  savehistory();
}

void clipclear(const Arg *arg) {
  ClipEntry *e, *next;

  if (!clipboardactive)
    return;
  for (e = history; e; e = next) {
    next = e->next;
    if (e == lastentry)
      lastentry = NULL;
    free(e->text);
    free(e);
  }
  history = NULL;
  historycount = 0;
  savehistory();
}