#!/bin/bash

# List of suckless programs to install
programs=("dmenu" "slock" "st" "tabbed" "slstatus" "dwmblocks" "dwm")

for prog in "${programs[@]}"; do
    echo "Installing $prog..."
    if cd "$prog"; then
        sudo make clean install || { echo "❌ Failed to install $prog"; exit 1; }
        cd ..
    else
        echo "❌ Directory $prog not found!"
        exit 1
    fi
done
