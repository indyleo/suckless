# WIKI — st Configuration Reference

All config lives in `config.h`. Rebuild with `make clean install` after any
change.

## Appearance

```c
static char *font   = "CaskaydiaCove NF:size=12";
static int borderpx = 6;
static float cwscale = 1.0;  /* character width multiplier */
static float chscale = 1.0;  /* character height multiplier */
```

## Shell / program execution

```c
static char *shell = "/bin/zsh";
char *utmp  = NULL;   /* utmp program, e.g. "utmp" — NULL disables */
char *scroll = NULL;  /* external scroll program — NULL since scrollback patch handles this in-terminal */
```

Precedence for what st execs: `-e` flag > `scroll`/`utmp` > `$SHELL` env var
> `/etc/passwd` shell entry > the `shell` value above.

## Transparency

```c
float alpha = 0.85;  /* 0.0 = fully transparent, 1.0 = fully opaque */
```

Requires a running compositor (picom) — without one, the window renders as
solid background color regardless of this value.

## Colors (Gruvbox, 16-color + extras)

```c
static const char *colorname[] = {
    "#282828", "#cc241d", "#98971a", "#d79921",  /* black red green yellow */
    "#458588", "#b16286", "#689d6a", "#a89984",  /* blue magenta cyan white */
    "#928374", "#fb4934", "#b8bb26", "#fabd2f",  /* bright variants 8-11 */
    "#83a598", "#d3869b", "#8ec07c", "#ebdbb2",  /* bright variants 12-15 */
    [255] = 0,
    "#ebdbb2", /* [256] default foreground */
    "#282828", /* [257] default background */
    "#458588", /* [258] selection background */
    "#ebdbb2", /* [259] selection foreground */
};

unsigned int defaultfg = 256;
unsigned int defaultbg = 257;
unsigned int defaultcs = 256;      /* cursor color */
unsigned int selectionbg = 258;
unsigned int selectionfg = 259;
static int ignoreselfg = 0;        /* 0 = use selectionfg uniformly; 1 = preserve each cell's own fg */
```

To re-theme: edit `colorname[]` 0–15 for the ANSI palette, and 256–259 for
defaults/selection. Index 255 must stay `0` (reserved).

## Cursor

```c
static unsigned int cursorstyle = 1;     /* 1 = blinking block (default) */
static Rune stcursor = 0x2603;           /* snowman ☃ — used for styles 7/8 */
static unsigned int cursorthickness = 2; /* underline/bar thickness, px */
static unsigned int blinktimeout = 800;  /* ms; 0 disables blinking entirely */
```

Cursor style values: `0`/`1` blinking block, `2` steady block, `3` blinking
underline, `4` steady underline, `5` blinking bar, `6` steady bar, `7`/`8`
blinking/steady custom glyph (`stcursor`).

## Latency / sync

```c
static double minlatency = 2;   /* ms — minimum delay before drawing */
static double maxlatency = 33;  /* ms — maximum delay before forcing a draw */
```

st batches PTY output and waits for it to go idle (within this range) before
redrawing, to avoid tearing/flicker on rapid output. Lower `minlatency` =
more responsive but more visual tearing on fast scrollers; higher
`maxlatency` = smoother but more perceptible input lag on the first
character of a burst.

## Bell

```c
static int bellvolume = 0;  /* -100 to 100; 0 = silent visual bell only */
```

## Tabs

```c
unsigned int tabspaces = 8;
```

Changing this requires regenerating `st.info`'s `it#` value and reinstalling
the terminfo entry (`tic -sx st.info`), or tab expansion will be wrong in
apps that query terminfo directly.

## Mouse

```c
static uint forcemousemod = ShiftMask;
```

Hold this modifier to force mouse selection/shortcuts even when an app has
grabbed mouse mode (`MODE_MOUSE`, e.g. vim, tmux, htop).

Mouse shortcuts (`mshortcuts[]`):

| Button | Modifier | Action |
|---|---|---|
| Button2 (middle-click) | any | Paste primary selection |
| Scroll up | any | Send `\033[5;2~` (app mode) or `\031` |
| Scroll down | any | Send `\033[6;2~` (app mode) or `\005` |

## Keyboard shortcuts

```c
static Shortcut shortcuts[] = {
    {TERMMOD, XK_Prior, zoom,      {.f = +1}},  /* Ctrl+Shift+PageUp:   font size + */
    {TERMMOD, XK_Next,  zoom,      {.f = -1}},  /* Ctrl+Shift+PageDown: font size - */
    {TERMMOD, XK_Home,  zoomreset, {.f =  0}},  /* Ctrl+Shift+Home: reset font size */
    {TERMMOD, XK_C,     clipcopy,  {.i =  0}},  /* Ctrl+Shift+C: copy */
    {TERMMOD, XK_V,     clippaste, {.i =  0}},  /* Ctrl+Shift+V: paste */
    {TERMMOD, XK_Y,     selpaste,  {.i =  0}},  /* Ctrl+Shift+Y: paste primary selection */
    {ShiftMask, XK_Insert, selpaste, {.i = 0}}, /* Shift+Insert: paste primary selection */
    {ShiftMask, XK_Page_Up,   kscrollup,   {.i = -1}}, /* Shift+PgUp: scrollback up */
    {ShiftMask, XK_Page_Down, kscrolldown, {.i = -1}}, /* Shift+PgDn: scrollback down */
};
```

`TERMMOD` is `Ctrl+Shift` by default. The large `key[]` array below this
(in `config.h`) maps special keys (arrows, function keys, keypad) to their
correct escape sequences for both normal and application cursor/keypad
modes — not normally edited unless you're adding support for an unusual key.

## Word delimiters

```c
wchar_t *worddelimiters = L" ";
```

Characters treated as word boundaries for double-click word selection. For
shell-style boundaries: `L" \`'\"()[]{}"`.
