# dwm

A build of [dwm](https://dwm.suckless.org/), the suckless dynamic window
manager, extended with a curated set of upstream patches plus several
custom, from-scratch additions — a built-in async status bar, a
notification daemon, an on-screen-display popup, native clipboard
history, and more — aimed at removing the usual pile of external daemons
(`dwmblocks`, `dunst`/`mako`, a clip manager) a typical dwm setup relies
on, without giving up dwm's single-binary, no-config-language philosophy.

See **[WIKI.md](WIKI.md)** for the full configuration reference (every
`config.h` array/knob, keybind, and FIFO command), and
**[DOCS.md](DOCS.md)** for internals — how the pieces fit together, why
things are structured the way they are, and what to know before editing
any of the custom modules below.

## Patches

- **`pertag`** — per-tag layout, `mfact`, and master-count memory.
- **`uselessgap`** — configurable gaps between windows.
- **`movestack`** — reorder windows within the stack without a mouse.
- **`attachbelow`** — new clients attach below the focused one instead of
  always becoming master.
- **`scratchpads`** — toggle-able floating scratchpad terminals.
- **`swallow`** — a terminal swallows the GUI window it launches (e.g.
  opening an image viewer from a shell) and reappears when it closes.
- **`status2d`** — 2D-drawn status bar text (inline icon/color codes).
- **`statuscmd`** — clickable status bar segments.

## Custom additions

None of these are suckless.org patches — they're modules written for
this build, documented in depth in `DOCS.md`.

- **Built-in status bar** (`statusbar.c`/`.h`) — per-block refresh
  interval and click handling, all in-process. No external `dwmblocks`
  binary or autostart entry needed; see [dwmblocks-async](../dwmblocks-async/)
  if you'd rather run a separate status bar process instead.
- **On-screen-display popup** (`osd.c`/`.h`) — volume, brightness,
  microphone, and keyboard-backlight controls each flash a label and
  percentage bar. Reading the current level back is a zero-fork sysfs
  read or a single direct `wpctl` call for the controls that support it
  (`fastget` in `osd.h`), not a subprocess spawned through a wrapper
  script.
- **Media OSD popup** (`mediaosd.c`/`.h`) — a second popup for the
  current track (title/artist/art/progress) via an external `mediactl`
  script, driven by a non-blocking fetch state machine so a slow
  `playerctl` call or an album-art network fetch never freezes the rest
  of dwm.
- **Notification daemon** (`notifications.c`/`.h`) — a complete
  `org.freedesktop.Notifications` DBus server: popups, word-wrapped
  bodies, a history overlay, Do Not Disturb, and a bar indicator. No
  dunst/mako or anything else needed alongside it.
- **Native clipboard history** (`clipboard.c`/`.h`) — XFixes-watched,
  pinning, a `dmenu` picker, debounced disk persistence. No external clip
  daemon.
- **Async wallpaper engine** (`wallpaper.c`/`.h`) — Imlib2 loading off the
  main thread so a large image never blocks the event loop, automatic
  timed rotation, and a manual "next wallpaper" keybind.
- **Screenshot tool** (`screenshot.c`/`.h`) — region select, a color
  picker, clipboard + notification integration, no external
  scrot/maim/slop chain.
- **FIFO remote control** (`ipc.c`/`.h`) — script or shell into
  `/tmp/dwm.fifo` for tag/layout/wallpaper/clipboard/notification/OSD
  commands without a keybind.
- **RandR monitor hotplug** — monitors are detected and laid out
  automatically on connect/disconnect, no manual `xrandr` + restart.
- **Central `theme.h` palette** — every color used anywhere in the build
  is a named constant in one file, shared naming with the companion `qs`
  Quickshell config if you run both.
- **Custom autostart management** (`autostart.sh`) — starts/supervises a
  small process list once per session (see `autostart.sh` itself for the
  current list) instead of a pile of `exec-once` lines.

## Building & installing

```sh
cd dwm
make clean install
```

Installs to `/usr/local/bin` by default (`PREFIX` in `config.mk`; needs
root for `install`). Dependencies are listed in the top-level repo
README.

## Configuring

Everything user-facing lives in `config.h` (copy from `config.def.h` on a
fresh checkout if it doesn't exist yet) — see **[WIKI.md](WIKI.md)** for
what every array and knob does. Colors specifically live in `theme.h`.
After any change:

```sh
make clean install
```

Several of the custom modules also have their own tuning `#define`s at
the top of their `.c` file rather than in `config.h` (popup dimensions,
timeouts, etc.) — `WIKI.md` notes which ones and why, next to the
relevant feature.

## External scripts this build expects

A few status bar blocks and OSD controls shell out to small scripts kept
outside this repo (so the build itself has no bash dependency baked in
beyond what these can be swapped for):

| Script     | Used by                                                                    |
| ---------- | -------------------------------------------------------------------------- |
| `sysctl`   | OSD volume/brightness/mic/keyboard-backlight changes                       |
| `sysstats` | Status bar blocks (battery, network, kbd, etc.) and their click handling   |
| `mediactl` | The media OSD and its `mediactl state-title`/`title`/`state` status blocks |

None of these need to be named exactly that or written in bash — swap in
your own as long as they accept the same argv shape `config.h` invokes
them with (see `WIKI.md`'s "Status bar blocks" and "On-screen display"
sections for exactly what each call looks like).
