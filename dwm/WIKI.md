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
[SchemeUrg]  = {gruvbox_urgfgcolor,  gruvbox_urgbgcolor,  gruvbox_urgbordercolor},  /* urgency hint */
```

`SchemeUrg` is used for any tag with an urgent client on it, and for that
client's own tab in the title area — both in `drawbar()`. It only fires
off `XWMHintsIsUrgent`/`isurgent`, which clears itself the moment you
focus the client.

To re-theme, edit the named hex variables — don't restructure the table
itself unless you're also adding a new `Scheme*`.

## Tags

```c
typedef struct {
  const char *icon; /* shown in the bar */
  const char *name; /* plain-text name, e.g. for the fifo state reply */
} Tag;

const Tag tags[] = {
    {"󰖟", "web"}, {"󰙯", "chat"}, {"", "dev"},
    {"", "game"}, {"󰨇", "vm"},
};
const int tagslen = LENGTH(tags);
```

Five tags, each with two labels: `icon` (a Nerd Font glyph, drawn in the
bar via `drawbar()`) and `name` (a plain-text name used anywhere a glyph
isn't renderable or parseable — currently only the FIFO `state` reply, see
"Querying state back out" below). Keybinds still refer to tags by index
0–4 (see Keybindings below), regardless of either label.

`tagslen` is computed automatically from the array length — add or remove
a `{icon, name}` pair and everything downstream (bar rendering, keybind
generation via `TAGKEYS`, the FIFO reply, the compile-time 31-tag limit
check) adjusts on its own. You do still need to add/remove the matching
`TAGKEYS(XK_n, N)` line in `keys[]` by hand if you change the count.

## Scratchpads

Toggle-able floating terminal apps, bound via `togglescratch`:

| Slot           | Tag bind   | Command                              |
| -------------- | ---------- | ------------------------------------ |
| 0 `termsc`     | `MODKEY+t` | `st` (plain scratch terminal)        |
| 1 `lfsc`       | `MODKEY+y` | `st` running `lf` (file manager)     |
| 2 `qalsc`      | `MODKEY+z` | `st` running `qalc` (calculator)     |
| 3 `wiremixsc`  | `MODKEY+a` | `st` running `wiremix` (audio mixer) |
| 4 `gurks`      | `MODKEY+g` | `st` running `gurks`                 |
| 5 `discordo`   | `MODKEY+d` | `st` running `discordo`              |
| 6 `twitch-tui` | `MODKEY+c` | `st` running `twt`                   |
| 7 `musicsc`    | `MODKEY+m` | `st` running `subsonic-tui`          |

To add a new scratchpad: add a `const char *spcmdN[]` array, add it to the
`scratchpads[]` table, add a matching rule in `rules[]` using `SPTAG(N)`,
and bind `togglescratch` with `{.ui = N}` in `keys[]`.

`MODKEY+SHIFT+t` (`hideallscratchpads`) collapses every currently visible
scratchpad out of view in one call — handy before a screen share.

Note: scratchpad 3 (`wiremixsc`) can also be toggled from outside a
keybind entirely — it's how the status bar's volume block opens the mixer
on right-click, via `echo "togglescratch 3" > /tmp/dwm.fifo`. Any external
script can pop a scratchpad this way, not just dwm's own keybindings.

## Window rules

```c
static const Rule rules[] = {
    /* class     instance      title           tags mask  isfloating  isterminal
                     noswallow  monitor  w  h   x   y  setpos  center  forcefullscreen
       w/h: 0 = keep the client's requested size
       x/y: only applied when setpos=1; offset from the monitor's
            work-area origin (top-left)
       setpos: 1 = place at x,y instead of dwm's default centering
       center: 1 = explicitly center (dwm's default anyway; mostly for
               readability, or to force-center a rule that would
               otherwise not match the defaults)
       forcefullscreen: 1 = go fullscreen immediately on open */
    {"Gimp",       NULL,     NULL,   0,         1,          0,          0,  -1},
    {"Firefox",    NULL,     NULL,   1 << 8,    0,          0,          -1, -1},
    {"St",         NULL,     NULL,   0,         0,          1,          0,  -1},
    {NULL, NULL, "^Picture-in-Picture$", 0, 1, 0, 0, -1, 480, 270, 14, 12, 1, 0},
    {"^steam_app_(?!0$)[0-9]+$", NULL, NULL, 0, 0, 0, 0, -1, 0, 0, 0, 0, 0, 0, 1},
    ...
};
```

Rules have 15 positional fields; trailing fields you don't need can simply
be omitted — C zero-fills the rest of the struct (`0` for numeric fields,
`NULL` for pointers), which for every trailing field here means "don't
override, use dwm's default behavior."

### Matching (`class` / `instance` / `title`)

Any of the three can be `NULL` (wildcard, matches everything). Non-`NULL`
fields are **full PCRE2 regular expressions**, not plain substrings —
anchors (`^`/`$`), character classes, alternation, and lookaround
(`(?!...)`, `(?=...)`) all work exactly like they would in Hyprland's
`windowrule` matching. An unanchored literal like `"Firefox"` still behaves
like the old plain substring match, since PCRE2 without anchors searches
for the pattern anywhere in the string — so existing simple rules didn't
need to change.

Each rule's pattern is compiled once (cached internally, keyed per rule/
field) the first time a matching window is opened, not re-compiled on every
window open. If a pattern fails to compile (bad regex syntax), dwm prints
an error to stderr naming the pattern and the byte offset of the problem,
and that rule simply never matches (it doesn't crash or skip other rules).

**Requires `libpcre2-8`** — see [Requirements](README.md#requirements).

### Size / move / center

- `w`, `h` — override the client's requested size in pixels. `0` leaves
  the app's own requested size alone.
- `x`, `y`, `setpos` — `setpos = 1` places the window at `(x, y)` measured
  from the top-left of the monitor's work area (below the bar, inside
  gaps), instead of dwm's default "center it" behavior. This is the
  `move` equivalent from Hyprland's rule syntax.
- `center` — explicit opt-in to centering. Since centering is already
  dwm's default for new windows, this mostly exists for readability, or to
  force a window back to centered if an earlier-matching rule set
  `setpos`.

There's no dwm equivalent for Hyprland's `pin` (keep a window visible
across tag/workspace switches) — that would need extra state tracked
through `arrange()`/`showhide()`, not just a `Rule` field, and isn't
implemented here.

### Forcefullscreen

`forcefullscreen = 1` calls the same `setfullscreen()` path a client's own
`_NET_WM_STATE_FULLSCREEN` request would trigger, applied right after the
window's monitor and geometry are finalized in `manage()`. Useful for
apps (like Steam games) that don't request fullscreen themselves but you
always want fullscreened on open.

The Steam example above uses a negative lookahead (`(?!0$)`) to
fullscreen every `steam_app_NNNNN` window except `steam_app_0`, which some
Steam launch paths use for the client UI itself rather than an actual game.

### Notable existing entries

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
    /* symbol   name       arrange function */
    {"", "tile", tile},  /* first entry is default */
    {"", "float", NULL}, /* no layout function means floating behavior */
    {"󰊓", "monocle", monocle},
};
```

Each layout has both a `symbol` (the glyph shown in the bar via `ltsymbol`)
and a plain-text `name`. The name isn't drawn anywhere in the bar; it only
surfaces in the FIFO `state` reply (`layout=tile`/`float`/`monocle`) so a
script reading it doesn't have to parse a Nerd Font glyph.

`monocle()` still overrides `ltsymbol` at runtime to show a client count,
but now prepends the icon instead of replacing it: `[N]` becomes
`<icon>[N]`, so the bar keeps a consistent glyph across all three layouts.

## Keybindings

All keybinds use `MODKEY` (= Super/Win key) as the primary modifier. Full
table lives in `keys[]`; grouped highlights below.

**Window/layout control**

| Key                      | Action                                |
| ------------------------ | ------------------------------------- |
| `MODKEY+j` / `k`         | Focus next/prev visible window        |
| `MODKEY+SHIFT+j` / `k`   | Focus next/prev hidden window         |
| `MODKEY+ALT+j` / `k`     | Move window down/up in the stack      |
| `MODKEY+h` / `l`         | Shrink/grow master area               |
| `MODKEY+SHIFT+=` / `-`   | Increase/decrease master count        |
| `MODKEY+SHIFT+z`         | Zoom (swap with master)               |
| `MODKEY+ALT+SHIFT+t/f/m` | Set layout: tile / floating / monocle |
| `MODKEY+ALT+space`       | Cycle to next layout                  |
| `MODKEY+ALT+SHIFT+space` | Cycle to prev layout                  |
| `MODKEY+SHIFT+f`         | Toggle fullscreen                     |
| `MODKEY+SHIFT+space`     | Toggle floating                       |
| `MODKEY+=` / `-`         | Show / hide focused window            |
| `MODKEY+SHIFT+t`         | Hide all visible scratchpads          |
| `MODKEY+0` / `SHIFT+0`   | View / tag all tags                   |
| `MODKEY+,` / `.`         | Focus prev/next monitor               |
| `MODKEY+SHIFT+,` / `.`   | Send window to prev/next monitor      |

**Tags** — `MODKEY+[1-5]` view tag, `+CTRL` toggle-view, `+SHIFT` move
window to tag, `+CTRL+SHIFT` toggle-tag on window.

**System**

| Key              | Action                                   |
| ---------------- | ---------------------------------------- |
| `MODKEY+SHIFT+q` | Quit dwm                                 |
| `MODKEY+SHIFT+r` | Restart dwm (re-exec, preserves session) |
| `MODKEY+q`       | Kill focused client                      |
| `MODKEY+SHIFT+w` | Next wallpaper                           |

**Apps/launchers** — `MODKEY+Return` (terminal), `+f` (file manager),
`+b` (browser), `+r` (dmenu), plus dedicated launchers for emoji picker,
power menu, screen recorder, etc. — see `keys[]` for the full list, these
mostly call out to small wrapper scripts (`sysctl`, `mediactl`, etc.) rather
than calling apps directly.

**Media/volume/mic/brightness** — XF86 keys are bound where the hardware
sends them; `MODKEY+ALT`/`MODKEY+SHIFT` combos are provided as a fallback
for keyboards without media keys. The status bar itself is also
interactive for these — see the "Status text" row in Mouse bindings below.

**Screenshots**

| Key                  | Action                      |
| -------------------- | --------------------------- |
| `Print`              | Select a region to capture  |
| `MODKEY+Print`       | Capture focused monitor     |
| `MODKEY+SHIFT+Print` | Capture full (all monitors) |
| `MODKEY+CTRL+Print`  | Capture focused window      |
| `MODKEY+ALT+Print`   | Pick a color under cursor   |

Screenshots save to `~/Pictures/Screenshots/<timestamp>.png`, copy to
clipboard via `xclip`, and confirm via `notify-send`. The colorpicker copies
the hex value as text (not a file) and notifies with the hex string instead
of an image thumbnail.

## Mouse bindings

| Click target  | Button                           | Action                                                                 |
| ------------- | -------------------------------- | ---------------------------------------------------------------------- |
| Layout symbol | Left                             | Cycle to tile layout                                                   |
| Layout symbol | Right                            | Cycle to monocle                                                       |
| Window title  | Left                             | Toggle window (scratchpad-style)                                       |
| Window title  | Middle                           | Zoom                                                                   |
| Status text   | Left/Middle/Right/Scroll up/down | Sent to whichever block script is under the cursor, via `sigstatusbar` |
| Client window | `MODKEY`+Left                    | Move (drag)                                                            |
| Client window | `MODKEY`+Middle                  | Toggle floating                                                        |
| Client window | `MODKEY`+Right                   | Resize (drag)                                                          |
| Tag bar       | Left/Right                       | View / toggle-view tag                                                 |
| Tag bar       | `MODKEY`+Left/Middle             | Tag / toggle-tag window                                                |

Unlike the other rows, "Status text" isn't one fixed action — dwm only
identifies _which block_ was clicked (by the signal number embedded in
that segment of the status string) and forwards the button number to it.
What actually happens is entirely up to that block's own script via
`$BLOCK_BUTTON`. See the dwmblocks-async repo's WIKI/README for the
current per-block mapping (volume, brightness, media, etc.) — blocks with
signal `0` in dwmblocks' `BLOCKS()` config can't be clicked at all, dwm
ignores clicks on that segment unconditionally.

## FIFO commands (IPC)

dwm reads commands from `/tmp/dwm.fifo` (path set via `fifopath`) on every
event loop tick (~10ms latency). Send a command by writing a line to it:

```sh
echo "<command> [arg]" > /tmp/dwm.fifo
```

| Command            | Arg              | Effect                                 |
| ------------------ | ---------------- | -------------------------------------- |
| `view`             | 0–4              | Switch to tag                          |
| `tag`              | 0–4              | Move window to tag                     |
| `toggleview`       | 0–4              | Add/remove tag from current view       |
| `toggletag`        | 0–4              | Toggle tag on focused window           |
| `setmfact`         | float e.g. `0.6` | Set master area size                   |
| `incnmaster`       | `1` or `-1`      | Add/remove master client               |
| `cyclelayout`      | `1` or `-1`      | Cycle to next/prev layout              |
| `zoom`             | —                | Swap focused window with master        |
| `togglefloating`   | —                | Toggle float on focused window         |
| `togglefullscreen` | —                | Toggle fullscreen                      |
| `focusstackvis`    | `1` or `-1`      | Focus next/prev visible window         |
| `focusmon`         | `1` or `-1`      | Focus next/prev monitor                |
| `tagmon`           | `1` or `-1`      | Send window to next/prev monitor       |
| `switchcol`        | —                | Focus first window in the other column |
| `show`             | —                | Show focused window                    |
| `hide`             | —                | Hide focused window                    |
| `showall`          | —                | Show all hidden windows                |
| `togglewin`        | —                | Toggle hide/show on the focused window |
| `killclient`       | —                | Close focused window                   |
| `togglescratch`    | 0–7              | Toggle scratchpad by index             |
| `hideallscratch`   | —                | Hide every visible scratchpad          |
| `togglebar`        | —                | Show/hide bar                          |
| `nextwallpaper`    | —                | Load new random wallpaper              |
| `screenshot`       | 0–3              | Capture full/monitor/window/select     |
| `colorpicker`      | —                | Pick a color under cursor              |
| `state`            | —                | Write a state dump to `fiforeplypath`  |
| `quit`             | —                | Quit dwm (`1` = restart)               |

Examples:

```sh
echo "view 2" > /tmp/dwm.fifo
echo "setmfact 0.65" > /tmp/dwm.fifo
echo "nextwallpaper" > /tmp/dwm.fifo
echo "togglescratch 3" > /tmp/dwm.fifo
```

This is intended for scripting — bind it to acpi events, a rofi menu,
a hardware button, or any external trigger that shouldn't need its own
X11 keybind. See `DOCS.md` → "Adding a new FIFO command" to extend the
table.

Note: `screenshot 3` (select) and `colorpicker` block dwm's event loop
until the interaction completes, same as a mouse-drag resize would — don't
trigger them from something expecting an instant return.

### Querying state back out

The command fifo is one-way (script → dwm). For the other direction, send
`state` and then read `/tmp/dwm.fifo.reply` (path set via `fiforeplypath`):

```sh
echo "state" > /tmp/dwm.fifo
cat /tmp/dwm.fifo.reply
# mon=0 tags=web layout=monocle clients=3 urgent=0 title=st
```

`tags` here is built from each visible tag's `name` (not `icon`, and not a
raw bitmask) — if multiple tags are being viewed at once (e.g. after
`MODKEY+CTRL+2` toggle-views tag 2 on top of tag 1), they're joined with
`+`: `tags=web+chat`.

`clients` is the visible-client count on the current tag — the same count
monocle's bar symbol shows as `[N]`, just available to scripts without
having to parse the glyph. It's not layout-specific; it's reported for
every layout, since it's cheap and generally useful (e.g. deciding whether
it's worth switching to monocle).

The reply fifo is opened `O_RDWR|O_NONBLOCK` the same way the command fifo
is, so writing to it never blocks dwm even if nothing is reading it. Any
unread previous reply is drained before the new one is written, so it
can't grow unbounded — but that also means a reply you don't read within
one `state` call is gone, so read it right after sending the command
rather than polling it independently.

## Autostart

`autostart.sh` runs once at dwm startup (called from `main()` before the
event loop starts). Put any background processes you want running every
session in there (compositor, wallpaper helper daemons, notification
daemon, etc.) rather than in `.xinitrc`, so they're tied to dwm's lifecycle.
