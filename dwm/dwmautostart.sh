#!/bin/env bash

# Vars
XRANDR_OUTPUT=$(xrandr -q)
PROCS=(
    clipmenud
    flameshot
    nm-applet
    dunst
    lxpolkit
    udiskie
    slstatus
)
FLAT_PROCS=(
    org.fkoehler.KTailctl
)

ARGS=(
    timexwalr ~/Pictures/Wallpapers 900
)

ARG_PROCS=(
    xwall
)

# Functions
command_exists() {
    command -v "$1" >/dev/null 2>&1
}

StartProc() {
    for program in "${PROCS[@]}"; do
        # Check if the program is already running
        if ! pgrep -x "$program" > /dev/null; then
            # If it's not running, start it in the background
            "$program" &
        fi
    done
}

ArgStart() {
    for program in "${ARG_PROCS[@]}"; do
        if ! pgrep -x "$program" > /dev/null; then
            # Start the program with arguments in the background
            "$program" "${ARGS[@]}" &
        fi
    done
}

StartFlat() {
    # Iterate over each app in the provided array of Flatpak app IDs
    for app_id in "${FLAT_PROCS[@]}"; do
        # Check if the Flatpak app is running
        if flatpak ps | grep -q "$app_id"; then
            echo "$app_id is already running."
        else
            echo "$app_id is not running. Starting it now..."

            # Try to launch the Flatpak application
            flatpak run "$app_id" &
        fi
    done
}

# Main
xset s off
xset s noblank
xset -dpms
xsetroot -cursor_name left_ptr
dbus-update-activation-environment --systemd --all

if command_exists nvidia-settings; then
    # Configure NVIDIA settings
    nvidia-settings --assign CurrentMetaMode="nvidia-auto-select +0+0 {ForceFullCompositionPipeline=On}"
fi

if echo "$XRANDR_OUTPUT" | grep -q "HDMI-0 connected"; then
    xrandr \
        --output HDMI-0 --mode 1920x1080 --rate 75.00 --primary \
        --output eDP-1-1 --mode 1920x1080 --rate 120.00 --right-of HDMI-0
else
    xrandr --auto
fi

picom -b
[[ -f "$HOME/.Xresources" ]] && xrdb -load "$HOME/.Xresources"
systemctl --user import-environment DISPLAY
StartProc
ArgStart
StartFlat

exec dwm
