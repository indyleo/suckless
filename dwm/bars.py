#!/usr/bin/env python3
"""
Efficient DWM status bar updater with signal-based and periodic modules.
Features:
- Signal-triggered updates (no polling)
- Periodic updates for slow-changing data
- Minimal CPU usage when idle
- Thread-safe module updates
"""

import signal
import subprocess
import sys
import threading
import time
from typing import Dict, List

# ==== CONFIGURATION ====
MODULES = [
    {
        "name": "chat",
        "enabled": False,
        "command": "tail -n 1 ~/.cache/twitch_chat.log 2>/dev/null || echo 'No chat yet'",
        "signal_offset": 19,
    },
    {
        "name": "ismedia",
        "enabled": True,
        "command": "sysstats ismedia",
        "signal_offset": 20,
    },
    {
        "name": "browser",
        "enabled": False,
        "command": "browserctrl -bst 20",
        "signal_offset": 21,
    },
    {
        "name": "song",
        "enabled": True,
        "command": "songctrl -sst SubsonicTUI 20",
        "signal_offset": 22,
    },
    {
        "name": "vol",
        "enabled": True,
        "command": "sysstats volume",
        "signal_offset": 23,
    },
    {
        "name": "bright",
        "enabled": True,
        "command": "sysstats brightness",
        "signal_offset": 24,
    },
    {
        "name": "bat",
        "enabled": True,
        "command": "sysstats battery",
        "update_interval": 30,
        "signal_offset": 25,  # Optional: allow manual refresh
    },
    {
        "name": "datetime",
        "enabled": True,
        "command": "sysstats date_time",
        "update_interval": 60,
    },
]

SEPARATOR = " || "
EMPTY_DISPLAY = ""

# ==== GLOBALS ====
running = True
data_lock = threading.Lock()
data: Dict[str, str] = {}
display_queue = threading.Event()


# ==== CORE FUNCTIONS ====
def run_command(cmd: str) -> str:
    """Execute shell command and return output."""
    try:
        return subprocess.check_output(
            cmd, shell=True, text=True, stderr=subprocess.DEVNULL, timeout=5
        ).strip()
    except (subprocess.CalledProcessError, subprocess.TimeoutExpired):
        return ""


def update_module(name: str, value: str) -> None:
    """Thread-safe module data update."""
    with data_lock:
        if data.get(name) != value:  # Only update if changed
            data[name] = value
            display_queue.set()


def get_display_string() -> str:
    """Build display string from current module data."""
    with data_lock:
        fields = [
            data.get(mod["name"], "") for mod in MODULES if mod.get("enabled", True)
        ]
    return SEPARATOR.join(f for f in fields if f)


def update_display() -> None:
    """Update xsetroot with current status."""
    display_str = get_display_string()
    subprocess.run(
        ["xsetroot", "-name", display_str], stderr=subprocess.DEVNULL, check=False
    )


# ==== THREAD WORKERS ====
def signal_worker(mod: Dict) -> None:
    """Wait for signals and update module."""
    name = mod["name"]
    event = threading.Event()

    # Store event for signal handler
    signal_events[name] = event

    # Initial update
    value = run_command(mod["command"])
    update_module(name, value)

    while running:
        event.wait()
        if not running:
            break

        value = run_command(mod["command"])
        update_module(name, value)
        event.clear()


def periodic_worker(mod: Dict) -> None:
    """Periodically update module."""
    name = mod["name"]
    interval = mod["update_interval"]

    while running:
        value = run_command(mod["command"])
        update_module(name, value)

        # Sleep in small chunks for responsive shutdown
        for _ in range(int(interval * 10)):
            if not running:
                break
            time.sleep(0.1)


def display_worker() -> None:
    """Update display when changes occur."""
    while running:
        display_queue.wait(timeout=1.0)
        if not running:
            break
        display_queue.clear()
        update_display()


# ==== SIGNAL HANDLING ====
signal_events: Dict[str, threading.Event] = {}


def handle_signal(signum: int, frame) -> None:
    """Route real-time signal to appropriate module."""
    offset = signum - signal.SIGRTMIN

    for mod in MODULES:
        if (
            mod.get("enabled", True)
            and mod.get("signal_offset") == offset
            and mod["name"] in signal_events
        ):
            signal_events[mod["name"]].set()
            break


def handle_exit(signum: int, frame) -> None:
    """Clean shutdown."""
    global running
    print("\nShutting down status bar...", file=sys.stderr)
    running = False

    # Wake all threads
    for event in signal_events.values():
        event.set()
    display_queue.set()

    # Clear display
    subprocess.run(["xsetroot", "-name", EMPTY_DISPLAY], stderr=subprocess.DEVNULL)
    sys.exit(0)


# ==== MAIN ====
def main():
    """Initialize and run status bar."""
    global running

    # Register signal handlers
    for mod in MODULES:
        if mod.get("enabled", True) and "signal_offset" in mod:
            sig = signal.SIGRTMIN + mod["signal_offset"]
            signal.signal(sig, handle_signal)

    signal.signal(signal.SIGINT, handle_exit)
    signal.signal(signal.SIGTERM, handle_exit)

    # Start worker threads
    threads: List[threading.Thread] = []

    # Display updater (single thread)
    t = threading.Thread(target=display_worker, daemon=True, name="display")
    t.start()
    threads.append(t)

    for mod in MODULES:
        if not mod.get("enabled", True):
            continue

        # Signal-based updates
        if "signal_offset" in mod:
            t = threading.Thread(
                target=signal_worker,
                args=(mod,),
                daemon=True,
                name=f"signal-{mod['name']}",
            )
            t.start()
            threads.append(t)

        # Periodic updates (can coexist with signal updates)
        if "update_interval" in mod:
            t = threading.Thread(
                target=periodic_worker,
                args=(mod,),
                daemon=True,
                name=f"periodic-{mod['name']}",
            )
            t.start()
            threads.append(t)

    print(f"Status bar running with {len(threads)} worker threads", file=sys.stderr)

    # Keep main thread alive
    try:
        while running:
            time.sleep(1)
    except KeyboardInterrupt:
        handle_exit(signal.SIGINT, None)


if __name__ == "__main__":
    main()
