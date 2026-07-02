# st (custom build)

A personal fork of [st](https://st.suckless.org/) — the suckless simple
terminal. This build adds sixel graphics support, background transparency,
scrollback via keyboard, and a full Gruvbox 16-color palette.

## Feature summary

| Feature | Source |
|---|---|
| Sixel graphics (inline images in terminal) | `sixel` patch |
| Background transparency (`alpha`) | `alpha` patch |
| Keyboard scrollback (`Shift+PgUp/PgDn`) | `scrollback` patch |
| Sync patch (reduces tearing/flickering) | `sync` patch |
| Gruvbox 16-color palette | custom colors |
| CaskaydiaCove Nerd Font, size 12 | custom font |
| Background opacity `0.85` | custom alpha |
| Shell: `/bin/zsh` | custom shell |
| Bold is bright colors disabled | custom |
| Wide glyph support | built-in |

See **[DOCS.md](DOCS.md)** for code layout and **[WIKI.md](WIKI.md)** for
configuration reference.

## Requirements

- Xlib (`libx11-dev`)
- Xft + fontconfig (`libxft-dev`, `libfontconfig-dev`)
- HarfBuzz (`libharfbuzz-dev`) — required by the sixel/hb patches
- A compositor (picom) for transparency to take visual effect

```sh
sudo apt install libx11-dev libxft-dev libfontconfig-dev libharfbuzz-dev
```

## Building

```sh
make clean install
```

Also run `tic -sx st.info` once after install to register the `St` terminfo
entry, so applications correctly detect terminal capabilities (colors, sixel,
etc.):

```sh
tic -sx st.info
```

## Usage

```sh
st                    # open terminal with default shell
st -e htop            # run specific command
st -f "JetBrainsMono NF:size=14"  # override font
st -a 0.9             # override alpha (if xresources patch active)
```

In dwm, bound to `MODKEY+Return`.

## License

MIT/X11 — see `LICENSE`.
