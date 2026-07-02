# DOCS — dmenu Code Layout & Internals

## File overview

| File | Purpose |
|---|---|
| `dmenu.c` | Main binary: input loop, item matching, rendering, key/mouse handling |
| `drw.c` / `drw.h` | Drawing primitives — same drw library shared with dwm/st |
| `util.c` / `util.h` | `die()`, `ecalloc()`, `LENGTH()` macro |
| `stest.c` | Standalone utility: filter a list of files by existence/mode |
| `dmenu_run` | Shell wrapper: `dmenu_path | dmenu | sh` |
| `dmenu_path` | Shell script: builds a cached list of executables in `$PATH` |
| `config.def.h` | Upstream default config — do not edit |
| `config.h` | Your config — compiled into the binary |
| `config.mk` | Build flags and install prefix |

## Program flow

```
main()
  ├─ parse args (-b, -F, -l, -n, -p, -x, ...)
  ├─ setup()
  │    ├─ XOpenDisplay / XineramaQueryScreens
  │    ├─ drw_create / drw_fontset_create
  │    ├─ grab keyboard (busy-wait up to 1000ms for WM to release)
  │    └─ create dmenu window + XMapRaised
  ├─ readstdin()       — read all items from stdin into items[]
  ├─ match()           — initial filter pass (empty input = show all)
  └─ run()             — X event loop
       ├─ keypress()   — typing updates input → match() → drawmenu()
       ├─ mouse click  — select item or scroll
       └─ on Enter/Escape: print result or exit
```

dmenu grabs the keyboard immediately on startup, blocking all other input
until it exits — this is intentional and is why it "feels" instant even
though it's a separate process.

## Matching

Two modes, toggled by the `-F` flag or the `fuzzy` config default:

**Prefix mode** (default with `-x` or `use_prefix = 1`): items must start
with the input string. Fast, simple, good for known-prefix completions like
command names.

**Fuzzy mode** (default when `fuzzy = 1`): all characters of the input must
appear in the item in order, but don't need to be contiguous. `"fxbr"` would
match `"foxybrown"`. Items are scored and sorted by match quality — exact
prefix matches rank first, then contiguous matches, then scattered.

Highlighting (`SchemeSelHighlight`, `SchemeNormHighlight`) marks the matched
characters in the rendered item text using `drw_text()` with per-character
color switching.

## Transparency

The `alpha` and `alphas[]` config values control per-scheme background
opacity via the X composite extension. `0xff` = fully opaque, `0x00` = fully
transparent. Foreground is always `OPAQUE` in this build — only backgrounds
are blended. Requires a running compositor (picom) to take visible effect.

## Vertical list mode

`-l N` switches from the default single-line mode to a vertical list with N
visible rows. The input field is still at the top; items scroll inside the
list area. The `lines` config default sets this without needing a flag.
