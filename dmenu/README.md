# dmenu (custom build)

A personal fork of [dmenu](https://tools.suckless.org/dmenu/) — the suckless
dynamic menu — with fuzzy matching, transparency, prefix matching, and a
custom Gruvbox color scheme with per-scheme highlight colors.

## Feature summary

| Feature | Source |
|---|---|
| Fuzzy matching with toggle | `fuzzymatch` patch |
| Per-scheme highlight colors (`SchemeSelHighlight`, `SchemeNormHighlight`) | `highlight` patch |
| Background transparency (per-scheme alpha) | `alpha` patch |
| Prefix matching mode (toggleable with `-x`) | `prefix` patch |
| Custom bar height padding (`user_bh`) | `lineheight` patch |
| Preselected item on open (`-n`) | `preselect` patch |
| Custom prompt text (`-p`) | stock |

See **[DOCS.md](DOCS.md)** for code layout and **[WIKI.md](WIKI.md)** for
configuration reference.

## Requirements

- Xlib headers (`libx11-dev`)
- Xft + fontconfig (`libxft-dev`, `libfontconfig-dev`)

```sh
sudo apt install libx11-dev libxft-dev libfontconfig-dev
```

## Building

```sh
make clean install
```

Installs `dmenu`, `dmenu_run`, `dmenu_path`, and `stest` to the prefix in
`config.mk` (`/usr/local` by default).

## Usage

```sh
# Basic launcher
dmenu_run

# Pipe arbitrary input
echo -e "option1\noption2\noption3" | dmenu

# Vertical list, 10 lines
some_command | dmenu -l 10

# Disable fuzzy matching for this invocation
some_command | dmenu -F

# Preselect item at index 2
some_command | dmenu -n 2

# Use prefix matching mode
some_command | dmenu -x
```

## License

MIT/X11 — see `LICENSE`.
