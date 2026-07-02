# slock (custom build)

A personal fork of [slock](https://tools.suckless.org/slock/) — the suckless
screen locker. Locks the X display and requires your user password to unlock.
This build adds a background blur effect and a custom drawn logo, with a
Gruvbox color scheme for lock states.

## Feature summary

| Feature | Source |
|---|---|
| Background blur (Gaussian, configurable radius) | `blur` patch |
| Pixelation mode (alternative to blur, off by default) | `blur` patch |
| Custom drawn logo (rectangle grid, right-center aligned) | `dwmlogo` patch |
| Per-state color scheme (idle / input / failed) | custom Gruvbox colors |
| Privilege drop to `nobody:nobody` after locking | stock |
| `failonclear` — cleared input counts as failed attempt | stock |

## Requirements

- Xlib (`libx11-dev`)
- Xext (for blur — `libxext-dev`)
- PAM or shadow auth (depending on your system)

```sh
sudo apt install libx11-dev libxext-dev
```

## Building

```sh
make clean install
```

slock must be installed **setuid root** to be able to lock the display and
check passwords. The Makefile handles this. If the binary loses its setuid
bit after reinstall, run:

```sh
sudo chmod u+s /usr/local/bin/slock
```

## Usage

```sh
slock          # lock immediately
slock command  # run command, lock when it exits (e.g. slock suspend)
```

Bound in dwm to `MODKEY+SHIFT+l` via `SHCMD("slock")`.

## License

MIT/X11 — see `LICENSE`.
