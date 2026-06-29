# WIKI — Configuration Reference

All configuration lives in `config.h` and takes effect on rebuild
(`make clean install`) — there is no runtime config file or reload-without-
recompile, aside from the FIFO commands covered at the bottom of this page.

## Appearance

```c
static const unsigned int borderpx = 1;   /* window border width, px */
static const unsigned int gappx    = 8;   /* gap between windows, px */
static const unsigned int snap     = 16;  /* snap distance for floating windows, px */
static const int swallowfloating  = 0;    /* 1 = floating windows can swallow */
static const int showbar = 1;             /* 0 = no bar at all */
static const int topbar  = 1;             /* 0 = bar at bottom of screen */
```

## Wallpaper

```c
static const char *wallpaperdir      = "~/Pictures/Wallpapers/gruvbox";
static const int   wallpaperinterval = 900; /* seconds; 0 disables the timer */
static const char *fifopath          = "/tmp/dwm.fifo";
```

- `wallpaperdir` — directory scanned for images; `~` is expanded to `$HOME`.
- `wallpaperinterval` — how often a new random wallpaper is picked
  automatically. Set to `0` to disable automatic rotation entirely (manual
  trigger only, via keybind or FIFO).
- Manual triggers: `MODKEY+SHIFT+w`, `kill -USR1 $(pidof dwm)`, or
  `echo nextwallpaper > /tmp/dwm.fifo`.

## Fonts & Colors

```c
static const char *fonts[] = {
    "MesloLGS Nerd Font Mono:pixelsize=12",
    "NotoColorEmoji:pixelsize=12:antialias=true:autohint=true"
};
```

Multiple fonts act as fallbacks in order — the second entry here exists so
emoji in window titles/status text render instead of showing tofu boxes.

Colors are Gruvbox by default, defined as three named variables (fg/bg/border)
per state, then assembled into the `colors[][3]` table:

```c
[SchemeNorm] = {gruvbox_normfgcolor, gruvbox_normbgcolor, gruvbox_normbordercolor}, /* unfocused */
[SchemeSel]  = {gruvbox_selfgcolor,  gruvbox_selbgcolor,  gruvbox_selbordercolor},  /* focused */
[SchemeHid]  = {gruvbox_hidfgcolor,  gruvbox_hidbgcolor,  gruvbox_hidbordercolor},  /* hidden/scratchpad */
```

To re-theme, edit the named hex variables — don't restructure the table
itself unless you're also adding a new `Scheme*`.

## Tags

```c
static const char *tags[] = {"", "", "", "", ""};
```

Five tags, labeled with Nerd Font glyphs rather than numbers. Keybinds still
refer to them by index 0–4 (see Keybindings below), regardless of the glyph
shown in the bar.

## Scratchpads

Toggle-able floating terminal apps, bound via `togglescratch`:

| Slot | Tag bind | Command |
|---|---|---|
| 0 `termsc` | `MODKEY+t` | `st` (plain scratch terminal) |
| 1 `lfsc` | `MODKEY+y` | `st` running `lf` (file manager) |
| 2 `qalsc` | `MODKEY+z` | `st` running `qalc` (calculator) |
| 3 `wiremixsc` | `MODKEY+a` | `st` running `wiremix` (audio mixer) |
| 4 `gurks` | `MODKEY+g` | `st` running `gurks` |
| 5 `discordo` | `MODKEY+d` | `st` running `discordo` |
| 6 `twitch-tui` | `MODKEY+c` | `st` running `twt` |
| 7 `musicsc` | `MODKEY+m` | `st` running `subsonic-tui` |

To add a new scratchpad: add a `const char *spcmdN[]` array, add it to the
`scratchpads[]` table, add a matching rule in `rules[]` using `SPTAG(N)`,
and bind `togglescratch` with `{.ui = N}` in `keys[]`.

## Window rules

```c
static const Rule rules[] = {
    /* class       instance  title   tags mask  isfloating  isterminal  noswallow  monitor */
    {"Gimp",       NULL,     NULL,   0,         1,          0,          0,  -1},
    {"Firefox",    NULL,     NULL,   1 << 8,    0,          0,          -1, -1},
    {"St",         NULL,     NULL,   0,         0,          1,          0,  -1},
    ...
};
```

Match by WM class/instance/title (any can be `NULL` = wildcard). Notable
entries:
- `Firefox` is forced to tag index 8 (a "hidden" tag beyond the 5 visible
  ones) — useful for keeping a browser parked off your main tags.
- `St`, `Alacritty`, and WezTerm are marked `isterminal` so the `swallow`
  patch knows they're allowed to swallow GUI children they spawn.
- The `xev` test window (matched by title `"Event Tester"`) is marked
  `noswallow` so it doesn't get hidden by accident while debugging.

## Layouts

```c
static const float mfact        = 0.55; /* master area size, 0.05–0.95 */
static const int   nmaster      = 1;     /* clients in master area */
static const int   resizehints  = 0;     /* 1 = respect app size hints when tiled */
static const int   attachbelow  = 1;     /* 1 = new clients attach after the active one */
static const int   lockfullscreen = 1;   /* 1 = fullscreen client keeps focus */

static const Layout layouts[] = {
    {"", tile},    /* default on startup */
    {"", NULL},    /* floating */
    {"", monocle},
};
```

## Keybindings

All keybinds use `MODKEY` (= Super/Win key) as the primary modifier. Full
table lives in `keys[]`; grouped highlights below.

**Window/layout control**
| Key | Action |
|---|---|
| `MODKEY+j` / `k` | Focus next/prev visible window |
| `MODKEY+SHIFT+j` / `k` | Focus next/prev hidden window |
| `MODKEY+ALT+j` / `k` | Move window down/up in the stack |
| `MODKEY+h` / `l` | Shrink/grow master area |
| `MODKEY+SHIFT+=` / `-` | Increase/decrease master count |
| `MODKEY+SHIFT+z` | Zoom (swap with master) |
| `MODKEY+ALT+SHIFT+t/f/m` | Set layout: tile / floating / monocle |
| `MODKEY+SHIFT+f` | Toggle fullscreen |
| `MODKEY+SHIFT+space` | Toggle floating |
| `MODKEY+=` / `-` | Show / hide focused window |
| `MODKEY+0` / `SHIFT+0` | View / tag all tags |
| `MODKEY+,` / `.` | Focus prev/next monitor |
| `MODKEY+SHIFT+,` / `.` | Send window to prev/next monitor |

**Tags** — `MODKEY+[1-5]` view tag, `+CTRL` toggle-view, `+SHIFT` move
window to tag, `+CTRL+SHIFT` toggle-tag on window.

**System**
| Key | Action |
|---|---|
| `MODKEY+SHIFT+q` | Quit dwm |
| `MODKEY+SHIFT+r` | Restart dwm (re-exec, preserves session) |
| `MODKEY+q` | Kill focused client |
| `MODKEY+SHIFT+w` | Next wallpaper |

**Apps/launchers** — `MODKEY+Return` (terminal), `+f` (file manager),
`+b` (browser), `+r` (dmenu), plus dedicated launchers for emoji picker,
power menu, screen recorder, etc. — see `keys[]` for the full list, these
mostly call out to small wrapper scripts (`sysctl`, `mediactl`, etc.) rather
than calling apps directly.

**Media/volume/mic/brightness** — XF86 keys are bound where the hardware
sends them; `MODKEY+ALT`/`MODKEY+SHIFT` combos are provided as a fallback
for keyboards without media keys.

**Screenshots** — `Print` alone, plus `MODKEY`/`SHIFT`/`CTRL`/`ALT+Print`
for select/screen/full/window/colorpicker variants.

## Mouse bindings

| Click target | Button | Action |
|---|---|---|
| Layout symbol | Left | Cycle to tile layout |
| Layout symbol | Right | Cycle to monocle |
| Window title | Left | Toggle window (scratchpad-style) |
| Window title | Middle | Zoom |
| Status text | Left/Middle/Right | Sent to status script via `sigstatusbar` |
| Client window | `MODKEY`+Left | Move (drag) |
| Client window | `MODKEY`+Middle | Toggle floating |
| Client window | `MODKEY`+Right | Resize (drag) |
| Tag bar | Left/Right | View / toggle-view tag |
| Tag bar | `MODKEY`+Left/Middle | Tag / toggle-tag window |

## FIFO commands (IPC)

dwm reads commands from `/tmp/dwm.fifo` (path set via `fifopath`) on every
event loop tick (~10ms latency). Send a command by writing a line to it:

```sh
echo "<command> [arg]" > /tmp/dwm.fifo
```

| Command | Arg | Effect |
|---|---|---|
| `view` | tag index (0–4) | Switch to that tag |
| `tag` | tag index (0–4) | Move focused window to that tag |
| `togglebar` | — | Show/hide the bar |
| `killclient` | — | Close the focused window |
| `zoom` | — | Swap focused window with master |
| `setmfact` | float (e.g. `0.6`) | Set master area fraction directly |
| `nextwallpaper` | — | Force a new random wallpaper |
| `quit` | — | Quit dwm |

Examples:
```sh
echo "view 2" > /tmp/dwm.fifo
echo "setmfact 0.65" > /tmp/dwm.fifo
echo "nextwallpaper" > /tmp/dwm.fifo
```

This is intended for scripting — bind it to acpi events, a rofi menu,
a hardware button, or any external trigger that shouldn't need its own
X11 keybind. See `DOCS.md` → "Adding a new FIFO command" to extend the
table.

## Autostart

`autostart.sh` runs once at dwm startup (called from `main()` before the
event loop starts). Put any background processes you want running every
session in there (compositor, wallpaper helper daemons, notification
daemon, etc.) rather than in `.xinitrc`, so they're tied to dwm's lifecycle.
