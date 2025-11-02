#!/usr/bin/env python3
import signal
import subprocess
import sys
import threading
import time

running = True
lock = threading.Lock()
MAIN_INTERVAL = 0.0  # Update display every second
needs_refresh = threading.Event()

# ==== MODULE CONFIG ====
MODULES = [
    {
        "name": "chat",
        "enabled": False,
        "default_update": "tail -n 1 ~/.cache/twitch_chat.log 2>/dev/null || echo 'No chat yet'",
        "signal_offset": 19,
    },
    {
        "name": "ismedia",
        "enabled": True,
        "default_update": "sysstats ismedia",
        "signal_offset": 20,
    },
    {
        "name": "browser",
        "enabled": False,
        "default_update": "browserctrl -bst 20",
        "signal_offset": 21,
    },
    {
        "name": "song",
        "enabled": True,
        "default_update": "songctrl -sst SubsonicTUI 20",
        "signal_offset": 22,
    },
    {
        "name": "vol",
        "enabled": True,
        "default_update": "sysstats volume",
        "signal_offset": 23,
    },
    {
        "name": "bright",
        "enabled": True,
        "default_update": "sysstats brightness",
        "signal_offset": 24,
    },
    {
        "name": "bat",
        "enabled": True,
        "default_update": "sysstats battery",
        "update_interval": 30,
    },
    {
        "name": "datetime",
        "enabled": True,
        "default_update": "sysstats date_time",
        "update_interval": 60,
    },
]

# Shared data
data = {mod["name"]: "" for mod in MODULES}
module_events = {
    mod["name"]: threading.Event() for mod in MODULES if mod.get("enabled", True)
}


# ==== FUNCTIONS ====
def run(cmd):
    try:
        return subprocess.check_output(
            cmd, shell=True, text=True, stderr=subprocess.DEVNULL
        ).strip()
    except Exception:
        return ""


def signal_updater(mod):
    """Thread that waits for signals to update a module"""
    name = mod["name"]
    ev = module_events[name]

    while running:
        ev.wait()
        if not running:
            break

        val = run(mod["default_update"])
        with lock:
            data[name] = val
        needs_refresh.set()
        ev.clear()


def periodic_updater(mod):
    """Thread for modules with their own update_interval"""
    interval = mod["update_interval"]
    while running:
        val = run(mod["default_update"])
        with lock:
            data[mod["name"]] = val
        needs_refresh.set()
        time.sleep(interval)


def update_display():
    """Update xsetroot with current data"""
    with lock:
        fields = [
            data[mod["name"]]
            for mod in MODULES
            if mod.get("enabled", True) and data[mod["name"]]
        ]
        s_display = " || ".join(fields)

    subprocess.run(["xsetroot", "-name", s_display], stderr=subprocess.DEVNULL)


def handle_signal(signum, frame):
    """Route signals to appropriate module"""
    offset = signum - signal.SIGRTMIN
    for mod in MODULES:
        if mod.get("enabled", True) and mod.get("signal_offset") == offset:
            module_events[mod["name"]].set()
            return


def handle_exit(signum, frame):
    global running
    print("\nExiting cleanly...")
    running = False
    for e in module_events.values():
        e.set()
    needs_refresh.set()
    subprocess.run(["xsetroot", "-name", ""], stderr=subprocess.DEVNULL)
    sys.exit(0)


# ==== SETUP SIGNALS ====
for mod in MODULES:
    if mod.get("enabled", True) and "signal_offset" in mod:
        signal.signal(signal.SIGRTMIN + mod["signal_offset"], handle_signal)

signal.signal(signal.SIGINT, handle_exit)
signal.signal(signal.SIGTERM, handle_exit)

# ==== START THREADS ====
for mod in MODULES:
    if not mod.get("enabled", True):
        continue

    # Signal updater thread (for all modules with signal_offset)
    if "signal_offset" in mod:
        t = threading.Thread(target=signal_updater, args=(mod,), daemon=True)
        t.start()

    # Periodic interval thread (only for modules with update_interval)
    if "update_interval" in mod:
        t = threading.Thread(target=periodic_updater, args=(mod,), daemon=True)
        t.start()

# ==== MAIN LOOP ====
# Initial update
for mod in MODULES:
    if mod.get("enabled", True) and "update_interval" not in mod:
        data[mod["name"]] = run(mod["default_update"])

update_display()

while running:
    # Wait for refresh signal or timeout
    needs_refresh.wait(timeout=MAIN_INTERVAL)
    needs_refresh.clear()

    if not running:
        break

    # Update modules without update_interval
    for mod in MODULES:
        if mod.get("enabled", True) and "update_interval" not in mod:
            val = run(mod["default_update"])
            with lock:
                data[mod["name"]] = val

    update_display()
