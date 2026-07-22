# DOCS — Code Layout & Internals

This document describes how the source is organized, for anyone (including
future-you) editing `dwm.c` directly. Configuration values live in
`config.h` and are covered in `WIKI.md` instead.

## File overview

| File                  | Purpose                                                                                                                                                          |
| --------------------- | ---------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `dwm.c`               | Event loop, layouts, client management -- the stock-dwm core plus the merged patches                                                                             |
| `dwm.h`               | Shared surface between `dwm.c` and the modules below: `Arg`/`Client`/`Monitor` types, `ISVISIBLE`, and externs for the globals/functions those modules call into |
| `wallpaper.c` / `.h`  | Async Imlib2 wallpaper engine (custom, not a suckless patch)                                                                                                     |
| `ipc.c` / `.h`        | FIFO-based remote control (custom, not a suckless patch)                                                                                                         |
| `screenshot.c` / `.h` | Screenshot capture + colorpicker (custom, not a suckless patch)                                                                                                  |
| `movestack.c` / `.h`  | Implementation of the `movestack` patch -- its own translation unit, declared in `keys[]` via `#include "movestack.h"` in `config.h`                             |
| `drw.c` / `drw.h`     | Drawing primitives (the "drw" library) -- fonts, colors, the status bar surface                                                                                  |
| `util.c` / `util.h`   | Small helpers (`die()`, `ecalloc()`, the `LENGTH()`/`MAX()`/`MIN()` macros)                                                                                      |
| `transient.c`         | Transient-window handling helper, `#include`d where needed                                                                                                       |
| `config.def.h`        | Upstream default config -- **do not edit**, copy to `config.h` instead                                                                                           |
| `config.h`            | Your actual config -- compiled directly into the binary                                                                                                          |
| `config.mk`           | Build flags, install prefix, library paths                                                                                                                       |
| `autostart.sh`        | Shell script run once at dwm startup to launch background processes                                                                                              |
| `patches/`            | Reference copies of the patches already merged into `dwm.c` (kept for diffing/upgrading)                                                                         |

### Why the split

`dwm.c` used to contain everything, including three sizeable, largely
self-contained subsystems (wallpaper, IPC, screenshots) that don't touch
client/layout internals. Those three got pulled into their own `.c`/`.h`
pairs to keep `dwm.c` itself focused on the actual window manager.
`movestack.c` was already a separate file but was previously `#include`d
as text from `config.h` rather than compiled as its own translation unit
-- it's now wired up the same way as the other three.

`dwm.h` exists solely so those modules have something to compile against.
It is **not** a general-purpose dwm header -- it only exposes what's
actually used across a file boundary (checked against real call sites,
not copied wholesale). If you add a new cross-file dependency, add the
specific type/extern/function to `dwm.h` rather than widening what any
one module `#include`s.

One consequence worth knowing: `config.h` still does
`#include "movestack.h"` before the `keys[]` table, and `config.h` is
still `#include`d only once, from `dwm.c`. If you split another chunk of
`dwm.c` into its own file and that file also needs a `config.h` value
(the way `wallpaper.c` needs `wallpaperdir`), don't `#include "config.h"`
from the new file -- it'll pull in `movestack.h`/`keys[]`/etc. again, and
if `config.h` ever textually includes another `.c` file the way it used
to for `movestack.c`, that's a duplicate-symbol link error waiting to
happen. Instead, give the specific `config.h` variable external linkage
(drop `static` from just that line) and declare it `extern` in the new
file, the way `wallpaperdir` and `fifopath` are handled now.

## Program flow

```
main()
  ├─ XOpenDisplay()
  ├─ checkotherwm()        — refuse to start if another WM owns the display
  ├─ autostart_exec()      — run autostart.sh
  ├─ setup()               — screen geometry, atoms, cursors, the bar, signal handlers
  ├─ setupfifo()           — create/open the IPC FIFO (ipc.c)
  ├─ scan()                — adopt any windows already mapped
  ├─ setrandomwallpaper()  — initial wallpaper draw (wallpaper.c)
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

## Status bar rendering & clicks (`status2d` + `statuscmd` patches)

Functions: `drawbar()`, `drawstatusbar()` (status2d escape-code parsing),
`buttonpress()`, `sigstatusbar()`, `getstatusbarpid()`. All still in
`dwm.c` — these are two of the merged suckless patches, not a custom
module, so they weren't split into their own files like wallpaper/ipc/
screenshot were.

- `drawstatusbar()` draws the raw text dwmblocks writes to the root
  window's name (`stext`), parsing inline `^c#hex^` / `^b#hex^` / `^f<N>^`
  escape codes for foreground/background color and horizontal offset (the
  `status2d` patch) as it goes. It returns the pixel width of what it drew.
- `drawbar()` calls it and stores the result on the monitor:
  `m->stw = m->ww - drawstatusbar(...)`. `m->stw` is the authoritative
  "how wide is the status text" value for that monitor — `title_end`
  (used to decide whether a click landed in the status region at all) is
  computed from it: `title_end = m->ww - m->stw`.
- `buttonpress()` re-walks `stext` on a click inside that region, this
  time looking for the raw control bytes dwmblocks embeds ahead of each
  clickable block's output (see the dwmblocks-async repo's `DOCS.md` for
  the embedding side). Whichever byte the click's x-coordinate falls
  under becomes `statussig`.
- `sigstatusbar()` (wired via the `buttons[]` table's
  `{ClkStatusText, 0, ButtonN, sigstatusbar, {.i = N}}` entries) turns
  that into `sigqueue(getstatusbarpid(), SIGRTMIN + statussig,
{.sival_int = N})` — the button number rides along as the signal's
  payload. `getstatusbarpid()` caches the resolved PID and re-validates
  it against `/proc/<pid>/cmdline` before reusing it, falling back to
  `pidof -s dwmblocks` if that check fails.
- If `statussig` ends up `0` — either the click landed outside any
  clickable block, or that block was defined with signal `0` in
  dwmblocks' `BLOCKS()` — `sigstatusbar()` returns immediately without
  queuing anything. There's no such thing as a real "signal 0" block; `0`
  is the sentinel dwmblocks uses for "not clickable."

### Known bug (fixed): status-click x-origin read the wrong variable

`buttonpress()`'s `ClkStatusText` branch computes its scan's starting
x-coordinate as:

```c
int x = selmon->ww - statusw;
```

For this to work, `statusw` needs to be the same _file-scope_ storage
that `drawbar()` writes to — and it is declared at file scope
(`static int statusw;`) for exactly that reason. But `drawbar()` actually
does:

```c
int statusw = m->ww - drawstatusbar(m, bh, stext);  /* local — shadows the global */
m->stw = statusw;
```

That `int statusw = ...` is a _local_ declaration that shadows the
file-scope one for the rest of `drawbar()`'s body. The real width only
ever gets written into `m->stw`; the file-scope `statusw` that
`buttonpress()` reads is never assigned anywhere in the file and stays at
its zero-initialized default forever. `x` in `buttonpress()` therefore
always evaluates to `selmon->ww` — the monitor's far right edge — instead
of the actual left edge of the status text. Since that's `>=` any valid
on-screen `ev->x`, the scan loop's `x <= ev->x` condition fails before
its first iteration, `statussig` is left at `0`, and `sigstatusbar()`
silently no-ops every time.

Net effect: status-text clicks did nothing at all, with no error anywhere
— while every other clickable bar region (tags, layout symbol, window
title) worked fine, since none of them touch this variable. Easy to miss
precisely because the rest of the bar's click handling looks completely
correct.

**Fix:** read the monitor field `drawbar()` actually populates, instead
of the dead global:

```c
int x = selmon->ww - selmon->stw;
```

and delete the now-unused `static int statusw;` file-scope declaration
entirely (leaving it in place still compiles, but trips
`-Wunused-variable`).

## Wallpaper engine (`wallpaper.c` / `wallpaper.h`, custom, not a suckless patch)

Functions: `setrandomwallpaper()`, `nextwallpaper()`, `applywallpaperresult()`,
`refreshdamagedwallpapers()` are the public surface (declared in
`wallpaper.h`); everything else in this section (`wallpaperworker()`,
`rebuildrootwallpaper()`, the `wallpapercache_*()` family,
`dispatchwallpaperjobs()`) is `static` inside `wallpaper.c`.

- Wallpaper images are loaded via **Imlib2**, scaled to each monitor's
  geometry, and pushed to the root window as a `Pixmap`.
- `currentwallpaper[32]` and `lastwallpaper[32][2048]` track per-monitor
  state (indexed by monitor number) so each monitor can show a different
  image and avoid redundant reloads.
- Triggering:
  - `SIGALRM`, fired on the interval set by `wallpaperinterval`, sets the
    `wallpaperupdate` flag → picked up by `run()`. `wallpaperupdate`,
    `wallpaperready`, `wplock`, and `wpqueue` are defined in `wallpaper.c`
    and declared `extern` in `wallpaper.h` specifically so `run()` (still
    in `dwm.c`) can keep polling them each tick without needing a wrapper
    function call.
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

## Screenshot capture (`screenshot.c` / `screenshot.h`, custom, not a suckless patch)

Functions: `takescreenshot()`, `screenshotpath()`, `selectregion()`,
`pickcolor()`, `copytoclip()`, `copytextclip()`, `notifyshot()`,
`notifycolor()`. Only `takescreenshot()` and `pickcolor()` are public
(declared in `screenshot.h`, called from `ipc.c`'s `fifocmds[]` table and
from `keys[]`/`buttons[]` in `config.h`); the rest are `static` inside
`screenshot.c`.

- `takescreenshot()` grabs the root window via
  `imlib_create_image_from_drawable()`, then crops to a rectangle depending
  on `arg->i` (`ShotFull` = whole root, `ShotScreen` = `selmon`'s geometry,
  `ShotWindow` = `selmon->sel`'s geometry, `ShotSelect` = a user-dragged
  rectangle from `selectregion()`) using `imlib_create_cropped_image()`,
  and saves as PNG via `imlib_save_image()`. Reuses the same Imlib2 context
  calls as the wallpaper engine — no new library dependency.
- `screenshotpath()` builds `~/Pictures/Screenshots/<timestamp>.png`,
  creating the directory if missing.
- `selectregion()` follows the `movemouse()`/`resizemouse()` pointer-grab
  pattern: grabs the pointer on `root`, waits for `ButtonPress`, then tracks
  `MotionNotify` and redraws a rectangle on `root` using an XOR `GC`
  (`GXxor`) so each redraw erases the previous frame instead of needing a
  full repaint. `ButtonRelease` ends the grab. An `XSync()` is required
  right after the final erase — without it there's a race where
  `takescreenshot()`'s root grab can happen before the last XOR erase is
  flushed to the server, leaving a ghost rectangle baked into the capture.
- `pickcolor()` grabs the pointer, waits for a single `ButtonPress`, then
  reads the pixel under the cursor via `XGetImage()` on a 1×1 region and
  resolves it to RGB with `XQueryColor()`.
- Clipboard and notifications are deliberately **not** native. `copytoclip()`
  (image, via a file path) and `copytextclip()` (color hex, piped over
  stdin) each `fork()` + `execlp()` a thin external tool (`xclip`), same for
  `notifyshot()`/`notifycolor()` (`notify-send`) — rather than dwm
  implementing ICCCM selection ownership or a D-Bus notification client
  itself. Same reasoning as `spawn()`: dwm launches things, it doesn't
  become them. `SIGCHLD` is already `SA_NOCLDWAIT` (see `setup()`), so
  these forked children never need to be waited on.

## FIFO IPC layer (`ipc.c` / `ipc.h`, custom)

Functions: `setupfifo()`, `readfifo()`, plus the `FifoCmd` dispatch table
and the `fifo*` wrapper functions -- all in `ipc.c` now. `setupfifo()` and
`readfifo()` are the only two declared in `ipc.h`; the dispatch table and
wrappers are `static` inside `ipc.c`. `fifofd` is defined in `ipc.c` and
declared `extern` in `ipc.h` so `run()` and `cleanup()` (still in `dwm.c`)
can check/close it directly, same pattern as the wallpaper globals above.

- `setupfifo()` (called once from `main()`) creates the FIFO at `fifopath`
  (`config.h`) if it doesn't exist, then opens it **`O_RDWR | O_NONBLOCK`**.
  `O_RDWR` (rather than `O_RDONLY`) is deliberate: a FIFO opened read-only
  blocks until a writer attaches, and errors when the last writer detaches.
  Opening it read-write sidesteps both, so the `run()` loop never stalls.
  The same function also creates/opens a second FIFO at `fiforeplypath`
  into `fiforeplyfd`, using the identical `O_RDWR | O_NONBLOCK` trick, for
  the query side described below.
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
  `Arg`-taking functions directly, no wrapper needed. Those target
  functions (`view`, `tag`, `setmfact`, `show`, `quit`, etc.) are declared
  in `dwm.h` so `ipc.c` can call them.
- `cleanup()` closes both fds and `unlink()`s both FIFO paths on
  exit/restart.

**Query side.** `fifopath` is write-only from the caller's perspective —
dwm never talks back on it. The `state` command is the one exception:
`fifostate()` (static in `ipc.c`) builds a one-line summary (monitor,
tagset, layout name, visible client count, urgent tag bitmask, focused client title) and
`write()`s it to `fiforeplyfd`. Before writing, it drains any bytes still
unread from a previous `state` call — since the fifo is opened `O_RDWR` by
dwm itself, a write with no external reader would otherwise just
accumulate in the pipe buffer indefinitely rather than blocking or
failing. This makes `fiforeplypath` last-write-wins: read it immediately
after sending `state`, don't treat it as a persistent state file.

See `WIKI.md` → "FIFO Commands" for the full command table and usage
examples.

## Adding a new FIFO command

1. If the target function doesn't already take an `Arg*` with the shape
   you need, add a one-line wrapper in `ipc.c` near `fifoviewtag`/`fifotagtag`.
2. If the target function lives in `dwm.c` and isn't already declared in
   `dwm.h`, add its prototype there (drop `static` from its forward
   declaration in `dwm.c` too). Functions in `wallpaper.h`/`screenshot.h`
   are already visible to `ipc.c`.
3. Add a row to `fifocmds[]` in `ipc.c`: `{"yourcmd", yourfunc, argtype}`.
4. Rebuild. No other wiring needed — `readfifo()`'s dispatch loop is generic.

If your command needs to report something back (rather than just act),
follow the `fifostate()` pattern instead of adding output to the regular
command path: write to `fiforeplyfd`, guard against `fiforeplyfd < 0`, and
drain unread bytes first so repeated unread queries can't grow the pipe.

## Adding a new patch

Patches in `patches/` were merged by hand into `dwm.c`/`config.h` rather
than applied with `patch(1)` against a clean tree, since several interact
(e.g. `pertag` + `uselessgap` + `attachbelow` all touch `arrange()`/`attach()`).
If you add a new upstream patch, expect to merge it manually against the
current `dwm.c` rather than automaticly apply the patch.
