# DWM Keybindings

## Table Legend

- **Modifier**: The modifier keys used (MODKEY, SHIFTKEY, ALTKEY, CTRLKEY)
- **Key**: The key pressed
- **Function**: The DWM function called
- **Argument**: Any argument passed to the function

---

## DWM Controls

| Modifier                     | Key   | Function       | Argument   |
| ---------------------------- | ----- | -------------- | ---------- |
| MODKEY \| SHIFTKEY           | b     | togglebar      | 0          |
| MODKEY                       | j     | focusstackvis  | +1         |
| MODKEY                       | k     | focusstackvis  | -1         |
| MODKEY \| SHIFTKEY           | j     | focusstackhid  | +1         |
| MODKEY \| SHIFTKEY           | k     | focusstackhid  | -1         |
| MODKEY                       | h     | setmfact       | -0.05      |
| MODKEY                       | l     | setmfact       | +0.05      |
| MODKEY \| ALTKEY             | j     | movestack      | +1         |
| MODKEY \| ALTKEY             | k     | movestack      | -1         |
| MODKEY \| SHIFTKEY           | a     | incnmaster     | +1         |
| MODKEY \| SHIFTKEY           | m     | incnmaster     | -1         |
| MODKEY \| SHIFTKEY           | z     | zoom           | 0          |
| MODKEY                       | Tab   | view           | 0          |
| MODKEY \| ALTKEY \| SHIFTKEY | t     | setlayout      | layouts[0] |
| MODKEY \| ALTKEY \| SHIFTKEY | f     | setlayout      | layouts[1] |
| MODKEY \| ALTKEY \| SHIFTKEY | m     | setlayout      | layouts[2] |
| MODKEY \| SHIFTKEY           | f     | fullscreen     | 0          |
| MODKEY \| SHIFTKEY           | Space | togglefloating | 0          |
| MODKEY                       | 0     | view           | ~0         |
| MODKEY \| SHIFTKEY           | 0     | tag            | ~0         |
| MODKEY                       | ,     | focusmon       | -1         |
| MODKEY                       | .     | focusmon       | +1         |
| MODKEY \| SHIFTKEY           | ,     | tagmon         | -1         |
| MODKEY \| SHIFTKEY           | .     | tagmon         | +1         |
| MODKEY                       | =     | show           | 0          |
| MODKEY \| SHIFTKEY           | =     | showall        | 0          |
| MODKEY                       | -     | hide           | 0          |

---

## System Controls

| Modifier           | Key | Function   | Argument |
| ------------------ | --- | ---------- | -------- |
| MODKEY \| SHIFTKEY | q   | quit       | 0        |
| MODKEY \| SHIFTKEY | r   | quit       | 1        |
| MODKEY \| SHIFTKEY | l   | spawn      | slock    |
| MODKEY             | q   | killclient | 0        |

---

## Dmenu Shortcuts

| Modifier           | Key | Function | Argument       |
| ------------------ | --- | -------- | -------------- |
| MODKEY             | r   | spawn    | dmenu_run      |
| ALTKEY             | Tab | spawn    | dmenu_alttab   |
| MODKEY             | p   | spawn    | dmenu_flatpak  |
| MODKEY \| SHIFTKEY | c   | spawn    | dmenu_clip     |
| MODKEY \| SHIFTKEY | e   | spawn    | dmenu_emoji    |
| MODKEY \| ALTKEY   | e   | spawn    | dmenu_nerdfont |
| MODKEY \| SHIFTKEY | p   | spawn    | dmenu_power    |
| MODKEY             | n   | spawn    | dmenu_notebook |
| MODKEY \| ALTKEY   | r   | spawn    | dmenu_screen   |
| MODKEY \| ALTKEY   | m   | spawn    | dmenu_menu     |

---

## System Functions

| Modifier | Key                      | Function | Argument               |
| -------- | ------------------------ | -------- | ---------------------- |
| 0        | XF86XK_MonBrightnessUp   | spawn    | brightnessctrl --inc 5 |
| 0        | XF86XK_MonBrightnessDown | spawn    | brightnessctrl --dec 5 |

---

## Media Controls

| Modifier           | Key                     | Function | Argument                |
| ------------------ | ----------------------- | -------- | ----------------------- |
| MODKEY             | Up                      | spawn    | volumectrl --inc 5      |
| MODKEY             | Down                    | spawn    | volumectrl --dec 5      |
| MODKEY \| SHIFTKEY | Down                    | spawn    | volumectrl --togglemute |
| 0                  | XF86XK_AudioRaiseVolume | spawn    | volumectrl --inc 5      |
| 0                  | XF86XK_AudioLowerVolume | spawn    | volumectrl --dec 5      |
| 0                  | XF86XK_AudioMute        | spawn    | volumectrl --togglemute |

---

## Music Player Controls

| Modifier | Key              | Function | Argument               |
| -------- | ---------------- | -------- | ---------------------- |
| MODKEY   | Right            | spawn    | songctrl --skip        |
| MODKEY   | Left             | spawn    | songctrl --previous    |
| MODKEY   | Pause            | spawn    | songctrl --togglepause |
| MODKEY   | s                | spawn    | songctrl --togglepause |
| 0        | XF86XK_AudioNext | spawn    | songctrl --skip        |
| 0        | XF86XK_AudioPrev | spawn    | songctrl --previous    |
| 0        | XF86XK_AudioPlay | spawn    | songctrl --togglepause |

---

## Browser Media Controls

| Modifier         | Key              | Function | Argument                  |
| ---------------- | ---------------- | -------- | ------------------------- |
| MODKEY \| ALTKEY | Right            | spawn    | browserctrl --skip        |
| MODKEY \| ALTKEY | Left             | spawn    | browserctrl --previous    |
| MODKEY \| ALTKEY | Pause            | spawn    | browserctrl --togglepause |
| MODKEY \| ALTKEY | s                | spawn    | browserctrl --togglepause |
| ALTKEY           | XF86XK_AudioNext | spawn    | browserctrl --skip        |
| ALTKEY           | XF86XK_AudioPrev | spawn    | browserctrl --previous    |
| ALTKEY           | XF86XK_AudioPlay | spawn    | browserctrl --togglepause |

---

## Applications

| Modifier           | Key    | Function | Argument                                                                       |
| ------------------ | ------ | -------- | ------------------------------------------------------------------------------ |
| MODKEY             | Return | spawn    | st                                                                             |
| MODKEY             | f      | spawn    | thunar                                                                         |
| MODKEY             | b      | spawn    | qutebrowser                                                                    |
| MODKEY             | e      | spawn    | neovide                                                                        |
| MODKEY \| SHIFTKEY | d      | spawn    | flatpak run dev.vencord.Vesktop                                                |
| MODKEY \| SHIFTKEY | g      | spawn    | signal-desktop                                                                 |
| MODKEY             | i      | spawn    | `st -c keymaps -n Keymaps -e zsh -c moar $HOME/Github/suckless/dwm/keymaps.md` |

---

## Screenshot

| Modifier           | Key   | Function | Argument             |
| ------------------ | ----- | -------- | -------------------- |
| 0                  | Print | spawn    | sstool --select      |
| MODKEY             | Print | spawn    | sstool --screen      |
| MODKEY \| SHIFTKEY | Print | spawn    | sstool --fullscreen  |
| MODKEY \| CTRLKEY  | Print | spawn    | sstool --window      |
| MODKEY \| ALTKEY   | Print | spawn    | sstool --colorpicker |

---

## Wallpaper

| Modifier           | Key | Function | Argument                              |
| ------------------ | --- | -------- | ------------------------------------- |
| MODKEY \| SHIFTKEY | w   | spawn    | xwall themewall ~/Pictures/Wallpapers |

---

## Tags

| Key  | Tag |
| ---- | --- |
| XK_1 | 0   |
| XK_2 | 1   |
| XK_3 | 2   |
| XK_4 | 3   |
| XK_5 | 4   |

---

## Scratch Pads

| Modifier | Key | Function      | Argument | Command                                                     |
| -------- | --- | ------------- | -------- | ----------------------------------------------------------- |
| MODKEY   | t   | togglescratch | 0        | `st -c termsc -n Termsc`                                    |
| MODKEY   | y   | togglescratch | 1        | `st -c lfsc -n Lfsc -e zsh -c lf`                           |
| MODKEY   | z   | togglescratch | 2        | `st -c qalsc -n Qalsc -e zsh -c qalc`                       |
| MODKEY   | a   | togglescratch | 3        | `st -c gurks -n Gurks -e zsh -c gurks`                      |
| MODKEY   | g   | togglescratch | 4        | `st -c pulsesc -n Pulsesc -e zsh -c pulsemixer`             |
| MODKEY   | d   | togglescratch | 5        | `st -c discordo -n Discordo -e zsh -c discodio`             |
| MODKEY   | c   | togglescratch | 6        | `st -c twitch-tui -n Twitch-tui -e zsh -c twt`              |
| MODKEY   | m   | togglescratch | 7        | `st -c subsonic-tui -n Subsonic-TUI -e zsh -c subsonic-tui` |
