#!/usr/bin/env python3
import signal
import subprocess
import sys
import threading
import time

running = True
lock = threading.Lock()
MAIN_INTERVAL = 0.0  # interval for modules without their own update_interval

# ==== MODULE CONFIG ====
MODULES = [
    {
        "name": "ismedia",
        "enabled": True,
        "default_update": "sysstats ismedia",
        "default_signal_offset": 20,
        "cmds": {"left": "dmenu_screen"},
        "signal_offsets": {"left": 1},
    },
    {
        "name": "browser",
        "enabled": False,
        "default_update": "browserctrl -bst 20",
        "default_signal_offset": 21,
        "cmds": {"left": "notify-send 'Browser' \"$(browserctrl -bst 45)\""},
        "signal_offsets": {"left": 2},
    },
    {
        "name": "song",
        "enabled": True,
        "default_update": "songctrl -sst SubsonicTUI 20",
        "default_signal_offset": 22,
        "cmds": {
            "left": "songctrl --previous",
            "right": "songctrl --skip",
        },
        "signal_offsets": {
            "left": 3,
            "right": 4,
        },
    },
    {
        "name": "vol",
        "enabled": True,
        "default_update": "sysstats volume",
        "default_signal_offset": 23,
        "cmds": {
            "left": "notify-send 'Audio Device' \"$(pactl info | grep 'Default Sink' | awk '{print $3}')\"",
            "middle": "volumectrl --togglemute",
            "right": "pavucontrol",
            "scroll_up": "volumectrl --inc 5",
            "scroll_down": "volumectrl --dec 5",
        },
        "signal_offsets": {
            "left": 5,
            "middle": 6,
            "right": 7,
            "scroll_up": 8,
            "scroll_down": 9,
        },
    },
    {
        "name": "bright",
        "enabled": True,
        "default_update": "sysstats brightness",
        "default_signal_offset": 24,
        "cmds": {
            "left": "notify-send 'Brightness' \"$(brightnessctl)\"",
            "scroll_up": "brightnessctrl --inc 5",
            "scroll_down": "brightnessctrl --dec 5",
        },
        "signal_offsets": {
            "left": 10,
            "scroll_up": 11,
            "scroll_down": 12,
        },
    },
    {
        "name": "bat",
        "enabled": True,
        "default_update": "sysstats battery",
        "update_interval": 30,  # update every 30s automatically
        "cmds": {"left": "notify-send 'Battery' \"$(acpi -b)\""},
        "signal_offsets": {"left": 13},
    },
    {
        "name": "datetime",
        "enabled": True,
        "default_update": "sysstats date_time",
        "update_interval": 60,  # update every 60s automatically
        "cmds": {"left": "notify-send 'Date & Time' \"$(date)\""},
        "signal_offsets": {"left": 14},
    },
]

# Shared data
data = {mod["name"]: "" for mod in MODULES}
events = {}
for mod in MODULES:
    for btn in mod.get("signal_offsets", {}):
        events[f"{mod['name']}_{btn}"] = threading.Event()
    if mod.get("enabled", True) and "default_signal_offset" in mod:
        events[f"{mod['name']}_default"] = threading.Event()


# ==== FUNCTIONS ====
def run(cmd):
    try:
        return subprocess.check_output(cmd, shell=True, text=True).strip()
    except subprocess.CalledProcessError:
        return ""


def updater(mod, btn):
    """Thread per module/button/default signal"""
    if not mod.get("enabled", True):
        return

    name = mod["name"]

    if btn == "default":
        cmd = mod["default_update"]
        ev = events[f"{name}_default"]
    else:
        cmd = mod["cmds"][btn]
        ev = events[f"{name}_{btn}"]

    while running:
        ev.wait()
        if not running:
            break
        val = run(cmd)
        with lock:
            if btn == "default" or btn == "left":
                data[name] = val
        ev.clear()


def periodic_updater(mod):
    """Thread for modules with their own update_interval"""
    interval = mod.get("update_interval", MAIN_INTERVAL)
    while running:
        val = run(mod["default_update"])
        with lock:
            data[mod["name"]] = val
        time.sleep(interval)


def handle_signal(signum, frame):
    offset = signum - signal.SIGRTMIN
    for mod in MODULES:
        if not mod.get("enabled", True):
            continue

        # Buttons
        for btn, btn_offset in mod.get("signal_offsets", {}).items():
            if offset == btn_offset:
                events[f"{mod['name']}_{btn}"].set()

        # Default signal
        if offset == mod.get("default_signal_offset"):
            events[f"{mod['name']}_default"].set()


def handle_exit(signum, frame):
    global running
    print("\nExiting cleanly...")
    running = False
    for e in events.values():
        e.set()
    subprocess.run(["xsetroot", "-name", ""])
    sys.exit(0)


# ==== SETUP SIGNALS ====
for mod in MODULES:
    if not mod.get("enabled", True):
        continue

    # Button signals
    for btn, offset in mod.get("signal_offsets", {}).items():
        signal.signal(signal.SIGRTMIN + offset, handle_signal)

    # Default signal
    if "default_signal_offset" in mod:
        signal.signal(signal.SIGRTMIN + mod["default_signal_offset"], handle_signal)

signal.signal(signal.SIGINT, handle_exit)
signal.signal(signal.SIGTERM, handle_exit)

# ==== START THREADS ====
for mod in MODULES:
    if not mod.get("enabled", True):
        continue

    # Button/default signal threads
    for btn in mod.get("signal_offsets", {}):
        t = threading.Thread(target=updater, args=(mod, btn))
        t.daemon = True
        t.start()
    if "default_signal_offset" in mod:
        t = threading.Thread(target=updater, args=(mod, "default"))
        t.daemon = True
        t.start()

    # Periodic interval thread
    if "update_interval" in mod:
        t = threading.Thread(target=periodic_updater, args=(mod,))
        t.daemon = True
        t.start()

# ==== MAIN LOOP: periodic default updates for modules without update_interval ====
while running:
    with lock:
        fields = []
        for mod in MODULES:
            if not mod.get("enabled", True):
                continue
            # Skip modules with their own update_interval
            if "update_interval" in mod:
                val = data[mod["name"]]
            else:
                val = run(mod.get("default_update"))
                data[mod["name"]] = val

            if val:
                fields.append(val)

        s_display = " || ".join(fields)

    subprocess.run(f"xsetroot -name '{s_display}'", shell=True)
    time.sleep(MAIN_INTERVAL)
