# DOCS — dmenu Code Layout & Internals

## File overview

| File                | Purpose                                                               |
| ------------------- | --------------------------------------------------------------------- |
| `dmenu.c`           | Main binary: input loop, item matching, rendering, key/mouse handling |
| `drw.c` / `drw.h`   | Drawing primitives — same drw library shared with dwm/st              |
| `util.c` / `util.h` | `die()`, `ecalloc()`, `LENGTH()` macro                                |
| `stest.c`           | Standalone utility: filter a list of files by existence/mode          |
| `dmenu_run`         | Shell wrapper: `dmenu_path                                            | dmenu | sh` |
| `dmenu_path`        | Shell script: builds a cached list of executables in `$PATH`          |
| `config.def.h`      | Upstream default config — do not edit                                 |
| `config.h`          | Your config — compiled into the binary                                |
| `config.mk`         | Build flags and install prefix                                        |

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
  ├─ readflatpak()     — append installed flatpak apps to items[] (Enter on
  │                       one runs `flatpak run` instead of just printing it)
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

`match()` checks `fuzzy` first and returns immediately after calling
`fuzzymatch()` when it's on — `use_prefix` is never even read in that case,
so it has zero effect while fuzzy matching is active. It only comes into
play in the non-fuzzy branch below.

**Fuzzy mode** (default when `fuzzy = 1`; `-F` disables it): all characters
of the input must appear in the item in order, but don't need to be
contiguous. `"fxbr"` would match `"foxybrown"`. Items are scored by a
distance formula (`log(sidx + 2) + (eidx - sidx - text_len)`) that rewards
matches starting earlier and being more contiguous — this naturally tends
to rank prefix-like matches first, but it's an unconditional property of
the scoring formula, not something `use_prefix` controls.

**Non-fuzzy mode** (only reachable via `-F`): items are ranked exact match,
then prefix match (starts with the first whitespace-separated token), then
— only if `use_prefix = 0` (`-x` flips the default) — plain substring match
anywhere in the item. With `use_prefix = 1` (the default), non-prefix
substring matches aren't just ranked lower, they're excluded entirely.

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

## Flatpak integration

`readflatpak()` runs after `readstdin()` in the default (non-`-f`) path,
appending installed flatpak apps to whatever `readstdin()` already
collected — so `dmenu_path | dmenu` shows `$PATH` executables and flatpak
apps in the same list. Items sourced this way carry a `flatpak_id`; on
`Enter`, if the selected item has one set, dmenu runs `flatpak run
<flatpak_id>` instead of just printing the selection to stdout. Not run in
the `-f` (fast) path, since that flag exists specifically to grab the
keyboard before doing any potentially slow work, and spawning `flatpak
list` would work against that.
