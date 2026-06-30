# DOCS — st Code Layout & Internals

## File overview

| File | Purpose |
|---|---|
| `st.c` | Terminal emulation core: VT parsing, screen buffer, selection, PTY |
| `st.h` / `win.h` | Shared types and interfaces between `st.c` and `x.c` |
| `x.c` | X11 frontend: window, rendering, input, fonts, transparency, sixel display |
| `hb.c` / `hb.h` | HarfBuzz shaping layer (ligatures / complex script support) |
| `sixel.c` / `sixel.h` | Sixel graphics decoder |
| `sixel_hls.c` / `sixel_hls.h` | HLS color conversion helpers for sixel |
| `config.h` | All user configuration — compiled into the binary |
| `config.def.h` | Upstream default config — do not edit |
| `config.mk` | Build flags and install prefix |
| `st.info` | Terminfo entry for the `St` terminal type |

## Architecture

st is split into two layers:

**`st.c` — terminal layer**: manages the PTY (pseudo-terminal), parses
VT100/VT220/xterm escape sequences, maintains the screen cell buffer
(`Term.line[][]`), handles selection, clipboard, and the alternate screen.
Calls back into `x.c` via the `ttyread()` → `draw()` path.

**`x.c` — X11 layer**: owns the window, font rendering (Xft), color
management, compositing (transparency), input handling, and sixel image
display. Calls into `st.c` for key event → PTY writes.

This separation means the terminal logic is in principle portable — `x.c`
could be replaced with a different display backend without touching `st.c`.

## Rendering pipeline

```
ttyread() reads PTY output
  → term escape parser updates Term.line[][] cells
  → draw() is called
       → drawregion() iterates dirty cells
            → xdrawglyph() / xdrawglyphfontspecs()
                 → Xft draws each glyph with correct color/attr
       → sixel images are composited over the cell grid
  → XSync / swap
```

The **sync patch** (`minlatency`/`maxlatency`) adds a timer-based batching
step: instead of drawing on every byte from the PTY, st waits until the PTY
goes quiet (up to `maxlatency` ms) before drawing. This eliminates the
partial-frame flicker you see in stock st when a command rapidly scrolls
output.

## Transparency

`alpha` in `config.h` sets the background opacity. Implementation: `x.c`
creates the window with a 32-bit ARGB visual (if available), sets the
background color with the alpha channel premultiplied, and lets the
compositor (picom) blend it against whatever's behind the window. Without
a compositor running, transparency has no visible effect — the background
will show as a solid color.

## Sixel graphics

The `sixel` patch adds a DCS (`\033P`) escape handler in `st.c` that
routes sixel data to the decoder in `sixel.c`. Decoded pixel data is stored
as an `XImage` and composited over the cell grid in `x.c`'s draw path.
Sixel images are anchored to a terminal cell position and scroll with the
terminal content.

HarfBuzz (`hb.c`) is used for text shaping — required because the sixel
patch's changes to the font rendering path need HarfBuzz for correct glyph
cluster handling.

## Scrollback

The `scrollback` patch extends the terminal buffer beyond the visible rows.
`Shift+Page Up` / `Shift+Page Down` scroll through history without requiring
an external scroll program. The scroll buffer is in-memory only — it does
not persist across st restarts.
