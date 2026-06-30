# DOCS — dwmblocks Code Layout & Internals

## File overview

| File | Purpose |
|---|---|
| `dwmblocks.c` | Main binary: block scheduling, subprocess management, root name update |
| `config.h` | Block definitions and delimiter settings — compiled into binary |
| `config.def.h` | Upstream default config — do not edit |
| `Makefile` | Build and install |

## How it works

dwmblocks runs as a persistent background process. On startup it forks a
subprocess for each block that has a non-zero interval, and maintains a
per-block timer. When a timer fires, the block's command is re-run. The
output of all blocks is concatenated with `DELIMITER` between them and
written to the root window's `WM_NAME` / `_NET_WM_NAME` property, which
dwm reads to render the status bar.

**Async model:** each block command runs in its own forked subprocess. The
main loop collects results via `waitpid()` (non-blocking) rather than
waiting synchronously, so a block that hangs (e.g. a network call) doesn't
freeze the others.

**Signal-driven updates:** dwmblocks registers handlers for `SIGRTMIN` +
each block's signal number. When a signal arrives, that block's command is
immediately re-run regardless of its interval timer. This is how `sysctl`
and `mediactl` trigger instant bar updates after changing volume/brightness.

**Clickable blocks:** disabled in this build (`CLICKABLE_BLOCKS 0`). When
enabled, dwm's `statuscmd` patch sends button click info to dwmblocks via
`SIGRTMIN`, which prepends a click-type byte to the block command's
environment — used for per-block mouse handling. Disabled here since status
interactions use the FIFO IPC instead.

## Block definition macro

```c
#define BLOCKS(X)                           \
    X("", "mediactl state-title 35", 0, 23) \
    X("", "sysstats volume",         0, 24) \
    X("", "sysstats brightness",     0, 25) \
    X("", "sysstats battery",       15,  0) \
    X("", "sysstats date_time",     30,  0)
```

Arguments to `X()`: `icon`, `command`, `interval_seconds`, `signal_number`.

- `interval = 0` → only update on signal (or startup)
- `signal = 0` → no signal-triggered update, interval only

## Adding a new block

1. Add an `X(icon, cmd, interval, signal)` row to `BLOCKS()` in `config.h`.
2. Pick a signal number not already in use (check existing blocks). Signal
   numbers are offsets from `SIGRTMIN` — don't exceed 31 or collide with
   existing entries.
3. In whatever script changes that state, add `pkill -RTMIN+N dwmblocks`
   to trigger an immediate refresh.
4. Rebuild: `make clean install`, then restart dwmblocks.
