#!/usr/bin/env python3
import signal
import subprocess
import sys
import threading
import time

running = True
lock = threading.Lock()

# ==== MODULE CONFIG ====
MODULES = [
    {
        "name": "ismedia",
        "enabled": True,
        "cmds": {"left": "sysstats ismedia"},
        "signal_offsets": {"left": 1},
    },
    {
        "name": "browser",
        "enabled": True,
        "cmds": {"left": "browserctrl -bst 20"},
        "signal_offsets": {"left": 2},
    },
    {
        "name": "song",
        "enabled": True,
        "cmds": {"left": "songctrl -sst SubsonicTUI 20"},
        "signal_offsets": {"left": 3},
    },
    {
        "name": "vol",
        "enabled": True,
        "cmds": {
            "left": "sysstats volume",
            "middle": "pavucontrol",
            "right": "pactl set-sink-mute @DEFAULT_SINK@ toggle",
            "scroll_up": "sysstats volume +5",
            "scroll_down": "sysstats volume -5",
        },
        "signal_offsets": {
            "left": 4,
            "middle": 5,
            "right": 6,
            "scroll_up": 7,
            "scroll_down": 8,
        },
    },
    {
        "name": "bright",
        "enabled": True,
        "cmds": {
            "left": "sysstats brightness",
            "scroll_up": "sysstats brightness +5",
            "scroll_down": "sysstats brightness -5",
        },
        "signal_offsets": {
            "left": 9,
            "scroll_up": 10,
            "scroll_down": 11,
        },
    },
    {
        "name": "bat",
        "enabled": True,
        "cmds": {"left": "sysstats battery"},
        "signal_offsets": {"left": 12},
    },
    {
        "name": "datetime",
        "enabled": True,
        "cmds": {"left": "sysstats date_time"},
        "signal_offsets": {"left": 13},
    },
]

MAIN_INTERVAL = 0.2

# Shared data
data = {mod["name"]: "" for mod in MODULES}
events = {}
for mod in MODULES:
    for btn in mod.get("signal_offsets", {}):
        events[f"{mod['name']}_{btn}"] = threading.Event()


# ==== FUNCTIONS ====
def run(cmd):
    try:
        return subprocess.check_output(cmd, shell=True, text=True).strip()
    except subprocess.CalledProcessError:
        return ""


def updater(mod, btn):
    """Thread per module/button"""
    if not mod.get("enabled", True):
        return  # skip disabled modules

    name = mod["name"]
    cmd = mod["cmds"][btn]
    ev = events[f"{name}_{btn}"]

    # Run once at startup if left click (update display)
    if btn == "left":
        val = run(cmd)
        with lock:
            data[name] = val

    while running:
        ev.wait()
        if not running:
            break
        val = run(cmd)
        with lock:
            if btn == "left":
                data[name] = val
        ev.clear()


def handle_signal(signum, frame):
    """Map SIGRTMIN + offset -> module/button event"""
    offset = signum - signal.SIGRTMIN
    for mod in MODULES:
        if not mod.get("enabled", True):
            continue
        for btn, btn_offset in mod.get("signal_offsets", {}).items():
            if offset == btn_offset:
                events[f"{mod['name']}_{btn}"].set()


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
    for btn, offset in mod.get("signal_offsets", {}).items():
        signal.signal(signal.SIGRTMIN + offset, handle_signal)

signal.signal(signal.SIGINT, handle_exit)
signal.signal(signal.SIGTERM, handle_exit)

# ==== START THREADS ====
for mod in MODULES:
    if not mod.get("enabled", True):
        continue
    for btn in mod.get("signal_offsets", {}):
        t = threading.Thread(target=updater, args=(mod, btn))
        t.daemon = True
        t.start()

# ==== MAIN LOOP ====
while running:
    with lock:
        fields = []
        for mod in MODULES:
            if not mod.get("enabled", True):
                continue
            val = data[mod["name"]]
            if val:
                # append marker as hidden char (left click offset)
                offset = mod.get("signal_offsets", {}).get("left", 1)
                if offset == 0:
                    offset = 1
                fields.append(val + chr(offset))

        # Display string without control chars
        s_display = " || ".join([f[:-1] for f in fields])

    subprocess.run(f"xsetroot -name '{s_display}'", shell=True)

    if MAIN_INTERVAL > 0:
        time.sleep(MAIN_INTERVAL)
    else:
        signal.pause()
