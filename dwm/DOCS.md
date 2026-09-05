# DOCS — Code Layout & Internals

This document describes how the source is organized, for anyone (including
future-you) editing `dwm.c` directly. Configuration values live in
`config.h` and are covered in `WIKI.md` instead.

## File overview

| File                     | Purpose                                                                                                                                                          |
| ------------------------ | ---------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `dwm.c`                  | Event loop, layouts, client management -- the stock-dwm core plus the merged patches                                                                             |
| `dwm.h`                  | Shared surface between `dwm.c` and the modules below: `Arg`/`Client`/`Monitor` types, `ISVISIBLE`, and externs for the globals/functions those modules call into |
| `wallpaper.c` / `.h`     | Async Imlib2 wallpaper engine (custom, not a suckless patch)                                                                                                     |
| `clipboard.c` / `.h`     | Clipboard history: XFixes watcher, pinning, dmenu picker (custom, not a suckless patch)                                                                          |
| `ipc.c` / `.h`           | FIFO-based remote control (custom, not a suckless patch)                                                                                                         |
| `screenshot.c` / `.h`    | Screenshot capture + colorpicker (custom, not a suckless patch)                                                                                                  |
| `statusbar.c` / `.h`     | Built-in status bar blocks -- replaces the dwmblocks binary (custom, not a suckless patch)                                                                       |
| `osd.c` / `.h`           | Volume/brightness/mic/keyboard-backlight on-screen-display popup (custom, not a suckless patch)                                                                  |
| `mediaosd.c` / `.h`      | Media-player on-screen-display popup (title/artist/art/progress via the external `mediactl` script, custom, not a suckless patch)                                |
| `notifications.c` / `.h` | Standalone `org.freedesktop.Notifications` DBus server -- popups, history, DND (custom, not a suckless patch)                                                    |
| `movestack.c` / `.h`     | Implementation of the `movestack` patch -- its own translation unit, declared in `keys[]` via `#include "movestack.h"` in `config.h`                             |
| `drw.c` / `drw.h`        | Drawing primitives (the "drw" library) -- fonts, colors, the status bar surface                                                                                  |
| `util.c` / `util.h`      | Small helpers (`die()`, `ecalloc()`, the `LENGTH()`/`MAX()`/`MIN()` macros)                                                                                      |
| `transient.c`            | Transient-window handling helper, `#include`d where needed                                                                                                       |
| `theme.h`                | Central color palette -- named `CAL0`..`CAL15` + semantic aliases, `#include`d from `config.def.h`/`config.h`; the single file to edit to re-theme dwm           |
| `config.def.h`           | Upstream default config -- **do not edit**, copy to `config.h` instead                                                                                           |
| `config.h`               | Your actual config -- compiled directly into the binary                                                                                                          |
| `config.mk`              | Build flags, install prefix, library paths                                                                                                                       |
| `autostart.sh`           | Shell script run once at dwm startup to launch background processes                                                                                              |
| `patches/`               | Reference copies of the patches already merged into `dwm.c` (kept for diffing/upgrading)                                                                         |

### Why the split

`dwm.c` used to contain everything, including several sizeable, largely
self-contained subsystems (wallpaper, IPC, screenshots, and now the status
bar blocks, the OSD popup, the notification server, and the clipboard
history watcher) that don't touch client/layout internals. Those got
pulled into their own `.c`/`.h` pairs to keep `dwm.c` itself focused on
the actual window manager. `movestack.c` was already a separate file but
was previously `#include`d as text from `config.h` rather than compiled
as its own translation unit -- it's now wired up the same way as the
others.

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
  ├─ setup()               — screen geometry, atoms, cursors, the bar, signal handlers,
  │                          statusbar_init() (statusbar.c), osdsetup() (osd.c),
  │                          notifsetup() (notifications.c), clipboardsetup()
  │                          (clipboard.c, only if XFixesQueryExtension() succeeds)
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
    if (wallpaperready) { /* drain the async loader's result queue -- see
                           * "Wallpaper engine" below */ }
    if (fifofd >= 0) readfifo();
    statusbar_tick(); /* statusbar.c: reruns any block whose interval elapsed */
    osdtick();         /* osd.c: unmaps the popup once its timeout elapsed */
    mediaosdtick();     /* mediaosd.c: drives the async status/art fetch state
                          * machine forward and unmaps the popup on timeout --
                          * see "Media OSD popup" below */
    notiftick();        /* notifications.c: pumps the dbus connection
                          * non-blockingly and expires timed-out popups */
    cliptick();          /* clipboard.c: flushes a debounced history save --
                          * see "Clipboard history" below */
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
wallpaper timer flag, the FIFO, the status blocks, the OSD auto-hide, the
notification DBus connection, and now the media OSD's fetch state machine
and the clipboard save debounce) was just a matter of checking a
flag/fd/elapsed-time at the top of the loop rather than restructuring it.
Latency for FIFO commands, signal-triggered wallpaper changes, block
reruns, OSD hiding, incoming DBus notifications, media OSD updates, and
clipboard persistence is all bounded by that 10ms tick. `statusbar_tick()`
and `osdtick()` are both cheap on every iteration where nothing's actually
due -- the former is a handful of `time(NULL)` comparisons, the latter one
`clock_gettime()` comparison -- and `notiftick()`/`cliptick()` are likewise
cheap when idle (a non-blocking dbus dispatch call and a `time(NULL)`
comparison, respectively). `mediaosdtick()` is a little more involved when
a fetch is actually in flight (see "Media OSD popup" below), but each call
is still a single non-blocking `read()` attempt -- never a wait.

`handler[]` is a lookup table indexed by X11 event type (`ButtonPress`,
`KeyPress`, `PropertyNotify`, etc.) mapping to the function that handles
that event — this is stock dwm's dispatch mechanism, untouched by this fork.

## Client/window lifecycle

- `manage()` — a new window is adopted: rules are applied (`applyrules()`),
  size hints read, and it's attached to the client list (`attach()` /
  `attachBelow()` depending on the `attachbelow` setting). If the matched
  rule set `forcefullscreen`, `setfullscreen()` is called here too, right
  after `arrange(c->mon)` so the client's monitor/geometry are already
  final — same code path a client's own `_NET_WM_STATE_FULLSCREEN`
  request would hit.
- `applyrules()` — matches the new client's WM class/instance/title against
  `rules[]` using PCRE2 (`regexmatch()`), not plain substring matching.
  Each rule's compiled pattern is cached in a static per-rule/per-field
  array (`rulecache`) so it's only compiled once, on first use, not on
  every window open.
- `unmanage()` — window is destroyed or withdrawn; detached from client and
  stacking lists.
- `focus()` / `unfocus()` — input focus and border-color changes.
- `arrange()` → `arrangemon()` → the active `Layout`'s `arrange` function
  (`tile()`, `monocle()`, or `NULL` for floating) — recomputes geometry for
  all visible clients on a monitor.

Per-tag state (layout, mfact, nmaster, selected client) is tracked via the
`Pertag` struct attached to each `Monitor`, populated/restored on `view()` —
this is the `pertag` patch's mechanism.

## Status bar rendering & clicks (`status2d` + `statuscmd` patches, now feeding from `statusbar.c`)

Functions: `drawbar()`, `drawstatusbar()` (status2d escape-code parsing),
`buttonpress()`, `sigstatusbar()`, `setstatustext()`. All still in
`dwm.c` — `drawstatusbar()`/`buttonpress()` are two of the merged
suckless patches, not a custom module, so they weren't split into their
own files like wallpaper/ipc/screenshot/statusbar/osd were. What _did_
change: this used to be dwmblocks-fed (an external binary wrote to the
root window's name via `xsetroot`); it's now fed by `statusbar.c` calling
`setstatustext()` directly, in-process. `drawstatusbar()`'s parsing and
`buttonpress()`'s click-region detection don't know or care where the
text came from and are otherwise unmodified.

- `drawstatusbar()` draws whatever's currently in `stext`, parsing inline
  `^c#hex^` / `^b#hex^` / `^f<N>^` escape codes for foreground/background
  color and horizontal offset (the `status2d` patch) as it goes. It
  returns the pixel width of what it drew.
- `drawbar()` calls it and stores the result on the monitor:
  `m->stw = m->ww - drawstatusbar(...)`. `m->stw` is the authoritative
  "how wide is the status text" value for that monitor — `title_end`
  (used to decide whether a click landed in the status region at all) is
  computed from it: `title_end = m->ww - m->stw`.
- `buttonpress()` re-walks `stext` on a click inside that region, looking
  for the raw delimiter bytes `statusbar.c`'s `rebuild()` embeds after
  each block's output (value `i+1` for block index `i`, same convention
  dwmblocks used). Whichever byte the click's x-coordinate falls under
  becomes `statussig`.
- `sigstatusbar()` (wired via the `buttons[]` table's
  `{ClkStatusText, 0, ButtonN, sigstatusbar, {.i = N}}` entries) now
  calls `statusbar_handleclick(statussig, arg->i)` directly instead of
  signaling an external process — no pid to resolve, no
  `SIGRTMIN`/`sigqueue()`, no `getstatusbarpid()` (deleted). The button
  number is passed straight through as a function argument instead of
  riding along as a signal payload.
- If `statussig` ends up `0` — the click landed outside any block's
  region — `sigstatusbar()` returns immediately. There's no "block 0";
  delimiter bytes start at `1` (see `statusbar.c`'s `rebuild()`), so `0`
  is naturally never a valid block index.

See "Built-in status bar blocks" below for what `statusbar_handleclick()`
actually does once it's called.

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

## Built-in status bar blocks (`statusbar.c` / `statusbar.h`, custom, replaces dwmblocks)

Functions: `statusbar_init()`, `statusbar_tick()`, `statusbar_handleclick()`,
`statusbar_refresh()` are the public surface (declared in `statusbar.h`,
called from `dwm.c`'s `setup()`/`run()`/`sigstatusbar()` and from
`ipc.c`'s `fifocmds[]` table); `runblock()` and `rebuild()` are `static`
inside `statusbar.c`.

- `statusblocks[]` (`config.h`) is an array of `{icon, cmd, interval}`.
  `cmd` is a full shell command (run via `popen("sh -c ...")`, so pipes/
  quoting/`awk` etc. all work) — only its first line of stdout is kept,
  trailing newlines trimmed. `interval` is in seconds; `0` means the
  block only updates on click or an explicit `statusbar_refresh()` call.
- `runblock(i, button)` runs `statusblocks[i].cmd`, prefixing it with
  `BLOCK_BUTTON=<button>` in the environment when called from a click —
  same convention dwmblocks used, so any existing block script that reads
  `$BLOCK_BUTTON` doesn't need to change. Output goes into
  `blocktext[i]`, prefixed with that block's `icon`.
- `rebuild()` concatenates `blocktext[]` into a single buffer — a space,
  then each block's text, then a literal delimiter byte `(char)(i+1)` —
  and hands it to `setstatustext()` (`dwm.c`), which copies it into
  `stext` and calls `drawbar()` for every monitor. This is the exact
  shape dwmblocks used to produce, which is why `drawstatusbar()`/
  `buttonpress()` in `dwm.c` needed no changes (see above).
- `statusbar_init()` runs every block once and calls `rebuild()`; called
  from `setup()` right after `updatebars()`/`updatestatus()`, so the
  placeholder `"dwm-VERSION"` text `updatestatus()` sets is immediately
  overwritten with real block output.
- `statusbar_tick()` — called every `run()` iteration — compares
  `time(NULL)` against each block's last-run timestamp and reruns any
  block whose `interval` has elapsed, then calls `rebuild()` once if
  anything changed. Second-granularity is deliberate: this only needs to
  be "close enough" for things like a clock or battery percentage, and
  avoids a signal/timer per block.
- `statusbar_handleclick(statussig, button)` is what `sigstatusbar()`
  (`dwm.c`) now calls directly instead of signaling an external pid —
  converts the delimiter byte back to a block index (`statussig - 1`),
  reruns just that block with `button` set, and rebuilds.
- `statusbar_refresh(arg)` — `arg->i` = a specific block index, or `-1`
  for all — reruns and rebuilds outside the click path. Used by the
  `statusblock N` FIFO command and by `osd.c` (see below) to keep a bar
  block in sync immediately after a volume/brightness change, rather than
  waiting out that block's own interval.

Blocks are capped at `STATUSBAR_MAXBLOCKS` (31, see `statusbar.h`) —
delimiter bytes are literal values `1..31` (anything `< ' '` is stripped
from what's actually _drawn_ by `drawstatusbar()`, but still walked by
`buttonpress()` to resolve which block was clicked), so that's a hard
ceiling, not a tunable.

## On-screen display / OSD popup (`osd.c` / `osd.h`, custom, not a suckless patch)

Functions: `osdsetup()`, `osdcleanup()`, `osdtrigger()`, `osdtick()` are
the public surface (declared in `osd.h`); `runargv_wait()`,
`runargv_getint()`, `osdpaint()`, `osd_vol_fastget()`, `osd_bri_fastget()`,
`osd_mic_fastget()`, `osd_kbd_fastget()`, `find_kbd_backlight_dir()` are
`static` inside `osd.c`.

- `osds[]` (`config.h`) is an array of `{label, changecmd, getcmd,
blockidx, fastget}` (`OsdItem`, `osd.h`). `changecmd`/`getcmd` are
  `NULL`-terminated argv arrays (exec'd directly via `fork()`+`execvp()`,
  no shell) rather than the shell-string commands `statusblocks[]` uses --
  this fires on every repeat of a held-down volume/brightness/kbd key, so
  skipping `sh -c` matters more here than it does for a once-every-15-
  seconds clock block.
- `osdtrigger(arg)` (`arg->i` = index into `osds[]`, bound directly in
  `keys[]`) runs `changecmd` via `runargv_wait()` (fork, `execvp`,
  `waitpid` -- blocks the event loop for the duration, which in practice
  is sub-tens-of-ms for something like `sysctl`), then determines the
  level for the popup's bar one of two ways:
  - **`fastget` set (vol/bri/mic/kbd all have one).** Calls
    `o->fastget(&level, text, sizeof(text))` directly -- a plain C
    function reading straight from `wpctl`'s cached state or the relevant
    sysfs file (`osd_bri_fastget()`/`osd_kbd_fastget()` read
    `/sys/class/backlight/*`/`/sys/class/leds/*kbd*` directly), with
    **zero forks**. `text` is also written straight into the matching
    `statusblocks[]` entry via `statusbar_setblock()` when `blockidx >=
0`, so the fast path updates the bar too without a `getcmd`/
    `statusbar_refresh()` round-trip. Returning `-1` (e.g. the sysfs path
    isn't found) falls back to the `getcmd` path below for that one
    trigger, so a missing fastget dependency degrades gracefully instead
    of breaking the OSD.
  - **`fastget` NULL, or it returned -1.** Reads `getcmd`'s stdout back as
    an int via `runargv_getint()` (a `pipe()` + the same fork/exec/wait
    shape as `runargv_wait()`), then if `o->blockidx >= 0` calls
    `statusbar_refresh()` for that index so the bar doesn't visibly lag a
    beat behind the popup.

  Either way `osdpaint()` is called last with whatever level/text was
  resolved.

- `find_kbd_backlight_dir()` scans `/sys/class/leds/*` for a name
  containing `kbd`/`keyboard` (checking both `brightness` and
  `max_brightness` exist before accepting a match), falling back to a
  short list of known exact names (`kbd_backlight`,
  `platform::kbd_backlight`, `tpacpi::kbd_backlight`,
  `dell::kbd_backlight`). Caches the result after the first call. **This
  detection logic is intentionally duplicated three times** --
  `find_kbd_device()` in `sysctl` (the shell script that actually writes
  the brightness), `_kbd_backlight_dir()` in `sysstats` (the bar's
  `kbd`/`kbd_raw` blocks), and this one -- because two are shell and one
  is C. All three need to agree on the same device, or the OSD popup can
  end up unable to _read_ a level that `sysctl`/`sysstats` can still
  _write_. If you ever touch the matching rules, update all three.
- `osdpaint()` draws directly into `osddrw` — a **separate** `Drw`
  context from the bar's (`drw` in `dwm.c`), created once in `osdsetup()`
  with its own font set loaded from `fonts[]`/`fontslen` (given external
  linkage in `config.h` for exactly this reason, same pattern as
  `wallpaperdir`/`fifopath`). Deliberately not sharing the bar's `Drw`:
  the bar's pixmap is sized/resized for the bar window, and reusing it
  for a differently-sized popup would mean save/restore dancing around
  `drawbar()`'s own state every time either one draws. A second small
  `Drw` costs one extra font load at startup and avoids that class of bug
  entirely.
- The popup itself is a small override-redirect window created once in
  `osdsetup()` (bottom-center of `selmon`, sized/positioned via the
  `OSD_W`/`OSD_WIN_H`/`OSD_MARGIN_BOTTOM` `#define`s at the top of
  `osd.c` — not `config.h`, since these are layout constants rather than
  something you'd realistically want a different value per monitor/theme
  for) — `XMapRaised()`d on first paint, left mapped for repeat triggers.
- `osdtick()` — called every `run()` iteration — compares
  `CLOCK_MONOTONIC` against the last paint time and `XUnmapWindow()`s the
  popup once `OSD_TIMEOUT_MS` (1200ms) has elapsed. `CLOCK_MONOTONIC`
  rather than `time()` because this needs sub-second precision and
  shouldn't jump if the system clock does.
- `osdcleanup()` (`cleanup()`, `dwm.c`) frees `osddrw` and destroys the
  popup window on exit/restart.

## Media OSD popup (`mediaosd.c` / `mediaosd.h`, custom, not a suckless patch)

Functions: `mediaosdsetup()`, `mediaosdcleanup()`, `mediaosdtrigger()`,
`mediaosdtick()` are the public surface (declared in `mediaosd.h`);
`parsestate()`, `mediaosdpaint()`, `mediaosd_drawart()`,
`mosd_start_fetch()`, `mosd_poll_child()`, `mosd_reap_fetch()`,
`mosd_reap_zombie()`, `mosd_finish_and_paint()` are `static` inside
`mediaosd.c`.

Shows the current track (title/artist/album, play/pause icon, a progress
bar, and album art when available) via the external `mediactl` script,
same `changecmd`/`getcmd`-style argv-array philosophy as the OSD above --
but unlike `osd.c`'s `runargv_wait()`, **nothing here ever blocks the
event loop waiting on a child process**, because a `mediactl status`/`art`
call can be slow in a way volume/brightness controls never are (a cold
`playerctl` D-Bus round-trip, or -- for `art` -- a network fetch for a
remote cover image if the player doesn't cache one locally). An earlier
version of this file did use a blocking `runargv_getline()` here, the same
shape as `osd.c`'s; it was replaced with the state machine below
specifically because that fetch is the slow one.

- **The fetch is a small non-blocking state machine**, not a blocking
  call: `MosdStage` is `MOSD_IDLE` / `MOSD_FETCH_STATUS` /
  `MOSD_FETCH_ART`. `mosd_start_fetch(argv, stage)` forks+execs with the
  child's stdout piped back through a fd set `O_NONBLOCK` via `fcntl()`,
  records the pid/fd/start-time in static state, and returns immediately
  -- it does not wait for the child at all. `mosd_poll_child()`, called
  from `mediaosdtick()` every `run()` iteration whenever `mosdstage !=
MOSD_IDLE`, does exactly one non-blocking `read()` attempt per call:
  `EAGAIN`/`EWOULDBLOCK` means "nothing yet, try again next tick" and
  returns immediately (this is the whole point -- a call here costs a
  fraction of a millisecond even while a multi-hundred-millisecond fetch
  is still running in the background); EOF means the child is done
  writing, so it reaps the child and advances the chain.
- **The chain**: a real trigger (`mediaosdtrigger()`, bound to a media
  key) starts `MOSD_FETCH_STATUS` with `mosdpendingfetchart = 1`. Once
  that completes and parses as a non-idle track, it starts
  `MOSD_FETCH_ART` (`mediactl art`) before finally painting. The
  once-a-second background poll (from `mediaosdtick()`, while the popup
  is already visible, mirroring `MOSD_POLL_MS`) sets
  `mosdpendingfetchart = 0` instead, so it re-checks status/progress
  without re-fetching art every second -- `lastart` (the art path from
  the last real trigger) is reused. `mosdpendingistrigger` tracks which
  kind of chain is in flight so only a real trigger's completion resets
  `mosdshownat`/`mosdpolledat` (the popup's shown/timeout clock) -- a
  background poll landing shouldn't restart the countdown.
- **Timeout safety net**: `MOSD_FETCH_TIMEOUT_MS` (5000) is checked at
  the top of every `mosd_poll_child()` call against
  `mosdfetchstarted`. If a child hasn't produced EOF by then (a genuinely
  stalled network fetch, say), it's `SIGKILL`'d and abandoned rather than
  leaving the state machine stuck indefinitely -- the popup just falls
  back to whatever it last painted.
- **Reaping never blocks either.** `mosd_reap_fetch()` uses
  `waitpid(pid, NULL, WNOHANG)`; on the very rare chance the child's
  stdout hit EOF fractionally before the child itself fully exited, the
  pid is stashed in `mosdzombiepid` and `mosd_reap_zombie()` (called at
  the top of every `mediaosdtick()`, unconditionally) retries the
  non-blocking `waitpid()` on later ticks until it succeeds -- so no
  zombie accumulates and nothing ever waits.
- `mediaosdtick()` drives the fetch machine _before_ checking
  `mosdvisible`/the timeout, since a fresh trigger's chain hasn't painted
  (and therefore hasn't set `mosdvisible`) yet when the first tick after
  `mediaosdtrigger()` runs.
- `mediaosdcleanup()` (`cleanup()`, `dwm.c`) additionally `SIGKILL`s and
  reaps any in-flight or not-yet-reaped child before tearing down the
  window/`Drw`, so restarting/quitting dwm mid-fetch doesn't leak a
  process.
- `parsestate()` splits `mediactl`'s tab-separated 9-field line (see
  `mediactl`'s own `get_state()`) by hand with `strchr(p, '\t')` per
  field rather than `sscanf()`, since field values (titles, artist names)
  can contain arbitrary characters `sscanf("%s")` would mis-split on;
  `progress` is the one numeric field, read with a plain `atoi()`.

## Notifications (`notifications.c` / `notifications.h`, custom, not a suckless patch)

Functions: `notifsetup()`, `notifcleanup()`, `notiftick()`,
`notif_win_click()`, `notif_win_expose()`, `notif_blockclick()`,
`notif_dnd()`, `notif_dismissall()`, `notif_clearhistory()`,
`notif_dumphistory()` are the public surface (declared in
`notifications.h`); the dbus message parsing/handling functions, the
popup pool, and the history ring buffer are all `static` inside
`notifications.c`.

Unlike the other custom modules, this one doesn't call out to an external
tool at all (compare `screenshot.c`'s `notify-send`/`xclip` execs below,
or the OSD's `changecmd`/`getcmd` argv arrays) -- dwm implements the
`org.freedesktop.Notifications` DBus interface itself, using libdbus-1's
low-level API directly (no glib/sd-bus). This is the one place in the
codebase where dwm _becomes_ something rather than launching it, and it's
a deliberate exception to the `spawn()`/screenshot-notify reasoning
documented below: a notification daemon needs to own a well-known DBus
name and hold state (history, DND, in-flight popups) for the life of the
session, which doesn't fit the fork-and-forget model everything else here
uses.

- `notifsetup()` (called once from `setup()`, after `statusbar_init()`)
  connects to the session bus via `dbus_bus_get(DBUS_BUS_SESSION, ...)`
  and calls `dbus_bus_request_name()` for `org.freedesktop.Notifications`
  with `DBUS_NAME_FLAG_DO_NOT_QUEUE`. If that fails -- most likely because
  another daemon (dunst, mako, a desktop environment's own notification
  server) already owns the name -- dwm logs a warning to stderr and
  disables the notification system for that session rather than crashing
  or fighting over the name; everything else in dwm is unaffected. The
  same function also pre-creates a fixed pool of `NOTIF_MAX_POPUPS` (5)
  override-redirect windows, each with its own `Drw` (same reasoning as
  the OSD's separate `Drw` -- see above), sized off the bar's font
  metrics the first time through the loop.
- `notiftick()` -- called every `run()` iteration, same as
  `statusbar_tick()`/`osdtick()` -- does two things: non-blockingly pumps
  the dbus connection (`dbus_connection_read_write_dispatch(conn, 0)`,
  then drains `dbus_connection_pop_message()` in a loop) and walks the
  popup pool checking `CLOCK_MONOTONIC` against each active popup's
  per-urgency expiry.
- Incoming `Notify` calls are parsed by hand via `DBusMessageIter`
  (`handle_notify()`) -- app name, replaces-id, icon (accepted but not
  rendered), summary, body, the `actions` array (only the first action
  key is kept, invoked on left-click as the "default action"), the
  `hints` dict (only `urgency` is read), and `expire_timeout`. A
  malformed call gets a `DBUS_ERROR_INVALID_ARGS` reply rather than being
  silently dropped or crashing the parse. Urgency drives the default
  timeout when the caller doesn't specify one: low = 4s, normal = 6s,
  critical = never auto-expires (the sending app is expected to close it
  itself), matching common notification-daemon convention.
- Popups are laid out top-to-bottom from the top-right corner of `selmon`
  by `notif_relayout()`, called whenever a popup is shown or dismissed --
  slots are reused in place rather than creating/destroying windows on
  the common path. Each popup stacks using its own `height` (see next
  bullet), not a single shared value, so a tall wrapped body correctly
  pushes the next popup further down.
- **Word-wrap.** `wrap_text()` greedily wraps `p->body` at word
  boundaries to fit `NOTIF_W`'s text column, using the same
  `drw_fontset_getwidth()` measurement the rest of the file already
  relies on. `fill_popup()` computes this once per notification (not on
  every repaint -- an `Expose` just redraws the cached
  `p->bodylines[]`/`p->nbodylines`) and resizes that popup's window
  (`XResizeWindow()` + `drw_resize()`) to fit however many lines that
  took, up to `NOTIF_BODY_MAXLINES` (4); text past that is truncated with
  a trailing `...` on the last line rather than silently dropped. The
  history overlay (`notif_hist_paint()`) wraps the same way into a
  smaller, fixed budget (`NOTIF_HIST_BODY_MAXLINES`, 2) since its row
  height isn't per-row dynamic the way a toast's window is. Before this,
  the body was drawn as a single line with the clamp box set to the
  text's own measured width, which meant `drw_text()`'s ellipsis-clamp
  never actually triggered -- long bodies just overflowed past the
  window edge instead of being cut off cleanly; that's fixed as a side
  effect of routing everything through `wrap_text()`'s width-aware
  layout instead.
- Border color reuses the bar's existing `scheme[]`
  entries by urgency (`SchemeUrg` for critical, `SchemeSel` for normal,
  `SchemeHid` for low) rather than introducing a parallel color config.
- Click handling: `dwm.c`'s `buttonpress()`/`expose()` each call
  `notif_win_click()`/`notif_win_expose()` first, before their normal
  window-lookup logic, since popup windows are override-redirect and
  outside dwm's usual client/monitor bookkeeping. Left-click invokes the
  stored default action (if any, via an `ActionInvoked` signal) then
  dismisses; any other button just dismisses. Both close paths, plus
  timeout expiry and an explicit `CloseNotification` call, send a
  `NotificationClosed` signal with the reason code the spec defines (1 =
  expired, 2 = dismissed, 3 = closed via `CloseNotification`).
- A 25-entry ring buffer (`history[]`) records every notification
  received regardless of whether DND suppressed its popup, and
  `notif_dumphistory()` writes it out (newest first) to `fiforeplyfd`,
  following the exact same drain-then-write pattern `ipc.c`'s
  `fifostate()` uses (see "FIFO IPC layer" below) -- so it's subject to
  the same "read it right after you request it" caveat.
- **Statusbar integration.** `config.h`'s `notifblockidx` names which
  `statusblocks[]` entry (an otherwise-empty `{"", "", 0}` row -- there's
  no shell command to run) shows the bell/count. `notifications.c` pushes
  its own text into that slot directly via `statusbar_setblock()`
  whenever the unread count or DND state changes -- no shell fork, same
  technique the OSD's `blockidx` refresh path uses. The other half of
  this wiring lives in `statusbar.c`: `statusbar_handleclick()` special-
  cases `blockidx == notifblockidx` and calls `notif_blockclick()`
  instead of `runblock()`, since re-running an empty shell command would
  just clobber what `notifications.c` had pushed. Left click on that
  block dismisses all visible popups, middle click clears history, right
  click toggles DND -- see WIKI.md → "Notifications" for the config-facing
  version of this.
- `notifcleanup()` (`cleanup()`, `dwm.c`) frees every popup's `Drw`,
  destroys the popup windows, and `dbus_connection_unref()`s the
  connection. It deliberately does **not** call `dbus_connection_close()`
  -- `dbus_bus_get()` returns a connection libdbus itself owns/shares, so
  closing it is undefined; unref is the correct teardown for that API,
  same distinction as `XCloseDisplay()` vs. freeing an individual
  resource.

`autostart.sh` no longer starts `dunst` (or any other notification
daemon) for the same reason it stopped starting `dwmblocks` -- see
"Autostart" in WIKI.md.

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
  before falling back to `handler[ev.type]`. `clipboard.c`'s XFixes
  selection-notify event uses the identical `fixesbase + <event>`
  pattern, checked right next to this one -- see "Clipboard history"
  below.
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
  implementing ICCCM selection ownership itself. Same reasoning as
  `spawn()`: dwm launches things, it doesn't become them. `SIGCHLD` is
  already `SA_NOCLDWAIT` (see `setup()`), so these forked children never
  need to be waited on. `notifications.c` (see "Notifications" above) is
  the one deliberate exception to this pattern elsewhere in the codebase
  -- but note that even `notifyshot()`/`notifycolor()` still just `exec`
  `notify-send`; they don't call into `notifications.c` directly. That
  `notify-send` call is now received by dwm's own DBus server instead of
  an external daemon, but the screenshot code itself is unchanged and
  still doesn't know or care who's listening on the other end of the bus.

## Clipboard history (`clipboard.c` / `clipboard.h`, custom, not a suckless patch)

Functions: `clipboardsetup()`, `clipboardcleanup()`, `clipboardselectionnotify()`,
`clipboardfixesnotify()`, `clippick()`, `clippin()`, `clipclear()`,
`cliptick()` are the public surface (declared in `clipboard.h`, called
from `dwm.c`'s `setup()`/`cleanup()`/`handler[]`/`run()` and from `ipc.c`'s
`fifocmds[]` table); everything else (`clipboardpush()`, `cliptrim()`,
`savehistory()`/`loadhistory()`, `runargv_io()`, `copytextclip()`,
`clipappendline()`, `cliplistmove()`) is `static` inside `clipboard.c`.

Same reasoning as `screenshot.c`'s clipboard writes (see above): dwm
never implements ICCCM selection ownership itself. It only _watches_
`CLIPBOARD` (via the XFixes extension) and, when you pick a history
entry, hands the text to `xclip` over a pipe -- same fork/exec shape as
`screenshot.c`'s `copytextclip()`, just parameterized on length instead
of assuming a NUL-terminated string. This sidesteps having to serve
`SelectionRequest` events, which is most of the complexity a "real"
clipboard manager has to deal with.

- `clipboardsetup()` (called once from `setup()`, and only if
  `XFixesQueryExtension()` succeeded -- see "Monitor hotplug handling"
  above for the identical pattern with RandR) interns the `CLIPBOARD`
  and `UTF8_STRING` atoms, creates a small never-mapped requestor
  window (`clipwin`, same idea as `wmcheckwin`), and calls
  `XFixesSelectSelectionInput()` to subscribe to ownership-change
  notifications on it. It then calls `loadhistory()` to restore
  persisted state, and if the clipboard already has an owner at
  startup, immediately requests its content the same way a live change
  would -- so restarting dwm doesn't lose whatever's currently copied.
- Extension event dispatch mirrors RandR's: `fixesbase` (the XFixes
  event base, queried in `setup()`) is checked in `run()` right next to
  the existing `rrbase` check, since `XFixesSelectionNotify`'s real
  event type (`fixesbase + XFixesSelectionNotify`) is assigned at
  runtime and can fall outside `handler[]`'s indexed range, same
  constraint RandR events have.
- `clipboardfixesnotify()` is what that dispatch calls. It checks the
  event is for `CLIPBOARD` and a real ownership change (not a
  destroy/close), then calls `XConvertSelection()` targeting
  `UTF8_STRING` onto `clipwin`'s property -- this is a _request_, not
  the data itself.
- `clipboardselectionnotify()` **is** wired into `handler[]` directly
  (under `SelectionNotify`, a fixed core-protocol event type, unlike
  the extension event above) -- it's the reply to that
  `XConvertSelection()` call. It reads the property via
  `XGetWindowProperty()` (deleting it as it reads, per ICCCM
  convention) and hands the bytes to `clipboardpush()`.
- `clipboardpush()` dedups against `lastentry` (the single most
  recently captured entry, tracked across both lists -- see below),
  truncates absurdly large clips at `CLIP_MAX_ENTRY` (256KB), and
  prepends a new `ClipEntry` onto `history`. The dedup check is what
  keeps `xclip` re-asserting ownership of something you just picked
  from history (which fires another `XFixesSelectionNotify`) from
  spawning a duplicate at the top of the list.
- **Two singly linked lists**, both newest-first: `history` (unpinned,
  capped at `CLIP_MAX_HISTORY` = 200) and `pinned` (capped at
  `CLIP_MAX_PINNED` = 100). `cliptrim()` evicts the oldest entry (the
  tail -- an O(n) walk, cheap at these caps) whenever either list
  exceeds its cap; pinned entries are never evicted by unpinned growth
  and vice versa.
- `lastentry` -- the single most recently _captured_ entry, whichever
  list it's currently sitting in. It's set to the head of `history` at
  every push, and `clippin()`/`cliplistmove()` relocate it (in place,
  no realloc) between list heads without ever invalidating the
  pointer. This works because of an invariant `cliplistmove()` asserts
  defensively: `lastentry` is _always_ the head of whichever list it's
  in when `clippin()` runs, since only a fresh push (always to
  `history`'s head) or a prior pin/unpin toggle (always to the other
  list's head) can have put it there.
- `clippin()` toggles `lastentry` between `history` and `pinned` via
  `cliplistmove()`. This is a head-to-head move with no predecessor
  search needed, given the invariant above -- `cliplistmove()` still
  checks `*from == e` and no-ops rather than corrupt a list if that's
  ever violated.
- `clipclear()` frees every node in `history` (not `pinned`), NULLing
  out `lastentry` first if it pointed into the list being freed.
- **Persistence** (`savehistory()`/`loadhistory()`) uses a
  deliberately dumb length-prefixed format rather than any escaping
  scheme: each record is `"<P|H> <unix-ts> <byte-len>\n"` followed by
  exactly that many raw bytes and a separator newline. This lets an
  entry contain anything -- embedded newlines, NULs, whatever the X
  selection handed over -- with no encoding step, at the cost of the
  file not being line-oriented in the usual sense. Written to (and read
  from) `$XDG_CACHE_HOME/dwm/clipboard_history`, falling back to
  `~/.cache/dwm/` the same way `screenshot.c`'s path helper falls back
  for `~/Pictures/Screenshots/`.
- **Debounced saves.** `savehistory()` rewrites the _entire_ file from
  scratch every time (it's the length-prefixed format above written out
  in full, not an append) -- fine for the low-frequency, user-initiated
  callers (`clippin()`, `clipclear()`, `clipboardcleanup()`, all of which
  still call it directly and immediately), but `clipboardpush()` fires on
  every clipboard change, and a burst of rapid copies would otherwise
  mean a full-history disk rewrite per copy. `clipboardpush()` instead
  just sets `clipdirty = 1` and stamps `clipdirtysince`; `cliptick()`
  (called from `run()`, same as `statusbar_tick()` etc. -- see "The event
  loop" above) flushes the real `savehistory()` once
  `CLIP_SAVE_DEBOUNCE_SEC` (2) seconds have passed with no further
  change. `clipboardcleanup()` always flushes unconditionally on
  shutdown regardless of `clipdirty`, so a normal quit/restart never
  loses anything -- the debounce window only matters for an _unclean_
  shutdown (crash, `kill -9`) landing within those ~2 seconds, in which
  case whatever was copied most recently within that window can be lost.
  `clippin()`/`clipclear()` deliberately stayed as immediate,
  non-debounced writes: both are rare enough that batching them buys
  nothing, and `clipclear()` specifically should persist right away --
  debouncing a "clear my history" action would mean the cleared (and
  possibly sensitive) data could still be sitting on disk if the machine
  crashed inside the debounce window.
- `clippick()` builds a `dmenu`-formatted menu string (pinned entries
  first, then history, each line `"<index> <pin-marker><preview>"`)
  alongside a parallel `ClipEntry*` index array in the same emission
  order, then calls `runargv_io()` -- a local bidirectional-pipe helper
  (own copy, same fork/pipe shape as `osd.c`'s `runargv_wait()`/
  `runargv_getint()`) that writes the menu to `dmenu`'s stdin, closes
  it (so `dmenu` sees EOF and knows the list is complete), then reads
  one line back from its stdout. The chosen line's leading integer is
  parsed with `sscanf()` and used to index back into the array --
  recovering the _full_, untruncated entry even though the preview
  shown in `dmenu` was truncated to `CLIP_PREVIEW_LEN` (100) bytes.
  Preview truncation is byte-based, not UTF-8-aware, so a multi-byte
  character can render oddly at the tail of a long line -- cosmetic
  only, since the index-based lookup means the truncation never
  touches what actually gets copied.
- `clipboardcleanup()` (`cleanup()`, `dwm.c`) saves history one last
  time, frees both lists, and destroys `clipwin`.

`autostart.sh` no longer starts an external clipboard daemon, for the
same reason it stopped starting `dunst` -- see "Autostart" in `WIKI.md`.

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
   declaration in `dwm.c` too). Functions in
   `wallpaper.h`/`screenshot.h`/`statusbar.h`/`osd.h`/`notifications.h`/
   `clipboard.h` are already visible to `ipc.c`.
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
