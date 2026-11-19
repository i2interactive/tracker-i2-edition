#!/usr/bin/env python3
"""i2 GRADE WHEELS: Lift / Gamma / Gain / Offset in DaVinci Resolve from i2 DECK.

Resolve has no keyboard shortcuts and no MIDI for its colour wheels. This
helper listens to the WHEELS page of i2 DECK's GRADE deck (MIDI channel 2)
and turns it into mouse drags on Resolve's own controls, the way your hand
would. It works the same in Resolve and Resolve Studio, on Mac and Windows.

  Pads   LIFT / GAMMA / GAIN / OFFSET  pick the wheel the dials work on
         LEARN                          learn mode on / off
         FINE                           smaller steps on / off
         RESET WHL                      clicks the picked wheel's reset button
  Dials  WHEEL    the picked wheel's master dial (the strip under it)
         BAL X    the wheel's colour balance, left / right
         BAL Y    the wheel's colour balance, down / up
         CONTRST  the Contrast field
         SAT      the Sat field

Learn once: press LEARN, then for each control hover the mouse over it in
Resolve and turn the dial (or press RESET WHL) that should drive it. Press
LEARN again to save. Positions live in ~/.i2deck/grade-wheels.json.

The helper only moves the mouse while DaVinci Resolve is the front app.
Run with --help for options. i2 interactive LLC, MIT licence.
"""

import argparse
import json
import os
import queue
import sys
import threading
import time

__version__ = "1.0.0"

# What the deck sends (grade.txt, page 2 WHEELS). MIDI channel 2 = index 1.
CHANNEL = 1
NOTE_WHEELS = {0: "lift", 1: "gamma", 2: "gain", 3: "offset"}
NOTE_LEARN = 12
NOTE_FINE = 13
NOTE_RESET = 14
# CC -> (target, axis, kind). "{w}" is the picked wheel.
CC_TARGETS = {
    1: ("{w}.master", "x", "master"),
    2: ("{w}.ball", "x", "ball"),
    3: ("{w}.ball", "y", "ball"),
    4: ("contrast", "x", "field"),
    5: ("sat", "x", "field"),
}
WHEEL_NAMES = {"lift": "LIFT", "gamma": "GAMMA", "gain": "GAIN", "offset": "OFFSET"}

DEFAULT_SETTINGS = {
    # pixels of drag per dial click, by kind of control
    "px": {"master": 4, "ball": 3, "field": 3},
    "fine_div": 4,          # FINE divides the step by this
    "release_ms": 350,      # let go of the mouse this long after the dial stops
    "settle_ms": 15,        # pause after moving to a control and after pressing
    "invert": {"ball_y": False, "master": False, "field": False},
    "app_match": "resolve", # front app name must contain this (lower case)
}

CONFIG_PATH = os.path.join(os.path.expanduser("~"), ".i2deck", "grade-wheels.json")


def say(text):
    print(time.strftime("%H:%M:%S ") + text, flush=True)


# ------------------------------------------------------------------------------------------
# Config
# ------------------------------------------------------------------------------------------
def load_config(path):
    cfg = {"points": {}, "settings": json.loads(json.dumps(DEFAULT_SETTINGS))}
    try:
        with open(path, "r", encoding="utf-8") as f:
            data = json.load(f)
        cfg["points"].update({k: [float(v[0]), float(v[1])] for k, v in data.get("points", {}).items()})
        for k, v in data.get("settings", {}).items():
            if isinstance(v, dict) and isinstance(cfg["settings"].get(k), dict):
                cfg["settings"][k].update(v)
            else:
                cfg["settings"][k] = v
    except FileNotFoundError:
        pass
    except (ValueError, OSError) as e:
        say("Couldn't read %s (%s); starting with no positions." % (path, e))
    return cfg


def save_config(path, cfg):
    os.makedirs(os.path.dirname(path), exist_ok=True)
    tmp = path + ".tmp"
    with open(tmp, "w", encoding="utf-8") as f:
        json.dump(cfg, f, indent=2, sort_keys=True)
    os.replace(tmp, path)


# ------------------------------------------------------------------------------------------
# The engine: MIDI in, mouse out. No OS calls here, so it can be tested.
# ------------------------------------------------------------------------------------------
class Engine:
    def __init__(self, mouse, front_ok, cfg, save=None, beep=None, clock=time.monotonic, pause=time.sleep):
        self.mouse = mouse
        self.front_ok = front_ok    # () -> bool: is Resolve the front app?
        self.cfg = cfg
        self.save = save or (lambda cfg: None)
        self.beep = beep or (lambda kind: None)
        self.clock = clock
        self.pause = pause
        self.wheel = "lift"
        self.learn = False
        self.fine = False
        self.drag = None            # {"key", "home", "last", "rem": [x, y]}
        self.warned = {}

    # --- helpers -------------------------------------------------------------------------
    @property
    def s(self):
        return self.cfg["settings"]

    def _settle(self):
        self.pause(self.s.get("settle_ms", 15) / 1000.0)

    def _warn(self, key, text):
        now = self.clock()
        if now - self.warned.get(key, -1e9) > 3.0:
            self.warned[key] = now
            say(text)

    def end_drag(self):
        if not self.drag:
            return
        self.mouse.up()
        self._settle()
        hx, hy = self.drag["home"]
        self.mouse.move_to(hx, hy)
        self.drag = None

    # --- MIDI ----------------------------------------------------------------------------
    def note(self, note, on):
        if not on:
            return
        if note in NOTE_WHEELS:
            self.end_drag()
            self.wheel = NOTE_WHEELS[note]
            say("Wheel: %s" % WHEEL_NAMES[self.wheel])
        elif note == NOTE_LEARN:
            self.end_drag()
            self.learn = not self.learn
            if self.learn:
                say("LEARN ON. Hover over a control in Resolve, then turn its dial "
                    "(or press RESET WHL over a wheel's reset button). Press LEARN again to save.")
                self.beep("on")
            else:
                self.save(self.cfg)
                say("LEARN OFF. Saved %d positions." % len(self.cfg["points"]))
                self.beep("off")
        elif note == NOTE_FINE:
            self.fine = not self.fine
            say("FINE %s" % ("on" if self.fine else "off"))
        elif note == NOTE_RESET:
            key = "%s.reset" % self.wheel
            if self.learn:
                self._learn(key)
            else:
                self._click(key)

    def cc(self, number, value):
        if number not in CC_TARGETS:
            return
        delta = value - 64          # 65 = one click up, 63 = one click down
        if delta == 0:
            return
        key_t, axis, kind = CC_TARGETS[number]
        key = key_t.format(w=self.wheel)
        if self.learn:
            self._learn(key)
            return
        if not self.front_ok():
            self.end_drag()
            self._warn("front", "DaVinci Resolve isn't the front app; ignoring the dial.")
            return
        point = self.cfg["points"].get(key)
        if point is None:
            self._warn("missing:" + key, "No position for %s yet: press LEARN, hover over it, turn the dial." % key)
            return
        now = self.clock()
        if self.drag and self.drag["key"] != key:
            self.end_drag()
        if not self.drag:
            home = self.mouse.position()
            self.mouse.move_to(point[0], point[1])
            self._settle()
            self.mouse.down()
            self._settle()
            self.drag = {"key": key, "home": home, "last": now, "rem": [0.0, 0.0]}
        self.drag["last"] = now

        step = float(self.s["px"].get(kind, 3))
        if self.fine:
            step /= float(self.s.get("fine_div", 4) or 1)
        inv = self.s.get("invert", {})
        if axis == "y":
            # clockwise = up = toward the top of the screen
            amount = -delta * step * (-1 if inv.get("ball_y") else 1)
            idx = 1
        else:
            amount = delta * step * (-1 if inv.get(kind) else 1)
            idx = 0
        self.drag["rem"][idx] += amount
        whole = int(self.drag["rem"][idx])
        if whole:
            self.drag["rem"][idx] -= whole
            dx, dy = (whole, 0) if idx == 0 else (0, whole)
            self.mouse.drag_by(dx, dy)

    def tick(self):
        if self.drag and (self.clock() - self.drag["last"]) * 1000.0 >= self.s.get("release_ms", 350):
            self.end_drag()

    # --- learn / click -------------------------------------------------------------------
    def _learn(self, key):
        x, y = self.mouse.position()
        old = self.cfg["points"].get(key)
        self.cfg["points"][key] = [float(x), float(y)]
        if old is None or abs(old[0] - x) > 0.5 or abs(old[1] - y) > 0.5:
            say("Learned %-14s at %d, %d" % (key, x, y))
            self.beep("learn")

    def _click(self, key):
        if not self.front_ok():
            self._warn("front", "DaVinci Resolve isn't the front app; ignoring.")
            return
        point = self.cfg["points"].get(key)
        if point is None:
            self._warn("missing:" + key, "No position for %s yet: press LEARN, hover over the wheel's reset button, press RESET WHL." % key)
            return
        self.end_drag()
        home = self.mouse.position()
        self.mouse.move_to(point[0], point[1])
        self._settle()
        self.mouse.down()
        self._settle()
        self.mouse.up()
        self._settle()
        self.mouse.move_to(home[0], home[1])
        say("Reset %s" % WHEEL_NAMES[self.wheel])


# ------------------------------------------------------------------------------------------
# macOS: Quartz events (needs Accessibility permission for the app running this)
# ------------------------------------------------------------------------------------------
class MacMouse:
    def __init__(self):
        import Quartz
        self.Q = Quartz
        self.x, self.y = self.position()
        self.pressed = False

    def position(self):
        Q = self.Q
        p = Q.CGEventGetLocation(Q.CGEventCreate(None))
        return (p.x, p.y)

    def _post(self, kind, x, y, dx=0, dy=0, click=0):
        Q = self.Q
        ev = Q.CGEventCreateMouseEvent(None, kind, (x, y), Q.kCGMouseButtonLeft)
        if dx or dy:
            # apps that scrub with a hidden cursor read these, not the position
            Q.CGEventSetIntegerValueField(ev, Q.kCGMouseEventDeltaX, int(dx))
            Q.CGEventSetIntegerValueField(ev, Q.kCGMouseEventDeltaY, int(dy))
        if click:
            Q.CGEventSetIntegerValueField(ev, Q.kCGMouseEventClickState, click)
        Q.CGEventPost(Q.kCGHIDEventTap, ev)

    def move_to(self, x, y):
        self.x, self.y = x, y
        self._post(self.Q.kCGEventMouseMoved, x, y)

    def down(self):
        self.pressed = True
        self._post(self.Q.kCGEventLeftMouseDown, self.x, self.y, click=1)

    def drag_by(self, dx, dy):
        self.x += dx
        self.y += dy
        self._post(self.Q.kCGEventLeftMouseDragged, self.x, self.y, dx, dy)

    def up(self):
        if self.pressed:
            self.pressed = False
            self._post(self.Q.kCGEventLeftMouseUp, self.x, self.y, click=1)


def mac_front_app():
    import Quartz as Q
    opts = Q.kCGWindowListOptionOnScreenOnly | Q.kCGWindowListExcludeDesktopElements
    for w in Q.CGWindowListCopyWindowInfo(opts, Q.kCGNullWindowID) or []:
        if w.get("kCGWindowLayer", 1) == 0:
            return str(w.get("kCGWindowOwnerName", ""))
    return ""


def mac_beep(kind):
    import subprocess
    sound = {"on": "Tink", "off": "Glass", "learn": "Pop"}.get(kind, "Tink")
    try:
        subprocess.Popen(["afplay", "/System/Library/Sounds/%s.aiff" % sound])
    except OSError:
        pass


def mac_check_access():
    try:
        import ApplicationServices as AS
        return bool(AS.AXIsProcessTrusted())
    except Exception:
        return True


# ------------------------------------------------------------------------------------------
# Windows: SendInput with absolute moves (no pointer acceleration)
# ------------------------------------------------------------------------------------------
class WinMouse:
    def __init__(self):
        import ctypes
        from ctypes import wintypes
        self.ct = ctypes
        self.u32 = ctypes.windll.user32
        try:
            ctypes.windll.shcore.SetProcessDpiAwareness(2)
        except Exception:
            try:
                self.u32.SetProcessDPIAware()
            except Exception:
                pass

        class MOUSEINPUT(ctypes.Structure):
            _fields_ = [("dx", wintypes.LONG), ("dy", wintypes.LONG), ("mouseData", wintypes.DWORD),
                        ("dwFlags", wintypes.DWORD), ("time", wintypes.DWORD),
                        ("dwExtraInfo", ctypes.POINTER(ctypes.c_ulong))]

        class KEYBDINPUT(ctypes.Structure):
            _fields_ = [("wVk", wintypes.WORD), ("wScan", wintypes.WORD), ("dwFlags", wintypes.DWORD),
                        ("time", wintypes.DWORD), ("dwExtraInfo", ctypes.POINTER(ctypes.c_ulong))]

        class _U(ctypes.Union):
            _fields_ = [("mi", MOUSEINPUT), ("ki", KEYBDINPUT)]

        class INPUT(ctypes.Structure):
            _anonymous_ = ("u",)
            _fields_ = [("type", wintypes.DWORD), ("u", _U)]

        self.MOUSEINPUT, self.INPUT = MOUSEINPUT, INPUT
        self.POINT = wintypes.POINT
        self.x, self.y = self.position()
        self.pressed = False

    def position(self):
        p = self.POINT()
        self.u32.GetCursorPos(self.ct.byref(p))
        return (p.x, p.y)

    def _send(self, flags, x=0, y=0):
        MOVE, ABSOLUTE, VIRTUALDESK = 0x0001, 0x8000, 0x4000
        dx = dy = 0
        if flags & MOVE:
            vx, vy = self.u32.GetSystemMetrics(76), self.u32.GetSystemMetrics(77)
            vw, vh = max(self.u32.GetSystemMetrics(78), 2), max(self.u32.GetSystemMetrics(79), 2)
            dx = int(round((x - vx) * 65535.0 / (vw - 1)))
            dy = int(round((y - vy) * 65535.0 / (vh - 1)))
            flags |= ABSOLUTE | VIRTUALDESK
        inp = self.INPUT(type=0)
        inp.mi = self.MOUSEINPUT(dx, dy, 0, flags, 0, None)
        self.u32.SendInput(1, self.ct.byref(inp), self.ct.sizeof(self.INPUT))

    def move_to(self, x, y):
        self.x, self.y = x, y
        self.u32.SetCursorPos(int(x), int(y))
        self._send(0x0001, x, y)

    def down(self):
        self.pressed = True
        self._send(0x0002)  # LEFTDOWN

    def drag_by(self, dx, dy):
        self.x += dx
        self.y += dy
        self._send(0x0001, self.x, self.y)

    def up(self):
        if self.pressed:
            self.pressed = False
            self._send(0x0004)  # LEFTUP


def win_front_app():
    import ctypes
    from ctypes import wintypes
    u32, k32 = ctypes.windll.user32, ctypes.windll.kernel32
    u32.GetForegroundWindow.restype = wintypes.HWND
    k32.OpenProcess.restype = wintypes.HANDLE
    k32.OpenProcess.argtypes = [wintypes.DWORD, wintypes.BOOL, wintypes.DWORD]
    k32.QueryFullProcessImageNameW.argtypes = [wintypes.HANDLE, wintypes.DWORD, wintypes.LPWSTR,
                                               ctypes.POINTER(wintypes.DWORD)]
    k32.CloseHandle.argtypes = [wintypes.HANDLE]
    u32.GetWindowThreadProcessId.argtypes = [wintypes.HWND, ctypes.POINTER(wintypes.DWORD)]
    hwnd = u32.GetForegroundWindow()
    if not hwnd:
        return ""
    pid = wintypes.DWORD()
    u32.GetWindowThreadProcessId(hwnd, ctypes.byref(pid))
    h = k32.OpenProcess(0x1000, False, pid.value)  # PROCESS_QUERY_LIMITED_INFORMATION
    if not h:
        return ""
    try:
        buf = ctypes.create_unicode_buffer(1024)
        size = wintypes.DWORD(1024)
        if k32.QueryFullProcessImageNameW(h, 0, buf, ctypes.byref(size)):
            return os.path.basename(buf.value)
        return ""
    finally:
        k32.CloseHandle(h)


def win_beep(kind):
    try:
        import winsound
        winsound.MessageBeep({"on": 0x40, "off": 0x30, "learn": 0}.get(kind, 0))
    except Exception:
        pass


# ------------------------------------------------------------------------------------------
# Running it
# ------------------------------------------------------------------------------------------
def pick_port(mido, wanted):
    names = mido.get_input_names()
    if wanted:
        for n in names:
            if wanted.lower() in n.lower():
                return n
        return None
    for n in names:
        low = n.lower()
        if "tracker" in low or "polyend" in low:
            return n
    return None


def make_front_check(get_name, match, any_app):
    if any_app:
        return lambda: True
    state = {"t": -1.0, "ok": False}

    def ok():
        now = time.monotonic()
        if now - state["t"] > 0.25:
            try:
                state["ok"] = match in get_name().lower()
            except Exception:
                state["ok"] = False
            state["t"] = now
        return state["ok"]
    return ok


def main(argv=None):
    ap = argparse.ArgumentParser(description="i2 GRADE WHEELS: Resolve colour wheels from i2 DECK.")
    ap.add_argument("--list", action="store_true", help="list MIDI inputs and quit")
    ap.add_argument("--port", default="", help="part of the MIDI input name to use (default: the Tracker)")
    ap.add_argument("--config", default=CONFIG_PATH, help="where positions are kept (default: %(default)s)")
    ap.add_argument("--any-app", action="store_true", help="move the mouse even when Resolve isn't in front")
    ap.add_argument("--forget", action="store_true", help="forget every learned position and quit")
    ap.add_argument("--version", action="version", version="i2 GRADE WHEELS " + __version__)
    args = ap.parse_args(argv)

    if args.forget:
        cfg = load_config(args.config)
        cfg["points"] = {}
        save_config(args.config, cfg)
        say("Forgot every position (%s)." % args.config)
        return 0

    try:
        import mido
        mido.set_backend("mido.backends.rtmidi")
    except ImportError:
        say("Missing mido / python-rtmidi. Run the start script, or: pip install -r requirements.txt")
        return 1

    if args.list:
        for n in mido.get_input_names():
            print("  " + n)
        return 0

    if sys.platform == "darwin":
        mouse, front_name, beep = MacMouse(), mac_front_app, mac_beep
        if not mac_check_access():
            say("macOS hasn't allowed this app to control the mouse yet. Open System Settings > "
                "Privacy & Security > Accessibility and turn on Terminal (or the app running this), "
                "then start the helper again.")
    elif sys.platform.startswith("win"):
        mouse, front_name, beep = WinMouse(), win_front_app, win_beep
    else:
        say("Mac and Windows only.")
        return 1

    cfg = load_config(args.config)
    front = make_front_check(front_name, cfg["settings"].get("app_match", "resolve"), args.any_app)
    engine = Engine(mouse, front, cfg, save=lambda c: save_config(args.config, c), beep=beep)

    events = queue.Queue()
    port_name = None
    port = None
    next_check = 0.0
    say("i2 GRADE WHEELS %s. %d positions learned (%s)." % (__version__, len(cfg["points"]), args.config))
    say("On the Tracker: GRADE (Resolve side) > screen button 7 for the WHEELS page.")
    try:
        while True:
            # (re)connect to the Tracker
            if port is None:
                port_name = pick_port(mido, args.port)
                if port_name is None:
                    engine._warn("noport", "Waiting for the Tracker's MIDI input (connect it, open i2 DECK)...")
                    time.sleep(1.0)
                    continue
                try:
                    port = mido.open_input(port_name, callback=events.put)
                    say("Listening to: %s" % port_name)
                except (IOError, OSError) as e:
                    engine._warn("open", "Couldn't open %s (%s). On Windows only one app can use it at a "
                                 "time: close the Creative Hub tab, or Resolve's MIDI audio console." % (port_name, e))
                    time.sleep(1.0)
                    continue
            try:
                msg = events.get(timeout=0.02)
            except queue.Empty:
                msg = None
            if msg is not None and getattr(msg, "channel", None) == CHANNEL:
                if msg.type == "note_on":
                    engine.note(msg.note, msg.velocity > 0)
                elif msg.type == "control_change":
                    engine.cc(msg.control, msg.value)
            engine.tick()
            if msg is None and time.monotonic() >= next_check:
                next_check = time.monotonic() + 1.0
                gone = port_name not in mido.get_input_names()
            else:
                gone = False
            if gone:
                engine.end_drag()
                say("The Tracker went away.")
                try:
                    port.close()
                except Exception:
                    pass
                port = None
    except KeyboardInterrupt:
        pass
    finally:
        engine.end_drag()
        if engine.learn:
            save_config(args.config, cfg)
        if port is not None:
            try:
                port.close()
            except Exception:
                pass
    say("Stopped.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
