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

| Scheme | Foreground | Background | Use |
|---|---|---|---|
| `SchemeNorm` | `#ebdbb2` | `#282828` | Unselected items |
| `SchemeSel` | `#282828` | `#fabd2f` | Selected item |
| `SchemeSelHighlight` | `#fe8019` | `#fabd2f` | Matched chars in selected item |
| `SchemeNormHighlight` | `#fe8019` | `#282828` | Matched chars in unselected items |
| `SchemeOut` | `#282828` | `#8ec07c` | Multi-select output items |
| `SchemePrompt` | `#928374` | `#282828` | Prompt text |

## Matching

```c
static int fuzzy = 1;       /* 1 = fuzzy matching on by default; -F disables */
static int use_prefix = 1;  /* 1 = prefer prefix matches; -x inverts */
```

With both on (default): fuzzy matching is active, but prefix matches score
higher and appear first in the list.

## List

```c
static unsigned int lines = 0;  /* 0 = horizontal; N = vertical list with N rows */
```

Override per-invocation with `-l N`.

## Preselect

```c
static unsigned int preselected = 0;  /* item index highlighted on open; -n overrides */
```

## Word delimiters

```c
static const char worddelimiters[] = " ";
```

Characters treated as word boundaries for `Ctrl+W` (delete word). Add more
to taste: `" /?\\\"&[]"` for shell-style word boundaries.

## Keybindings (built-in, not configurable in config.h)

| Key | Action |
|---|---|
| Type | Filter items |
| `Enter` | Accept selected item |
| `Shift+Enter` | Accept typed input verbatim (even if not in list) |
| `Escape` | Exit without output |
| `Tab` | Complete to selected item |
| `↑` / `↓` | Move selection (vertical mode) |
| `←` / `→` | Move selection (horizontal mode) / move cursor |
| `Ctrl+W` | Delete word |
| `Ctrl+U` | Clear input |
| `Ctrl+Y` | Paste from primary selection |
| `Page Up/Down` | Scroll list |

## Command-line flags

| Flag | Effect |
|---|---|
| `-b` | Appear at bottom of screen |
| `-f` | Grab keyboard before reading stdin (faster startup) |
| `-F` | Disable fuzzy matching |
| `-i` | Case-insensitive matching |
| `-l N` | Vertical list with N lines |
| `-n N` | Preselect item at index N |
| `-p prompt` | Override prompt text |
| `-x` | Toggle prefix matching mode |
| `-fn font` | Override font |
| `-nb`/`-nf`/`-sb`/`-sf` | Override normal/selected bg/fg colors |
