# DOCS — Code Layout & Internals

This document describes how the source is organized, for anyone (including
future-you) editing `dwm.c` directly. Configuration values live in
`config.h` and are covered in `WIKI.md` instead.

## File overview

| File                | Purpose                                                                                  |
| ------------------- | ---------------------------------------------------------------------------------------- |
| `dwm.c`             | Everything: event loop, layouts, client management, wallpaper engine, FIFO IPC           |
| `drw.c` / `drw.h`   | Drawing primitives (the "drw" library) — fonts, colors, the status bar surface           |
| `util.c` / `util.h` | Small helpers (`die()`, `ecalloc()`, the `LENGTH()`/`MAX()`/`MIN()` macros)              |
| `movestack.c`       | Implementation of the `movestack` patch, `#include`d directly into `config.h`            |
| `transient.c`       | Transient-window handling helper, `#include`d where needed                               |
| `config.def.h`      | Upstream default config — **do not edit**, copy to `config.h` instead                    |
| `config.h`          | Your actual config — compiled directly into the binary                                   |
| `config.mk`         | Build flags, install prefix, library paths                                               |
| `autostart.sh`      | Shell script run once at dwm startup to launch background processes                      |
| `patches/`          | Reference copies of the patches already merged into `dwm.c` (kept for diffing/upgrading) |

## Program flow

```
main()
  ├─ XOpenDisplay()
  ├─ checkotherwm()        — refuse to start if another WM owns the display
  ├─ autostart_exec()      — run autostart.sh
  ├─ setup()               — screen geometry, atoms, cursors, the bar, signal handlers
  ├─ setupfifo()           — create/open the IPC FIFO
  ├─ scan()                — adopt any windows already mapped
  ├─ setrandomwallpaper()  — initial wallpaper draw
  └─ run()                 — the main event loop (see below)
       └─ (on exit) cleanup() → XCloseDisplay()
```

If `restart` is set (via the FIFO `quit` command, `SIGHUP`, or a keybind
calling `quit(&(Arg){.i = 1})`), `main()` re-`execvp()`s itself instead of
exiting — this is what lets you reload after editing `config.h` without
losing your X session.

## The event loop (`run()`)

```c
void run(void) {
  while (running) {
    if (wallpaperupdate) { wallpaperupdate = 0; setrandomwallpaper(); }
    if (fifofd >= 0) readfifo();
    if (XPending(dpy)) {
      XNextEvent(dpy, &ev);
      handler[ev.type](&ev);
    } else {
      nanosleep(10ms);
    }
  }
}
```

This is **not** a `select()`/`epoll()` loop — it's a busy-poll with a 10ms
sleep when there's nothing to do. That's why adding new event sources (the
wallpaper timer flag, the FIFO) was just a matter of checking a flag/fd at
the top of the loop rather than restructuring it. Latency for FIFO commands
and signal-triggered wallpaper changes is bounded by that 10ms tick.

`handler[]` is a lookup table indexed by X11 event type (`ButtonPress`,
`KeyPress`, `PropertyNotify`, etc.) mapping to the function that handles
that event — this is stock dwm's dispatch mechanism, untouched by this fork.

## Client/window lifecycle

- `manage()` — a new window is adopted: rules are applied (`applyrules()`),
  size hints read, and it's attached to the client list (`attach()` /
  `attachBelow()` depending on the `attachbelow` setting).
- `unmanage()` — window is destroyed or withdrawn; detached from client and
  stacking lists.
- `focus()` / `unfocus()` — input focus and border-color changes.
- `arrange()` → `arrangemon()` → the active `Layout`'s `arrange` function
  (`tile()`, `monocle()`, or `NULL` for floating) — recomputes geometry for
  all visible clients on a monitor.

Per-tag state (layout, mfact, nmaster, selected client) is tracked via the
`Pertag` struct attached to each `Monitor`, populated/restored on `view()` —
this is the `pertag` patch's mechanism.

## Wallpaper engine (custom, not a suckless patch)

Functions: `setwallpaper()`, `setrandomwallpaper()`, `nextwallpaper()`.

- Wallpaper images are loaded via **Imlib2**, scaled to each monitor's
  geometry, and pushed to the root window as a `Pixmap`.
- `currentwallpaper[32]` and `lastwallpaper[32][2048]` track per-monitor
  state (indexed by monitor number) so each monitor can show a different
  image and avoid redundant reloads.
- Triggering:
  - `SIGALRM`, fired on the interval set by `wallpaperinterval`, sets the
    `wallpaperupdate` flag → picked up by `run()`.
  - `SIGUSR1` does the same thing on demand (`kill -USR1 $(pidof dwm)`).
  - The `MODKEY+SHIFT+w` keybind calls `nextwallpaper()` directly.
  - The FIFO `nextwallpaper` command calls the same function.
- `~` in `wallpaperdir` is expanded against `$HOME` manually (X11/Imlib2
  don't do shell-style expansion).

### Async loading

The slow part of a wallpaper change — Imlib2 file decode and scale — runs
in a detached `pthread` (`wallpaperworker`). The worker does **no X calls**:
it produces a raw `uint32_t` pixel buffer (`DATA32 *`) and pushes it onto a
mutex-protected `wpqueue` singly-linked list, then sets `wallpaperready = 1`.

`run()` checks `wallpaperready` on every tick and drains the queue by calling
`applywallpaperresult()` for each entry. `applywallpaperresult()` does all X
work on the main thread: `XCreatePixmap` + `XPutImage` from the raw buffer,
then `XCopyArea` to the root window, then `rebuildrootwallpaper()`.

The reason X calls must stay off the worker thread: X11 ties all resources
(Pixmaps, GCs, etc.) to the client connection that created them. If the
worker created a Pixmap on its own `Display*` and then called
`XCloseDisplay()`, the server would immediately free that Pixmap — leaving
the main thread holding a dangling ID and crashing on the next `XCopyArea`.
Passing raw pixel data instead of a Pixmap ID sidesteps this entirely.

Only one worker job runs at a time — `wpthreadrunning` is checked before
`pthread_create` and a new request is silently skipped if a job is already
in flight. Given the 900s default interval this is never a practical
constraint, but it keeps the queue logic simple and bounded.

## Monitor hotplug handling (custom, not a suckless patch)

Functions: `applygeomchange()`, `rrscreenchangenotify()`, plus a small
addition to `configurenotify()` and `setup()`.

dwm's existing `updategeom()` (stock + Xinerama) already knows how to
diff the current monitor list against a fresh Xinerama query and add/remove
`Monitor`s accordingly — it was just never _triggered_ except by a root
window resize (`configurenotify()`). Plugging/unplugging an external
monitor doesn't always also resize the root window, depending on the
driver, so hotplug events could be silently missed.

- `setup()` calls `XRRQueryExtension()` to ask the X server for the RandR
  extension's event base, stored in the `rrbase` global, then
  `XRRSelectInput(dpy, root, RRScreenChangeNotifyMask)` to subscribe to
  hotplug/resolution-change notifications on the root window.
- RandR event types aren't fixed core-protocol constants — the server
  reports the base at runtime, and the real event type is
  `rrbase + RRScreenChangeNotify`. This number can fall outside the range
  `handler[]` is indexed for, so it can't be dropped into that dispatch
  table like ordinary events. Instead, `run()` checks for it explicitly
  before falling back to `handler[ev.type]`.
- `rrscreenchangenotify()` calls `XRRUpdateConfiguration()` first — this
  refreshes Xlib's cached screen/rotation info, which Xinerama's query
  depends on — then calls the existing `updategeom()`.
- `applygeomchange()` is `configurenotify()`'s old "something changed,
  now fix everything up" body (resize the bar/drw, reposition fullscreen
  clients, move bar windows, refocus, rearrange), factored out so both
  the resize path and the new hotplug path share one implementation
  instead of duplicating it.

No `config.h` setting controls this — it's always active once RandR is
available, since there's no meaningful reason to disable monitor detection.

### Known quirk: stale geometry immediately after hotplug

On some GPU drivers, Xinerama's screen list lags a few milliseconds behind
the RandR event that announces a hotplug — so `updategeom()` can fire
_before_ Xinerama has actually updated, and report the old monitor count.

If you notice a freshly plugged monitor not appearing until you trigger
another event (e.g. resize a window, or unplug/replug again), add a short
delay before the `updategeom()` call in `rrscreenchangenotify()`:

```c
void rrscreenchangenotify(XEvent *e) {
  XRRUpdateConfiguration(e);
  usleep(50000); /* 50ms — let Xinerama catch up to RandR */
  if (updategeom())
    applygeomchange();
}
```

This isn't applied by default since it adds a small (if imperceptible)
delay to every screen-change event, and most setups don't need it. Only
add it if you actually observe the lag on your hardware.

## FIFO IPC layer (custom)

Functions: `setupfifo()`, `readfifo()`, plus the `FifoCmd` dispatch table.

- `setupfifo()` (called once from `main()`) creates the FIFO at `fifopath`
  (`config.h`) if it doesn't exist, then opens it **`O_RDWR | O_NONBLOCK`**.
  `O_RDWR` (rather than `O_RDONLY`) is deliberate: a FIFO opened read-only
  blocks until a writer attaches, and errors when the last writer detaches.
  Opening it read-write sidesteps both, so the `run()` loop never stalls.
- `readfifo()` is called every iteration of `run()`. It performs a
  non-blocking `read()` into a small static buffer, then processes any
  complete (`\n`-terminated) lines it finds, leaving partial lines in the
  buffer for the next call.
- Each line is `sscanf`'d into a command name and an optional single
  argument, looked up in `fifocmds[]`, and dispatched to the matching
  function with an `Arg` built according to that command's declared
  `argtype` (`int`/`uint`/`float`/none).
- `fifoviewtag()` / `fifotagtag()` exist purely to translate a human-typed
  tag _index_ (`view 3`) into the bitmask dwm's `view()`/`tag()` actually
  expect (`1 << 3`) — every other FIFO command calls dwm's existing
  `Arg`-taking functions directly, no wrapper needed.
- `cleanup()` closes the fd and `unlink()`s the FIFO path on exit/restart.

See `WIKI.md` → "FIFO Commands" for the full command table and usage
examples.

## Adding a new FIFO command

1. If the function doesn't already take an `Arg*` with the shape you need,
   add a one-line wrapper near `fifoviewtag`/`fifotagtag`.
2. Add a row to `fifocmds[]`: `{"yourcmd", yourfunc, argtype}`.
3. Rebuild. No other wiring needed — `readfifo()`'s dispatch loop is generic.

## Adding a new patch

Patches in `patches/` were merged by hand into `dwm.c`/`config.h` rather
than applied with `patch(1)` against a clean tree, since several interact
(e.g. `pertag` + `uselessgap` + `attachbelow` all touch `arrange()`/`attach()`).
If you add a new upstream patch, expect to merge it manually against the
current `dwm.c` rather than apply it blind — diff against `dwm.c.orig` /
`config.def.h.orig` if you need to see what stock dwm looked like before
any patches were applied.
