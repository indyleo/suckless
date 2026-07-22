# DOCS — dwmblocks Code Layout & Internals

## File overview

| File                   | Purpose                                                                |
| ---------------------- | ---------------------------------------------------------------------- |
| `dwmblocks.c`          | Main binary: block scheduling, subprocess management, root name update |
| `src/block.c`          | Per-block state, execution (`block_execute`), output capture           |
| `src/signal-handler.c` | Realtime signal registration/dispatch (`SIGRTMIN + signal`)            |
| `src/status.c`         | Assembles all block outputs + delimiters into the root window name     |
| `config.h`             | Block definitions and delimiter settings — compiled into binary        |
| `config.def.h`         | Upstream default config — do not edit                                  |
| `Makefile`             | Build and install                                                      |

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

**Clickable blocks:** enabled in this build (`CLICKABLE_BLOCKS 1`, and
dwm's `statuscmd` patch is active). The full path, confirmed against both
codebases:

1. `status_update()` (`src/status.c`) prepends the raw byte value of
   `block->signal` right before each clickable block's icon+output when
   assembling the combined status string — this is what makes a given
   stretch of the bar text identifiable as "belonging" to a given block.
2. dwm's `buttonpress()` walks that string on click, extracts the byte
   under the cursor into `statussig`, and calls `sigqueue(dwmblocks_pid,
SIGRTMIN + statussig, {sival_int = button})` via `sigstatusbar()`.
3. `signal_handler_process()` (`src/signal-handler.c`) matches
   `signal - SIGRTMIN` back to a block and pulls the button number out of
   `info.ssi_int`.
4. `block_execute()` (`src/block.c`) sets `BLOCK_BUTTON=<button>` in the
   child's environment (only when `button != 0`) and runs the block's exact
   configured command via `popen()`, capturing only the **first line** of
   stdout (up to `MAX_BLOCK_OUTPUT_LENGTH` characters) as the new block text.

dwmblocks itself has no concept of what a click "does" — it's purely a
pipe: button number in, re-run the command, capture one line of output.
All actual click behavior (mute toggle, scroll to adjust, etc.) lives in
the block scripts themselves (`sysctl`, `sysstats`, `mediactl`, in the
scripts repo), gated on `$BLOCK_BUTTON`.

**No click queueing:** `block_execute()` returns immediately without
re-running the command if that block's previous invocation is still
in-flight (checks `block->fork_pid != -1`). A second click while, say, a
GUI mixer is still spawning from the first click is silently dropped, not
queued.

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
- `signal = 0` → no signal-triggered update, interval only, **and not
  clickable** — dwm's click handler ignores any block segment whose
  embedded signal byte is `0`, regardless of `CLICKABLE_BLOCKS`.

## Adding a new block

1. Add an `X(icon, cmd, interval, signal)` row to `BLOCKS()` in `config.h`.
2. Pick a signal number not already in use (check existing blocks). Signal
   numbers are offsets from `SIGRTMIN` — don't exceed 31 or collide with
   existing entries. Use a nonzero signal if you want it clickable.
3. In whatever script changes that state, add `pkill -RTMIN+N dwmblocks`
   to trigger an immediate refresh.
4. If the block should react to clicks, have the command check
   `$BLOCK_BUTTON` itself (1=left, 2=middle, 3=right, 4=scroll-up,
   5=scroll-down) and act accordingly before printing its normal output.
5. Rebuild: `make clean install`, then restart dwmblocks.
