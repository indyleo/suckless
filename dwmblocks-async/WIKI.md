# WIKI — dwmblocks Configuration Reference

All config lives in `config.h`. Rebuild with `make clean install` after any
change.

## Delimiter

```c
#define DELIMITER " || "
```

String placed between each block's output in the bar. Set to `""` to remove
separators entirely.

```c
#define LEADING_DELIMITER  0  /* 1 = prepend delimiter before first block */
#define TRAILING_DELIMITER 0  /* 1 = append delimiter after last block */
```

## Block output limit

```c
#define MAX_BLOCK_OUTPUT_LENGTH 45
```

Maximum Unicode characters a single block can output. Output beyond this is
truncated. Increase if a block's content gets cut off.

## Clickable blocks

```c
#define CLICKABLE_BLOCKS 1
```

Enabled. Requires dwm's `statuscmd` patch to also be active (it is, in this
build). When you click a block in the bar, dwm walks the raw status string
looking for the control byte dwmblocks embedded right before that block's
output — that byte is the block's own signal number — then sends
`SIGRTMIN + <that signal>` to dwmblocks via `sigqueue()`, with the button
number (1=left, 2=middle, 3=right, 4=scroll-up, 5=scroll-down) as the
signal's payload.

dwmblocks receives that signal, matches it to the block, and re-runs that
block's _exact configured command_ with `BLOCK_BUTTON=<button>` set in its
environment before doing so (only when button != 0). The block script reads
`$BLOCK_BUTTON` itself to decide what to do — dwmblocks has no per-block
click logic of its own, it just forwards the button number.

Note: if a block's command is still running when another click comes in,
the click is dropped rather than queued (`block_execute()` no-ops if
`block->fork_pid != -1`) — a rapid double-click on a slow-starting action
(e.g. launching a GUI mixer) can silently miss the second click.

Blocks with `signal = 0` in `BLOCKS()` below can never be clicked — dwm's
click handler bails out early whenever the resolved signal is `0`, so no
signal is ever sent for those blocks regardless of this setting.

## Block definitions

```c
#define BLOCKS(X)                           \
    X("", "mediactl state-title 35", 0, 23) \
    X("", "sysstats volume",         0, 24) \
    X("", "sysstats brightness",     0, 25) \
    X("", "sysstats battery",       15,  0) \
    X("", "sysstats date_time",     30,  0)
```

Columns: `icon`, `command`, `interval (seconds)`, `update signal`.

| Icon | Command                   | Updates                                 | Clickable     |
| ---- | ------------------------- | --------------------------------------- | ------------- |
| ``   | `mediactl state-title 35` | Signal 23 (`pkill -RTMIN+23 dwmblocks`) | Yes           |
| ``   | `sysstats volume`         | Signal 24                               | Yes           |
| ``   | `sysstats brightness`     | Signal 25                               | Yes           |
| ``   | `sysstats battery`        | Every 15s                               | No (signal 0) |
| ``   | `sysstats date_time`      | Every 30s                               | No (signal 0) |

### Current click behavior per block

Defined inside `mediactl` and `sysstats` themselves (via `$BLOCK_BUTTON`),
not in this repo — documented here for a quick reference since it lives
across two other repos:

| Block      | Left                        | Right                                                            | Scroll up | Scroll down |
| ---------- | --------------------------- | ---------------------------------------------------------------- | --------- | ----------- |
| media      | play/pause                  | —                                                                | previous  | next        |
| volume     | mute toggle                 | open `wiremix` mixer scratchpad (via dwm FIFO `togglescratch 3`) | +5%       | -5%         |
| brightness | dim/undim toggle (10%↔100%) | —                                                                | +5%       | -5%         |

## Manually triggering a block update

```sh
pkill -RTMIN+24 dwmblocks   # refresh volume block
pkill -RTMIN+23 dwmblocks   # refresh media/song block
```

Your `sysctl` and `mediactl` scripts should do this automatically after
changing state. The pattern is:

```sh
# In sysctl vol -i 5 (or similar):
pactl set-sink-volume @DEFAULT_SINK@ +5%
pkill -RTMIN+24 dwmblocks
```

Note this plain `pkill` form sends button `0` (no payload), so
`BLOCK_BUTTON` is never set for a signal sent this way — it's
indistinguishable from a periodic timer refresh to the block script. Only
a real click (routed through dwm) carries a button number.

## Block commands

Block commands are plain shell commands whose stdout becomes the block's
text. The icon string is prepended by dwmblocks before the command output.
Commands should be fast — anything slow (>100ms) will create visible lag
since the output won't arrive until the subprocess completes. For slow
data sources (network, battery), use a longer interval rather than polling
frequently. This applies doubly to click-triggered runs: a slow action
(e.g. `setsid -f pavucontrol`) blocks the _next_ click on that same block
until it finishes, per the no-queueing behavior noted above.
