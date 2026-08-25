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

`fonts[]` (and a companion `fontslen`) are declared without `static` here
deliberately — `osd.c` builds its own small font set from the same list
so the OSD popup's text matches the bar's, see "On-screen display (OSD)"
below.

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

The `gruvbox_*` variables above aren't hand-typed hex strings anymore —
each is an alias into `theme.h`, e.g. `gruvbox_normfgcolor` is just
`THEME_TEXT`. **To re-theme, edit `theme.h`, not this block.** `theme.h`
defines the palette two ways:

- `CAL0`..`CAL15` — the raw Gruvbox Dark hex values, named the same way
  as the companion `qs` (Quickshell) config's `Theme.qml`, so both
  projects can be kept in sync from one mental model.
- Semantic aliases (`THEME_BACKGROUND`, `THEME_TEXT`, `THEME_ACCENT`,
  etc.) — prefer these over the raw `CAL*` names in new code, since they
  describe *role* rather than palette index.

A handful of accents (`THEME_SEL_BORDER`, `THEME_HID_FG`, `THEME_HID_BG`,
`THEME_URG_FG`) live outside the shared 16-color set — dwm needs a couple
of shades (a selected-border tint, a harder-contrast hidden-tag bg) that
a shell/bar config never does. These are also defined in `theme.h`, just
called out separately in its header comment so it's clear they're
dwm-specific rather than part of the portable palette.

Don't restructure the `colors[][3]` table itself unless you're also
adding a new `Scheme*`.

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
keybind entirely — it's how the status bar's volume block can open the
mixer on right-click, by having that block's `statusblocks[]` command run
`echo "togglescratch 3" > /tmp/dwm.fifo` when `$BLOCK_BUTTON` is 3. Any
external script can pop a scratchpad this way, not just dwm's own
keybindings.

## Status bar blocks

```c
const char *statusdelim = " || ";  /* visible separator printed between blocks */
const int statusmaxlen = 45;        /* max Unicode codepoints kept per block */
const int statusclickable = 1;      /* 0 disables click-routing entirely */
const int statusleaddelim = 0;      /* 1 = print statusdelim before the first block too */
const int statustraildelim = 0;     /* 1 = print statusdelim after the last block too */

static const StatusBlock statusblocks[] = {
    /* icon  cmd                      interval(s) */
    {"", "mediactl state-title", 0},  /* refreshed by a track-change hook */
    {"", "sysstats kernel", 300},
    {"", "sysstats cpu", 3},
    {"", "sysstats gpu", 3},
    {"", "sysstats mem", 5},
    {"", "sysstats disk", 10},
    {"", "sysstats brightness", 0},   /* refreshed by the OSD, see below */
    {"", "<battery, hides on empty/N/A/No/Not/0%>", 15},
    {"", "<ethernet, hides unless Connected>", 15},
    {"", "<wifi, hides unless connected or ethernet is down>", 15},
    {"", "<tailscale, hides unless Connected>", 30},
    {"", "sysstats microphone", 0},   /* refreshed by the OSD, see below */
    {"", "sysstats volume", 0},       /* refreshed by the OSD, see below */
    {"", "sysstats date_time", 30},
    {"", "", 0},                      /* notifications -- pushed by
                                       * notifications.c directly, no
                                       * command to run; see
                                       * "Notifications" below */
};
```

The three network rows and the battery row above are shortened to their
intent here — see `config.h` for the actual `case`-guarded shell one-liners
each one runs, and the syspill hide-rule comment directly above
`statusblocks[]` in `config.h` for exactly what each guard matches on
(ported from the Quickshell bar's `shell.qml`, cross-checked against the
real `sysstats` script's wording). This order also isn't arbitrary: it
mirrors the Quickshell bar's `statsRow` layout left-to-right (kernel/cpu/
gpu, then mem/disk, then brightness/battery, then ethernet/wifi/
tailscale, then mic/volume), with the media block first and the clock
last, so the two bars read the same way if you're switching between them.

This replaces the old dwmblocks binary — there's nothing external to
install or autostart anymore, the bar builds its own text in-process. The
five knobs above mirror dwmblocks-async's `config.h` (`DELIMITER`,
`MAX_BLOCK_OUTPUT_LENGTH`, `CLICKABLE_BLOCKS`, `LEADING_DELIMITER`,
`TRAILING_DELIMITER`) one-for-one:

- `statusdelim` — the visible text printed between blocks (`" || "`
  above). This is separate from the invisible click-routing byte each
  block also gets — changing this only changes what you see, not how
  clicks are resolved.
- `statusmaxlen` — each block's trimmed output is truncated to this many
  Unicode codepoints (not bytes — a 4-byte emoji still counts as one),
  so one runaway block can't push everything else off the bar.
- `statusclickable` — set to `0` to disable click-routing bar-wide (no
  block ever receives `$BLOCK_BUTTON`, and clicking the status area does
  nothing). Leave at `1` for the normal per-block click behavior
  described below.
- `statusleaddelim` / `statustraildelim` — whether `statusdelim` also
  appears before the first block / after the last one. Both default to
  `0` (no leading/trailing separator), matching dwmblocks-async's
  defaults.

Each `statusblocks[]` row is `{icon, cmd, interval}`:

- `icon` — a short glyph/prefix, purely cosmetic, can be `""`.
- `cmd` — a full shell command (`popen`'d, so pipes and quoting work);
  only its first line of stdout is used. It can emit the same
  `^c#hex^`/`^b#hex^`/`^f<N>^` color codes the rest of the bar
  understands (see Fonts & Colors above) — those are stripped from width
  calculations but change the block's own color.
- `interval` — seconds between automatic reruns. `0` means the block
  only updates when clicked, or when something explicitly refreshes it
  (the `statusblock N` FIFO command, or the OSD popup below poking its
  matching block after a volume/brightness change).

The `mediactl state-title` block above has `interval = 0` on purpose —
under dwmblocks it was refreshed by a real-time signal fired whenever the
track changed. There's no signal to send anymore; point whatever watches
your media player at `echo "statusblock 0" > /tmp/dwm.fifo` instead (`0`
being that block's index in the array above).

On click, the block's `cmd` is rerun with `BLOCK_BUTTON` set in its
environment to the button number (1 = left, 2 = middle, 3 = right, 4/5 =
scroll up/down) — same convention the old dwmblocks setup used, so a
block script that branches on `$BLOCK_BUTTON` (e.g. left-click mutes,
right-click opens a mixer) doesn't need to change. See "Mouse bindings"
below for how a click gets routed to the right block in the first place.

To add a block: add a row to `statusblocks[]`. No index anywhere else to
update except any `osds[]`/`OsdItem.blockidx` entry that should point at
it — the bar, click routing, and the FIFO/OSD refresh path all size
themselves off the array automatically (`statusblockslen`), up to a hard
cap of 31 blocks.

## On-screen display (OSD)

```c
enum {
  OsdVolUp, OsdVolDown, OsdVolToggle,
  OsdBriUp, OsdBriDown,
  OsdMicUp, OsdMicDown, OsdMicToggle
};

static const OsdItem osds[] = {
    /* label  changecmd       getcmd      statusblocks[] index (-1 = none) */
    {"VOL", volupcmd,     volgetcmd, 12},  /* statusblocks[12] = "sysstats volume" */
    {"VOL", voldowncmd,   volgetcmd, 12},
    {"VOL", voltogglecmd, volgetcmd, 12},
    {"BRI", briupcmd,     brigetcmd, -1},  /* brightness has a pill at
                                            * statusblocks[6], but fastget
                                            * already keeps it in sync
                                            * without a rebuild(), so this
                                            * is left at -1 -- see the
                                            * comment above osds[] in
                                            * config.h */
    {"BRI", bridowncmd,   brigetcmd, -1},
    {"MIC", micupcmd,     micgetcmd, -1},
    {"MIC", micdowncmd,   micgetcmd, -1},
    {"MIC", mictogglecmd, micgetcmd, -1},
};
```

A small popup (bottom-center of the screen) that flashes a label and a
percentage bar for ~1.2s whenever you raise/lower/toggle volume,
brightness, or mic. Each `osds[]` row is `{label, changecmd, getcmd,
blockidx}`:

- `label` — short text shown in the popup, e.g. `"VOL"`.
- `changecmd` — argv array run first (the thing that actually raises/
  lowers/toggles the value).
- `getcmd` — argv array run afterward; its stdout must be a bare integer
  0–100 (no `%`, no units) for the level bar. Pass `NULL` to skip the bar
  and just flash the label.
- `blockidx` — index into `statusblocks[]` above to refresh immediately
  afterward, so the bar doesn't lag a beat behind the popup, or `-1` if
  nothing in the bar mirrors this control.

Both `changecmd` and `getcmd` are plain argv arrays (`{"sysctl", "vol",
"-i", "5", NULL}`), not shell strings — no shell is invoked, so no
quoting/pipes, but also no per-keypress `sh -c` overhead, which matters
since these fire on every repeat of a held-down key.

The `enum` above just gives the array indices readable names —
`keys[]` binds each control with `{.i = OsdVolUp}` etc.:

| Key                                                | Action          |
| -------------------------------------------------- | --------------- |
| `MODKEY+ALT+Up` / `XF86AudioRaiseVolume`           | Volume up       |
| `MODKEY+ALT+Down` / `XF86AudioLowerVolume`         | Volume down     |
| `MODKEY+ALT+m` / `XF86AudioMute`                   | Volume toggle   |
| `XF86MonBrightnessUp`                              | Brightness up   |
| `XF86MonBrightnessDown`                            | Brightness down |
| `MODKEY+SHIFT+Up` / `SHIFT+XF86AudioRaiseVolume`   | Mic up          |
| `MODKEY+SHIFT+Down` / `SHIFT+XF86AudioLowerVolume` | Mic down        |
| `MODKEY+SHIFT+m` / `XF86AudioMicMute`              | Mic toggle      |

The popup's size, position, and timeout are `#define`s at the top of
`osd.c` (`OSD_W`/`OSD_WIN_H`/`OSD_MARGIN_BOTTOM`/`OSD_TIMEOUT_MS`) rather
than `config.h` values — edit those directly and rebuild if you want a
different size/position/duration.

Can also be triggered outside a keybind via the FIFO: `echo "osd 2" >
/tmp/dwm.fifo` fires `osds[2]` (`OsdVolToggle` per the enum above).

## Notifications

```c
const int notifblockidx = 14; /* index into statusblocks[] of the
                                * notification bell/count -- see
                                * "Status bar blocks" above */
```

dwm is a complete `org.freedesktop.Notifications` DBus server on its own —
`notify-send`, browser notifications, Discord, etc. all reach dwm directly,
with no dunst/mako/other daemon needed (and none should be autostarted
alongside dwm — see "Autostart" below). Popups appear in the top-right
corner of the focused monitor and stack as more arrive.

- **Urgency & timeout.** Low urgency auto-dismisses after ~4s, normal
  after ~6s, critical never auto-dismisses (the sending app is expected to
  close it once whatever needed attention is resolved) — unless the
  sender explicitly requested a different timeout, which is always
  honored.
- **Clicking a popup.** Left-click invokes the notification's default
  action (if the sending app provided one) and dismisses it; any other
  button just dismisses it.
- **The bar indicator.** `notifblockidx` above points at the
  `statusblocks[]` row (see "Status bar blocks") that shows a bell icon
  and unread count — that row's own `cmd` is intentionally empty, since
  `notifications.c` pushes the text directly rather than running a shell
  command. Clicking it:

  | Button | Action                     |
  | ------ | -------------------------- |
  | Left   | Dismiss all visible popups |
  | Middle | Clear notification history |
  | Right  | Toggle Do Not Disturb      |

  Set `notifblockidx = -1` to drop the bar indicator entirely — popups,
  history, and DND all keep working either way, you just lose the at-a-
  glance bar summary.

- **Do Not Disturb.** While enabled, incoming notifications are still
  recorded to history but no popup is shown. Toggle via the bar
  indicator (right-click), `MODKEY+ALT+n`, or `echo "notifdnd" >
/tmp/dwm.fifo`.
- **History.** The most recent 25 notifications are kept in memory
  (regardless of DND) and can be dumped via the FIFO — see "FIFO
  commands" below.

| Key              | Action                     |
| ---------------- | -------------------------- |
| `MODKEY+SHIFT+n` | Dismiss all visible popups |
| `MODKEY+ALT+n`   | Toggle Do Not Disturb      |

The popup pool size, dimensions, spacing, and per-urgency timeouts are
`#define`s at the top of `notifications.c` (`NOTIF_MAX_POPUPS`/`NOTIF_W`/
`NOTIF_MARGIN`/`NOTIF_GAP`/`NOTIF_TIMEOUT_LOW_MS`/`NOTIF_TIMEOUT_NORMAL_MS`)
rather than `config.h` values, same reasoning as the OSD's own `#define`s
above — edit those directly and rebuild for a different look.

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
override, use dwm's default behavior." `config.h`'s actual `rules[]`
spells out all 15 fields on every row instead of relying on that
zero-fill, purely for readability (every knob visible at a glance,
aligned in columns) — omitting trailing fields, as in the shortened
examples above, still compiles and behaves identically.

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
| `MODKEY+SHIFT+n` | Dismiss all notification popups          |
| `MODKEY+ALT+n`   | Toggle notification Do Not Disturb       |

**Apps/launchers** — `MODKEY+Return` (terminal), `+f` (file manager),
`+b` (browser), `+r` (dmenu), plus dedicated launchers for emoji picker,
power menu, screen recorder, etc. — see `keys[]` for the full list, these
mostly call out to small wrapper scripts (`sysctl`, `mediactl`, etc.) rather
than calling apps directly.

**Media/volume/mic/brightness** — XF86 keys are bound where the hardware
sends them; `MODKEY+ALT`/`MODKEY+SHIFT` combos are provided as a fallback
for keyboards without media keys. These all trigger the on-screen-display
popup rather than a plain `spawn` — see "On-screen display (OSD)" above
for the full key table. The status bar itself is also interactive for
these — see the "Status text" row in Mouse bindings below.

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

## Clipboard

| Key                          | Action                                    |
| ----------------------------- | ------------------------------------------ |
| `MODKEY+SHIFT+c`             | Open clipboard history in dmenu           |
| `MODKEY+CTRL+c`               | Pin/unpin the most recently copied entry  |
| `MODKEY+SHIFT+CTRL+c`         | Clear unpinned history (pinned entries kept) |

dwm watches the `CLIPBOARD` selection itself (via the XFixes extension)
and keeps a history of up to 200 unpinned + 100 pinned entries, persisted
to `~/.cache/dwm/clipboard_history` so it survives a restart. Picking an
entry runs it through `xclip` the same way screenshots/colorpicker copy
their output — dwm never becomes the clipboard's long-term owner itself,
it only watches and, on pick, hands text off to `xclip`.

Pinning always acts on whatever you *most recently copied* (whether it's
already pinned or not) rather than needing you to locate it in the
picker first — copy something, then `MODKEY+CTRL+c` immediately if you
want to keep it around past the unpinned cap.

This replaces the older `"clip daemon"` + `clip select` script pair —
see "Autostart" below.

## Mouse bindings

| Click target  | Button                           | Action                                                                                   |
| ------------- | -------------------------------- | ---------------------------------------------------------------------------------------- |
| Layout symbol | Left                             | Cycle to tile layout                                                                     |
| Layout symbol | Right                            | Cycle to monocle                                                                         |
| Window title  | Left                             | Toggle window (scratchpad-style)                                                         |
| Window title  | Middle                           | Zoom                                                                                     |
| Status text   | Left/Middle/Right/Scroll up/down | Reruns whichever block is under the cursor, via `sigstatusbar` → `statusbar_handleclick` |
| Client window | `MODKEY`+Left                    | Move (drag)                                                                              |
| Client window | `MODKEY`+Middle                  | Toggle floating                                                                          |
| Client window | `MODKEY`+Right                   | Resize (drag)                                                                            |
| Tag bar       | Left/Right                       | View / toggle-view tag                                                                   |
| Tag bar       | `MODKEY`+Left/Middle             | Tag / toggle-tag window                                                                  |

Unlike the other rows, "Status text" isn't one fixed action — dwm only
identifies _which block_ was clicked (by the delimiter byte embedded
after that segment of the status string) and reruns that block's
`statusblocks[]` command with `$BLOCK_BUTTON` set to the button number.
What actually happens is entirely up to that command — see "Status bar
blocks" above for the current block list. Clicks that land outside any
block's region are ignored (`statussig` stays `0`, which is never a valid
block index — see `DOCS.md` for why).

## FIFO commands (IPC)

dwm reads commands from `/tmp/dwm.fifo` (path set via `fifopath`) on every
event loop tick (~10ms latency). Send a command by writing a line to it:

```sh
echo "<command> [arg]" > /tmp/dwm.fifo
```

| Command             | Arg              | Effect                                        |
| ------------------- | ---------------- | --------------------------------------------- |
| `view`              | 0–4              | Switch to tag                                 |
| `tag`               | 0–4              | Move window to tag                            |
| `toggleview`        | 0–4              | Add/remove tag from current view              |
| `toggletag`         | 0–4              | Toggle tag on focused window                  |
| `setmfact`          | float e.g. `0.6` | Set master area size                          |
| `incnmaster`        | `1` or `-1`      | Add/remove master client                      |
| `cyclelayout`       | `1` or `-1`      | Cycle to next/prev layout                     |
| `zoom`              | —                | Swap focused window with master               |
| `togglefloating`    | —                | Toggle float on focused window                |
| `togglefullscreen`  | —                | Toggle fullscreen                             |
| `focusstackvis`     | `1` or `-1`      | Focus next/prev visible window                |
| `focusmon`          | `1` or `-1`      | Focus next/prev monitor                       |
| `tagmon`            | `1` or `-1`      | Send window to next/prev monitor              |
| `switchcol`         | —                | Focus first window in the other column        |
| `show`              | —                | Show focused window                           |
| `hide`              | —                | Hide focused window                           |
| `showall`           | —                | Show all hidden windows                       |
| `togglewin`         | —                | Toggle hide/show on the focused window        |
| `killclient`        | —                | Close focused window                          |
| `togglescratch`     | 0–7              | Toggle scratchpad by index                    |
| `hideallscratch`    | —                | Hide every visible scratchpad                 |
| `togglebar`         | —                | Show/hide bar                                 |
| `nextwallpaper`     | —                | Load new random wallpaper                     |
| `screenshot`        | 0–3              | Capture full/monitor/window/select            |
| `colorpicker`       | —                | Pick a color under cursor                     |
| `clippick`          | —                | Open the clipboard history picker (dmenu)     |
| `clippin`           | —                | Pin/unpin the most recently copied entry      |
| `clipclear`         | —                | Clear unpinned clipboard history              |
| `statusblock`       | 0–N, or `-1`     | Rerun one status bar block, or all            |
| `osd`               | 0–N              | Trigger an OSD popup by `osds[]` index        |
| `notifdnd`          | —                | Toggle notification Do Not Disturb            |
| `notifdismissall`   | —                | Dismiss all visible notification popups       |
| `notifclearhistory` | —                | Clear notification history                    |
| `notifhistory`      | —                | Write notification history to `fiforeplypath` |
| `state`             | —                | Write a state dump to `fiforeplypath`         |
| `quit`              | —                | Quit dwm (`1` = restart)                      |

Examples:

```sh
echo "view 2" > /tmp/dwm.fifo
echo "setmfact 0.65" > /tmp/dwm.fifo
echo "nextwallpaper" > /tmp/dwm.fifo
echo "togglescratch 3" > /tmp/dwm.fifo
echo "statusblock -1" > /tmp/dwm.fifo   # rerun every status bar block
echo "osd 0" > /tmp/dwm.fifo            # OsdVolUp, per the enum in config.h
echo "notifdnd" > /tmp/dwm.fifo         # toggle Do Not Disturb
echo "clippick" > /tmp/dwm.fifo         # open the clipboard history picker
```

This is intended for scripting — bind it to acpi events, a rofi menu,
a hardware button, or any external trigger that shouldn't need its own
X11 keybind. See `DOCS.md` → "Adding a new FIFO command" to extend the
table.

Note: `screenshot 3` (select), `colorpicker`, and `clippick` block dwm's
event loop until the interaction completes, same as a mouse-drag resize
would — don't trigger them from something expecting an instant return.

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

`notifhistory` uses the same `fiforeplypath` mechanism and the same
caveat applies — read it right after sending the command:

```sh
echo "notifhistory" > /tmp/dwm.fifo
cat /tmp/dwm.fifo.reply
# [14:32:07] Firefox: Download complete - report.pdf finished downloading
# [14:29:51] Signal: Jane Doe - Are we still on for 3?
```

## Autostart

`autostart.sh` runs once at dwm startup (called from `main()` before the
event loop starts). Put any background processes you want running every
session in there (compositor, etc.) rather than in `.xinitrc`, so they're
tied to dwm's lifecycle.

`dwmblocks` is no longer in `PROCS` — the status bar builds its own
content in-process now (see "Status bar blocks" above), there's nothing
external left to autostart for it. Likewise, `dunst` is no longer in
`PROCS` — dwm is its own `org.freedesktop.Notifications` server now (see
"Notifications" above). Don't autostart a separate notification daemon
alongside this build: whichever one starts first wins the DBus name, and
the other silently does nothing.

The old `"clip daemon"` entry is gone from `DAEMON_PROCS` for the same
reason — dwm watches the clipboard itself now (see "Clipboard" above).
Don't autostart a separate clipboard manager alongside this build; it'll
just fight dwm for ownership of whatever you copy.
