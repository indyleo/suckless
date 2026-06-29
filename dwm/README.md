# dwm (custom build)

A personal fork of [dwm](https://dwm.suckless.org/) — the suckless dynamic
window manager — with a curated patch set, a custom Imlib2 wallpaper engine,
and a hand-rolled FIFO IPC layer. Targets real laptop/desktop hardware
(not just X11-in-a-VM).

## Why this fork exists

Stock dwm is intentionally minimal. This build adds the quality-of-life
patches that come up in nearly every dwm setup (scratchpads, swallowing,
gaps, per-tag layouts) plus a few custom features that aren't part of the
suckless patch ecosystem at all — most notably wallpaper management and
external IPC control.

## Feature summary

| Category | Feature | Source |
|---|---|---|
| Layout | Per-tag layout/mfact/nmaster memory | `pertag` patch |
| Layout | Gaps between windows | `uselessgap` patch |
| Layout | Move windows within the stack | `movestack` patch |
| Layout | Attach new clients below the active one | `attachbelow` patch |
| Windows | Terminal scratchpads (toggle-able floating apps) | `scratchpads` patch |
| Windows | Terminal swallows GUI child windows | `swallow` patch |
| Bar | 2D-drawn status bar (icons/colors in status text) | `status2d` patch |
| Bar | Clickable status segments | `statuscmd` patch |
| Wallpaper | Imlib2-based wallpaper renderer, per-monitor | custom |
| Wallpaper | Random wallpaper rotation on a timer | custom |
| Wallpaper | Manual "next wallpaper" keybind | custom |
| IPC | FIFO-based remote control (`/tmp/dwm.fifo`) | custom |
| Session | Custom autostart process management | `autostart.sh` |

See **[DOCS.md](DOCS.md)** for how the code is organized and **[WIKI.md](WIKI.md)**
for how to configure it.

## Requirements

- Xlib headers (`libx11-dev` or equivalent)
- Xinerama (multi-monitor support)
- Xft + fontconfig (font rendering)
- Imlib2 (wallpaper rendering)
- libxcb + xcb-res (used for process/PID lookups, e.g. swallow)

On Debian/Ubuntu-style systems:

```sh
sudo apt install libx11-dev libxinerama-dev libxft-dev libimlib2-dev \
                  libxcb1-dev libxcb-res0-dev
```

## Building

```sh
make clean install
```

This builds `dwm` and installs it (along with `autostart.sh`, the man page,
a `.desktop` entry, and an icon) into the prefix set in `config.mk`
(`/usr/local` by default). Root privileges are typically needed for `install`.

To rebuild after editing `config.h` (the only file you should normally
touch), just run `make clean install` again — `config.h` is compiled
directly into the `dwm` binary, there's no runtime config file.

## Running

Add to your `.xinitrc`:

```sh
exec dwm
```

Or select "dwm" from your display manager's session list (the installed
`.desktop` file enables this).

## Quick start after install

```sh
# Confirm it's running and the FIFO exists
ls -l /tmp/dwm.fifo

# Try the IPC layer
echo "view 1" > /tmp/dwm.fifo
echo "nextwallpaper" > /tmp/dwm.fifo
```

## License

MIT/X11, same as upstream dwm — see `LICENSE`.
