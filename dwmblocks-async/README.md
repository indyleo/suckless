# dwmblocks (custom build)

A personal fork of [dwmblocks](https://github.com/torrinfail/dwmblocks) — a
modular status bar for dwm that runs independent block scripts on configurable
intervals and signals.

This build uses the **async** dwmblocks variant: each block runs in its own
subprocess, so a slow block never stalls the others. Blocks are defined via
the `BLOCKS()` X-macro in `config.h` and write to `WM_NAME` / `_NET_WM_NAME`
on the root window, which dwm reads for its bar.

## Blocks (current config)

| Icon | Command | Interval | Signal |
|---|---|---|---|
| `` | `mediactl state-title 35` | on signal only | 23 |
| `` | `sysstats volume` | on signal only | 24 |
| `` | `sysstats brightness` | on signal only | 25 |
| `` | `sysstats battery` | 15s | — |
| `` | `sysstats date_time` | 30s | — |

Blocks are separated by ` \|\| ` in the bar. No leading or trailing delimiter.

## Updating blocks

Signal-driven blocks (interval = 0) only update when explicitly triggered:

```sh
# Update volume block (signal 24)
kill -44 $(pidof dwmblocks)   # SIGRTMIN+24 = 34+24 = 58... use pkill instead:
pkill -RTMIN+24 dwmblocks

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
