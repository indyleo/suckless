# dwmblocks (custom build)

A personal fork of [dwmblocks](https://github.com/torrinfail/dwmblocks) — a
modular status bar for dwm that runs independent block scripts on configurable
intervals and signals.

This build uses the **async** dwmblocks variant: each block runs in its own
subprocess, so a slow block never stalls the others. Blocks are defined via
the `BLOCKS()` X-macro in `config.h` and write to `WM_NAME` / `_NET_WM_NAME`
on the root window, which dwm reads for its bar.

## Blocks (current config)

| Icon | Command                   | Interval       | Signal | Clickable |
| ---- | ------------------------- | -------------- | ------ | --------- |
| ``   | `mediactl state-title 35` | on signal only | 23     | Yes       |
| ``   | `sysstats volume`         | on signal only | 24     | Yes       |
| ``   | `sysstats brightness`     | on signal only | 25     | Yes       |
| ``   | `sysstats battery`        | 15s            | —      | No        |
| ``   | `sysstats date_time`      | 30s            | —      | No        |

Blocks are separated by `\|\|` in the bar. No leading or trailing delimiter.

## Clickable blocks

`CLICKABLE_BLOCKS 1` — clicking or scrolling a block runs its command again
with `BLOCK_BUTTON` set (1=left, 2=middle, 3=right, 4=scroll-up,
5=scroll-down); the script decides what that means. Current mapping (lives
in the block scripts themselves, see `sysctl`/`sysstats`/`mediactl`):

| Block      | Left             | Right                           | Scroll      |
| ---------- | ---------------- | ------------------------------- | ----------- |
| media      | play/pause       | —                               | prev / next |
| volume     | mute toggle      | open `wiremix` mixer scratchpad | ±5%         |
| brightness | dim/undim toggle | —                               | ±5%         |

Battery and date/time aren't clickable — they're on signal `0`, which dwm's
click handler ignores unconditionally. See [WIKI.md](WIKI.md) for the full
mechanism.

## Updating blocks

Signal-driven blocks (interval = 0) only update when explicitly triggered:

```sh
pkill -RTMIN+24 dwmblocks   # refresh volume block

# Or from dwm's statuscmd patch — clicking a block sends its signal automatically
```

Your `sysctl` and `mediactl` scripts should call `pkill -RTMIN+N dwmblocks`
after changing state to trigger an immediate update.

## Building

```sh
make clean install
```

## Requirements

No extra libraries — just Xlib (`libx11-dev`).

## License

GPLv3 — see `LICENSE`.
