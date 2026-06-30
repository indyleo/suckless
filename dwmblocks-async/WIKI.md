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
#define CLICKABLE_BLOCKS 0
```

Set to `1` to enable click handling (requires dwm's `statuscmd` patch to
also be active). When enabled, clicking a block in the bar sends that block's
signal with a button-number prefix to the command's environment.

Currently disabled — status interactions use the dwm FIFO instead.

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

| Icon | Command | Updates |
|---|---|---|
| `` | `mediactl state-title 35` | Signal 23 (`pkill -RTMIN+23 dwmblocks`) |
| `` | `sysstats volume` | Signal 24 |
| `` | `sysstats brightness` | Signal 25 |
| `` | `sysstats battery` | Every 15s |
| `` | `sysstats date_time` | Every 30s |

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

## Block commands

Block commands are plain shell commands whose stdout becomes the block's
text. The icon string is prepended by dwmblocks before the command output.
Commands should be fast — anything slow (>100ms) will create visible lag
since the output won't arrive until the subprocess completes. For slow
data sources (network, battery), use a longer interval rather than polling
frequently.
