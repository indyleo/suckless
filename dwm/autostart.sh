#!/usr/bin/env bash

# Vars
PROCS=(
    dunst
    lxqt-policykit-agent
    udiskie
    medianotify
    dwmblocks
)
DAEMON_PROCS=(
    "clip daemon"
    "organizer.py --daemon"
    "python3 -m http.server 8080 --bind 127.0.0.1 --directory ~/.config/startpage"
)

# Functions
command_exists() {
    command -v "$1" >/dev/null 2>&1
}

# Conditional additions
if command_exists sunshine; then
    PROCS+=("sunshine")
fi
if command_exists solaar; then
    DAEMON_PROCS+=("solaar --window hide")
fi

start_processes() {
    for program in "${PROCS[@]}"; do
        if ! pidof -x "$program" > /dev/null 2>&1; then
            "$program" &
            sleep 0.1
        fi
    done
}

start_daemons() {
    for cmd in "${DAEMON_PROCS[@]}"; do
        program=$(echo "$cmd" | awk '{print $1}')
        if ! pgrep -f "$program" > /dev/null; then
            $cmd &
            sleep 0.1
        fi
    done
}

# Main execution
xset s off -dpms
xset s noblank
xsetroot -cursor_name left_ptr
dbus-update-activation-environment --systemd --all

# NVIDIA settings
if command_exists nvidia-settings; then
    nvidia-settings --assign CurrentMetaMode="nvidia-auto-select +0+0 {ForceFullCompositionPipeline=On}"
fi

# Start compositor
picom -b

# Load X resources
[[ -f "$HOME/.Xresources" ]] && xrdb -load "$HOME/.Xresources"

# Import environment
systemctl --user import-environment DISPLAY

# Start all processes
start_processes
start_daemons
