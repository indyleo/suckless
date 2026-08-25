# dwm: native clipboard history + central theme

Two new features, built and verified against your uploaded dwm source
(full clean build + link tested; patches verified to `patch -p1` apply
onto a fresh checkout and rebuild independently).

## What's here

- `clipboard.c` / `clipboard.h` — new module: XFixes-based clipboard
  history with pinning, persisted to `~/.cache/dwm/clipboard_history`,
  picked via `dmenu`, written back via `xclip` (same mechanism your
  `screenshot.c` already uses). Replaces the external `"clip daemon"` +
  `clip select` script pair.
- `theme.h` — new header: the Gruvbox Dark palette as `CAL0`..`CAL15`,
  matching your `qs/Theme.qml` naming, plus dwm-specific accents. One
  file to edit to re-theme dwm.
- `patches/*.patch` — unified diffs (`patch -p1` format) against every
  existing file that needed a wire-up: `config.def.h`, `config.h`,
  `dwm.c`, `ipc.c`, `Makefile`, `config.mk`, `autostart.sh`.

## Installing

From your dwm source root:

```sh
cp clipboard.c clipboard.h theme.h .
for p in patches/*.patch; do patch -p1 < "$p"; done
make clean && make
sudo make install
```

Then edit `autostart.sh` on your actual machine to stop whatever binary
your old `"clip daemon"` was (kill it once, or just reboot into the new
dwm — the patch already removes it from the process list dwm manages).

## New keybinds (config.h)

| Bind | Action |
|---|---|
| `MODKEY+SHIFT+c` | Open clipboard history in dmenu (was `clip select`) |
| `MODKEY+CTRL+c` | Pin/unpin the most recently copied entry |
| `MODKEY+SHIFT+CTRL+c` | Clear unpinned history (pinned entries kept) |

Also exposed as FIFO commands if you want to trigger them from a script:
`echo clippick > /tmp/dwm.fifo`, `clippin`, `clipclear`.

## Design notes / limitations

- **Text only.** No image/binary clipboard entries — matches what your
  `xclip`-based write path already assumes. If you regularly copy
  images and want those in history too, that's a bigger addition
  (would need Imlib2 involved the way `screenshot.c` uses it for
  reading, not just writing) — happy to add it if it'd help.
- **No ICCCM selection-owner protocol in dwm itself.** dwm only ever
  *watches* the clipboard (via XFixes) and *writes* it by handing text
  to `xclip`, exactly like `screenshot.c` does today. This sidesteps a
  lot of X11 selection-serving complexity at the cost of dwm not being
  the "clipboard manager of record" in the strict X11 sense — in
  practice this is invisible day to day.
- **Preview truncation is byte-based, not UTF-8-aware.** A multi-byte
  character can render oddly at the tail of a long dmenu preview line.
  Cosmetic only — the full original text is always what gets copied.
- **`theme.h` changes zero pixels.** It's a pure refactor: every hex
  value in `config.def.h`/`config.h` is unchanged, just routed through
  named constants instead of being typed twice.
- The `dmenu` invocation in `clippick()` uses plain `-i -l 20 -p
  clipboard:` flags. If your `dmenu_run` wrapper passes styling flags
  (`-nb`/`-nf`/`-fn`/etc.) you'll want to copy those into the `dmenuargv`
  array at the top of `clippick()` in `clipboard.c` so the picker
  matches your other dmenu-driven scripts.
