# Suckless Tools

A personal collection of patched and customized [suckless.org](https://suckless.org/) tools, tailored for a polished, productive desktop experience.

## Overview

This repository contains customized builds of essential suckless tools, each enhanced with carefully selected patches and custom features that extend their functionality while maintaining the suckless philosophy of simplicity, clarity, and frugality.

## Tools Included

### [dmenu](dmenu/)

A dynamic menu launcher for X, enhanced with:

- 🔍 **Fuzzy matching** with toggle (`-F` to disable)
- 🎨 **Per-scheme highlight colors** for better visual feedback
- 🌓 **Background transparency** (per-scheme alpha values)
- 🔖 **Prefix matching mode** (toggleable with `-x`)
- 📏 **Custom bar height padding** (`user_bh`)
- 📌 **Preselected item on open** (`-n` argument)
- 💬 **Custom prompt text** (`-p` argument)

[dmenu/README.md](dmenu/README.md)

### [dwm](dwm/)

A dynamic window manager for X, enhanced with:

- 🏷️ **Per-tag layout/mfact/nmaster memory** (`pertag` patch)
- ⚪ **Gaps between windows** (`uselessgap` patch)
- ⬆️⬇️ **Move windows within the stack** (`movestack` patch)
- 📥 **Attach new clients below the active one** (`attachbelow` patch)
- 🖥️ **Terminal scratchpads** (toggle-able floating apps) (`scratchpads` patch)
- 🤖 **Terminal swallows GUI child windows** (`swallow` patch)
- 🔍 **Window rules match via PCRE2 regex** (anchors, lookaround, etc.), with **size/move/center/forcefullscreen** overrides
- 📊 **2D-drawn status bar** (icons/colors in status text) (`status2d` patch)
- 🖱️ **Clickable status segments** (`statuscmd` patch)
- 🧱 **Built-in status bar blocks** — per-block interval/click, no external binary required
- 🔊 **On-screen-display popup** for volume/brightness/mic/keyboard-backlight, with zero-fork reads for the level bar where possible
- 🎵 **Media OSD popup** for the current track (title/artist/art/progress), fully non-blocking so a slow player query or album-art fetch never freezes the WM
- 🔔 **Standalone notification daemon** (`org.freedesktop.Notifications` over DBus, popups + word-wrapped bodies + history + Do Not Disturb — no dunst/mako required)
- 🖱️ **Click-for-details status blocks** — click Wi-Fi/Ethernet/Tailscale/battery for a notification with connection/ping/IP/health info beyond what fits in the bar
- 📋 **Native clipboard history** (XFixes-watched, pinning, dmenu picker, debounced disk persistence — no external clip daemon required)
- 🎨 **Central `theme.h` palette** (one file to re-theme every color, shared naming with the companion `qs` Quickshell config)
- 🖼️ **Async Imlib2 wallpaper loader** (no event loop blocking)
- 🔄 **Automatic wallpaper rotation** on a timer
- 🖼️ **Manual "next wallpaper" keybind**
- 📡 **FIFO-based remote control** (`/tmp/dwm.fifo`)
- 🖥️🖥️ **Automatic monitor hotplug detection** via RandR
- 🚀 **Custom autostart process management** (`autostart.sh`)
- 🖥️ **Inbuilt screenshot tool** (region select, color picker, clipboard + notifications)

[dwm/README.md](dwm/README.md)

### [dwmblocks-async](dwmblocks-async/)

An asynchronous status bar for dwm, featuring:

- ⚡ **Fully asynchronous execution** - no more freezing blocks
- 🧩 **Modular design** - each block updates at its own interval
- 🖱️ **Clickable blocks** - interact with your status bar
- 🔁 **Externally triggerable updates** via signals
- 🔄 **Compatible with i3blocks scripts**
- ⚡ Optimized to eliminate status bar flickering

> **Note:** the `dwm` build above no longer depends on this — its status
> bar blocks and click handling are now built directly into `dwm` itself
> (see `dwm`'s feature list above). `dwmblocks-async` is kept here as a
> standalone tool for anyone using it with a different dwm build, or who
> prefers a separate status bar process.

[dwmblocks-async/README.md](dwmblocks-async/README.md)

### [slock](slock/)

A simple X display locker, enhanced with:

- 🌫️ **Background blur** (Gaussian, configurable radius)
- 🔳 **Pixelation mode** (alternative to blur)
- 🎨 **Custom drawn logo** (rectangle grid, right-center aligned)
- 🎨 **Per-state color scheme** (idle/input/failed states with Gruvbox colors)
- 🔒 **Privilege drop** to `nobody:nobody` after locking
- 🧹 **`failonclear`** - cleared input counts as failed attempt

[slock/README.md](slock/README.md)

### [st](st/)

A simple terminal for X, enhanced with:

- 🖼️ **Sixel graphics** (inline images in terminal)
- 🌓 **Background transparency** (`alpha` patch)
- ⌨️ **Keyboard scrollback** (`Shift+PgUp/PgDn`)
- ⚡ **Sync patch** (reduces tearing/flickering)
- 🎨 **Gruvbox 16-color palette** (custom colors)
- 🔤 **CaskaydiaCove Nerd Font, size 12** (custom font)
- ⚙️ **Background opacity 0.85** (custom alpha)
- 🐚 **Default shell: `/bin/zsh`**
- ⚖️ **Bold is bright colors disabled**
- 🌍 **Wide glyph support** (built-in)

[st/README.md](st/README.md)

## Philosophy

This collection maintains the suckless ethos:

- **Simplicity**: Clean, understandable code
- **Clarity**: Obvious behavior and configuration
- **Frugality**: Minimal resource usage
- **Customization**: Patched and configured for real-world usability

Each tool is built from the upstream suckless version with a curated set of patches that solve common pain points, plus a few custom enhancements where the patch ecosystem falls short.

## Requirements

All tools require X11 development libraries. Specific dependencies vary by tool - see individual README files for details.

Typical dependencies on Debian/Ubuntu:

```bash
sudo apt install libx11-dev libxinerama-dev libxft-dev libimlib2-dev \
                 libxcb1-dev libxcb-res0-dev libxext-dev libfontconfig-dev \
                 libharfbuzz-dev libpcre2-dev libdbus-1-dev libxfixes-dev
```

## Building & Installation

Each tool follows the standard suckless build process:

```bash
cd <tool-directory>
make clean install
```

Most tools install to `/usr/local/bin` by default (requires root for installation). Modify `config.mk` in each directory to change the installation prefix.

## Configuration

Configuration is done by editing `config.h` in each tool's directory (except where noted). After modifying `config.h`, rebuild and reinstall:

```bash
make clean install
```

For `st`, don't forget to install the terminfo entry after installation:

```bash
tic -sx st.info
```

## Customization Philosophy

Each fork includes:

1. **Essential patches** from the suckless patch ecosystem that address common usability gaps
2. **Thoughtful custom features** where existing patches don't quite meet needs
3. **Consistent theming** (Gruvbox color scheme where applicable)
4. **Practical defaults** for desktop/laptop usage

## License

All tools retain their original MIT/X11 licenses from suckless.org. See individual `LICENSE` files for details.

---

_Customized and maintained by indyleo. Inspired by the suckless community's commitment to software that stays out of your way._
