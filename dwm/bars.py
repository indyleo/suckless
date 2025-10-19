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
        "name": "isstream",
        "cmd": "sysstats isstream",
        "interval": 0,
        "signal": "SIGUSR1",
    },
    {"name": "isrec", "cmd": "sysstats isrec", "interval": 0, "signal": "SIGUSR1"},
    {
        "name": "browser",
        # "cmd": "browserctrl -bst 20",
        "cmd": "echo ''",
        "interval": 0,
        "signal_offset": 4,
    },
    {
        "name": "song",
        "cmd": "songctrl -sst SubsonicTUI 20",
        "interval": 0,
        "signal_offset": 3,
    },
    {
        "name": "bright",
        "cmd": "sysstats brightness",
        "interval": 0,
        "signal_offset": 2,
    },
    {
        "name": "vol",
        "cmd": "sysstats volume",
        "interval": 0,
        "signal": "SIGUSR1",
        "signal_offset": 1,
    },
    {"name": "bat", "cmd": "sysstats battery", "interval": 30},
    {"name": "datetime", "cmd": "sysstats date_time", "interval": 60},
]

MAIN_INTERVAL = 0.2
data = {mod["name"]: "" for mod in MODULES}
events = {mod["name"]: threading.Event() for mod in MODULES}


# ==== FUNCTIONS ====
def run(cmd):
    try:
        return subprocess.check_output(cmd, shell=True, text=True).strip()
    except subprocess.CalledProcessError:
        return ""


def updater(mod):
    """Update the module on interval or when signal triggers"""
    name, cmd, interval = mod["name"], mod["cmd"], mod.get("interval", 0)

    # If interval == 0, run once at startup
    if interval == 0:
        val = run(cmd)
        with lock:
            data[name] = val

    while running:
        if interval > 0:
            val = run(cmd)
            with lock:
                data[name] = val
            time.sleep(interval)
        else:
            # wait for signal event
            events[name].wait()
            if not running:
                break
            val = run(cmd)
            with lock:
                data[name] = val
            events[name].clear()


def handle_signal(signum, frame):
    # Standard signals (SIGUSR1, SIGUSR2, etc.)
    if signum < signal.SIGRTMIN or signum > signal.SIGRTMAX:
        try:
            sig_name = signal.Signals(signum).name
            for mod in MODULES:
                if mod.get("signal") == sig_name:
                    events[mod["name"]].set()
        except ValueError:
            # unknown signal, ignore
            pass
    else:
        # Real-time signal
        offset = signum - signal.SIGRTMIN
        for mod in MODULES:
            if mod.get("signal_offset") == offset:
                events[mod["name"]].set()


def handle_exit(signum, frame):
    global running
    print("\nExiting cleanly...")
    running = False
    for e in events.values():
        e.set()
    subprocess.run(["xsetroot", "-name", ""])
    sys.exit(0)


# ==== SETUP SIGNALS ====
handled_signals = set()
for mod in MODULES:
    # standard signals
    sig_name = mod.get("signal")
    if sig_name and sig_name not in handled_signals:
        sig = getattr(signal, sig_name)
        signal.signal(sig, handle_signal)
        handled_signals.add(sig_name)
    # real-time signals
    if "signal_offset" in mod:
        sig = signal.SIGRTMIN + mod["signal_offset"]
        signal.signal(sig, handle_signal)

signal.signal(signal.SIGINT, handle_exit)
signal.signal(signal.SIGTERM, handle_exit)

# ==== START THREADS ====
for mod in MODULES:
    t = threading.Thread(target=updater, args=(mod,))
    t.daemon = True
    t.start()

# ==== MAIN LOOP ====
while running:
    with lock:
        fields = [v for v in data.values() if v]
        s = " || ".join(fields)
    subprocess.run(["xsetroot", "-name", s])
    if MAIN_INTERVAL > 0:
        time.sleep(MAIN_INTERVAL)
    else:
        signal.pause()
