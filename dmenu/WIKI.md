# WIKI — dmenu Configuration Reference

All config lives in `config.h`. Rebuild with `make clean install` after any
change.

## Appearance

```c
static int topbar = 1;               /* 0 = appear at bottom of screen */
static const int user_bh = 4;        /* extra pixels added to bar height */
static const char *fonts[] = {"FiraCode Nerd Font:pixelsize=12"};
static const char *prompt = "What to Run > "; /* shown left of input; -p overrides */
```

## Transparency

```c
static const unsigned int alpha = 0xff; /* global alpha fallback; 0xff = opaque */
```

Per-scheme alpha is set in `alphas[]`. Foreground (`[0]`) is always `OPAQUE`;
background (`[1]`) controls the blend:

```c
static const unsigned int alphas[SchemeLast][2] = {
    [SchemeNorm]          = {OPAQUE, 0xD9},  /* ~85% opaque background */
    [SchemeSel]           = {OPAQUE, 0xD9},
    [SchemeSelHighlight]  = {OPAQUE, 0xD9},
    [SchemeNormHighlight] = {OPAQUE, 0xD9},
    [SchemeOut]           = {OPAQUE, 0xD9},
    [SchemePrompt]        = {OPAQUE, 0xD9},
};
```

Values: `0xff` = fully opaque, `0x00` = fully transparent. Requires picom.

## Colors (Gruvbox)

| Scheme                | Foreground | Background | Use                               |
| --------------------- | ---------- | ---------- | --------------------------------- |
| `SchemeNorm`          | `#ebdbb2`  | `#282828`  | Unselected items                  |
| `SchemeSel`           | `#282828`  | `#fabd2f`  | Selected item                     |
| `SchemeSelHighlight`  | `#fe8019`  | `#fabd2f`  | Matched chars in selected item    |
| `SchemeNormHighlight` | `#fe8019`  | `#282828`  | Matched chars in unselected items |
| `SchemeOut`           | `#282828`  | `#8ec07c`  | Multi-select output items         |
| `SchemePrompt`        | `#928374`  | `#282828`  | Prompt text                       |

## Matching

```c
static int fuzzy = 1;       /* 1 = fuzzy matching on by default; -F disables */
static int use_prefix = 1;  /* 1 = require prefix match; -x inverts. Only
                                applies once fuzzy is off (-F) - see below */
```

`fuzzy` is checked first in `match()`, and when it's on (the default) dmenu
returns straight after fuzzy-matching — `use_prefix` is never even read in
that path, so it has **no effect at all** while fuzzy matching is active;
toggling `-x` changes nothing until you've also passed `-F`.

Once fuzzy is disabled (`-F`), `use_prefix` decides what counts as a match:
`1` (default) keeps only exact and prefix matches; `0` (`-x`) also includes
plain substring matches anywhere in the item, ranked after the prefix ones.

Fuzzy mode's own scoring formula happens to reward matches that start early
and are contiguous, which tends to put prefix-like results first even in
fuzzy mode — but that's a property of the scoring itself, independent of
`use_prefix`.

## List

```c
static unsigned int lines = 0;  /* 0 = horizontal; N = vertical list with N rows */
```

Override per-invocation with `-l N`.

## Preselect

```c
static unsigned int preselected = 0;  /* item index highlighted on open; -n overrides */
```

## Flatpak apps

No config knob for this one — it's always on in the default (non-`-f`)
path. Installed flatpak apps are appended to whatever list `readstdin()`
already collected, so `dmenu_path | dmenu` (i.e. `dmenu_run`) shows `$PATH`
executables and flatpak apps together. Selecting a flatpak entry runs
`flatpak run <app-id>` instead of printing the selection to stdout.

## Word delimiters

```c
static const char worddelimiters[] = " ";
```

Characters treated as word boundaries for `Ctrl+W` (delete word). Add more
to taste: `" /?\\\"&[]"` for shell-style word boundaries.

## Keybindings (built-in, not configurable in config.h)

| Key            | Action                                              |
| -------------- | --------------------------------------------------- |
| Type           | Filter items                                        |
| `Enter`        | Accept selected item                                |
| `Shift+Enter`  | Accept typed input verbatim (even if not in list)   |
| `Escape`       | Exit without output                                 |
| `Tab`          | Complete to selected item                           |
| `↑` / `↓`      | Move selection (vertical mode)                      |
| `←` / `→`      | Move selection (horizontal mode) / move cursor      |
| `Ctrl+W`       | Delete word                                         |
| `Ctrl+U`       | Clear input                                         |
| `Ctrl+Y`       | Paste from primary selection                        |
| `Ctrl+Shift+Y` | Paste from clipboard (instead of primary selection) |
| `Page Up/Down` | Scroll list                                         |

## Command-line flags

| Flag                        | Effect                                                                                 |
| --------------------------- | -------------------------------------------------------------------------------------- |
| `-v`                        | Print version and exit                                                                 |
| `-b`                        | Appear at bottom of screen                                                             |
| `-f`                        | Grab keyboard before reading stdin (faster startup; only if stdin isn't a tty)         |
| `-F`                        | Disable fuzzy matching                                                                 |
| `-s`                        | Case-**sensitive** matching (matching is case-insensitive by default)                  |
| `-P`                        | Password mode: mask typed input with dots                                              |
| `-x`                        | Invert `use_prefix` (only has an effect once `-F` is also passed — see Matching above) |
| `-l N`                      | Vertical list with N lines                                                             |
| `-m N`                      | Show on monitor N (Xinerama, 0-indexed)                                                |
| `-n N`                      | Preselect item at index N                                                              |
| `-p prompt`                 | Override prompt text                                                                   |
| `-fn font`                  | Override font                                                                          |
| `-nb`/`-nf`/`-sb`/`-sf`     | Override normal/selected background/foreground colors                                  |
| `-nhb`/`-nhf`/`-shb`/`-shf` | Override normal/selected **highlight** background/foreground colors                    |
| `-w windowid`               | Embed into an existing window instead of creating one                                  |

There is no `-i` flag — matching is case-insensitive by default, and `-s`
switches to case-sensitive (the opposite of what a hypothetical `-i` flag
would suggest).
