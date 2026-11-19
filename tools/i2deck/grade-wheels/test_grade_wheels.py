"""Tests for i2 GRADE WHEELS' engine, with a fake mouse and clock. Run: python3 test_grade_wheels.py"""
import json
import os
import tempfile
import unittest

import i2_grade_wheels as gw


class FakeMouse:
    def __init__(self, x=500, y=400):
        self.x, self.y = x, y
        self.log = []
        self.pressed = False

    def position(self):
        return (self.x, self.y)

    def move_to(self, x, y):
        assert not self.pressed, "moved without dragging while pressed"
        self.x, self.y = x, y
        self.log.append(("move", x, y))

    def down(self):
        assert not self.pressed
        self.pressed = True
        self.log.append(("down", self.x, self.y))

    def drag_by(self, dx, dy):
        assert self.pressed, "dragged with the button up"
        self.x += dx
        self.y += dy
        self.log.append(("drag", dx, dy))

    def up(self):
        assert self.pressed
        self.pressed = False
        self.log.append(("up", self.x, self.y))


class Clock:
    def __init__(self):
        self.t = 100.0

    def __call__(self):
        return self.t


def make(points=None, front=True):
    cfg = gw.load_config("/nonexistent/path.json")
    cfg["points"].update(points or {})
    mouse = FakeMouse()
    clock = Clock()
    saved = []
    state = {"front": front}
    eng = gw.Engine(mouse, lambda: state["front"], cfg, save=lambda c: saved.append(json.loads(json.dumps(c))),
                    clock=clock, pause=lambda s: None)
    return eng, mouse, clock, saved, state


POINTS = {
    "lift.master": [100, 700], "lift.ball": [100, 600], "lift.reset": [160, 540],
    "gain.master": [500, 700], "gain.ball": [500, 600],
    "contrast": [80, 800], "sat": [400, 800],
}


class EngineTests(unittest.TestCase):
    def test_master_drag_holds_then_releases_and_restores(self):
        eng, m, clock, _, _ = make(POINTS)
        for _ in range(3):
            eng.cc(1, 65)
            clock.t += 0.05
        self.assertEqual(m.log[0], ("move", 100, 700))
        self.assertEqual(m.log[1], ("down", 100, 700))
        self.assertEqual([e for e in m.log if e[0] == "drag"], [("drag", 4, 0)] * 3)
        self.assertTrue(m.pressed)
        clock.t += 0.2
        eng.tick()
        self.assertTrue(m.pressed, "released too early")
        clock.t += 0.2
        eng.tick()
        self.assertFalse(m.pressed)
        self.assertEqual(m.log[-1], ("move", 500, 400))  # cursor back where it was

    def test_one_press_per_spin(self):
        eng, m, clock, _, _ = make(POINTS)
        for _ in range(20):
            eng.cc(1, 63)
            clock.t += 0.03
            eng.tick()
        self.assertEqual(sum(1 for e in m.log if e[0] == "down"), 1)
        self.assertEqual(sum(e[1] for e in m.log if e[0] == "drag"), -80)

    def test_ball_axes_share_one_drag_and_y_is_up(self):
        eng, m, clock, _, _ = make(POINTS)
        eng.cc(2, 65)
        eng.cc(3, 65)  # clockwise = up = negative y
        eng.cc(3, 63)
        self.assertEqual(sum(1 for e in m.log if e[0] == "down"), 1)
        drags = [e for e in m.log if e[0] == "drag"]
        self.assertEqual(drags, [("drag", 3, 0), ("drag", 0, -3), ("drag", 0, 3)])

    def test_switching_control_ends_the_drag_first(self):
        eng, m, clock, _, _ = make(POINTS)
        eng.cc(1, 65)
        eng.cc(5, 65)
        kinds = [e[0] for e in m.log]
        self.assertEqual(kinds, ["move", "down", "drag", "up", "move", "move", "down", "drag"])
        self.assertEqual(m.log[4], ("move", 500, 400))   # home
        self.assertEqual(m.log[5], ("move", 400, 800))   # sat

    def test_wheel_pick(self):
        eng, m, clock, _, _ = make(POINTS)
        eng.note(2, True)  # GAIN
        eng.cc(1, 65)
        self.assertEqual(m.log[0], ("move", 500, 700))
        # picking a wheel mid-drag lets go
        eng.note(0, True)
        self.assertFalse(m.pressed)

    def test_fine_accumulates_fractions(self):
        eng, m, clock, _, _ = make(POINTS)
        eng.note(13, True)  # FINE: 4 px / 4 = 1 px; field 3 px / 4 = 0.75
        for _ in range(4):
            eng.cc(4, 65)
        self.assertEqual(sum(e[1] for e in m.log if e[0] == "drag"), 3)

    def test_multi_click_value(self):
        eng, m, clock, _, _ = make(POINTS)
        eng.cc(1, 67)  # three clicks in one message
        self.assertEqual([e for e in m.log if e[0] == "drag"], [("drag", 12, 0)])

    def test_not_front_does_nothing(self):
        eng, m, clock, _, state = make(POINTS, front=False)
        eng.cc(1, 65)
        self.assertEqual(m.log, [])
        # losing focus mid-drag lets go
        state["front"] = True
        eng.cc(1, 65)
        state["front"] = False
        eng.cc(1, 65)
        self.assertFalse(m.pressed)

    def test_unlearned_control_does_nothing(self):
        eng, m, clock, _, _ = make(POINTS)
        eng.note(1, True)  # GAMMA: nothing learned
        eng.cc(1, 65)
        self.assertEqual(m.log, [])

    def test_learn_records_and_saves(self):
        eng, m, clock, saved, _ = make({})
        eng.note(12, True)
        m.x, m.y = 321, 654
        eng.cc(1, 65)
        m.x, m.y = 330, 600
        eng.cc(3, 63)
        m.x, m.y = 380, 540
        eng.note(14, True)  # RESET WHL
        eng.note(2, True)   # GAIN
        m.x, m.y = 900, 650
        eng.cc(1, 65)
        m.x, m.y = 50, 820
        eng.cc(4, 65)
        self.assertEqual(m.log, [], "learn mode must not move the mouse")
        eng.note(12, True)  # off: save
        pts = saved[-1]["points"]
        self.assertEqual(pts["lift.master"], [321, 654])
        self.assertEqual(pts["lift.ball"], [330, 600])
        self.assertEqual(pts["lift.reset"], [380, 540])
        self.assertEqual(pts["gain.master"], [900, 650])
        self.assertEqual(pts["contrast"], [50, 820])

    def test_reset_clicks_and_returns(self):
        eng, m, clock, _, _ = make(POINTS)
        eng.note(14, True)
        self.assertEqual(m.log, [("move", 160, 540), ("down", 160, 540), ("up", 160, 540), ("move", 500, 400)])

    def test_note_off_and_other_ccs_ignored(self):
        eng, m, clock, _, _ = make(POINTS)
        eng.note(12, False)
        eng.cc(9, 65)
        eng.cc(1, 64)
        self.assertFalse(eng.learn)
        self.assertEqual(m.log, [])

    def test_invert_setting(self):
        eng, m, clock, _, _ = make(POINTS)
        eng.cfg["settings"]["invert"]["master"] = True
        eng.cc(1, 65)
        self.assertEqual([e for e in m.log if e[0] == "drag"], [("drag", -4, 0)])

    def test_config_round_trip(self):
        d = tempfile.mkdtemp()
        p = os.path.join(d, "sub", "c.json")
        cfg = gw.load_config(p)
        cfg["points"]["sat"] = [1.0, 2.0]
        cfg["settings"]["px"]["master"] = 7
        gw.save_config(p, cfg)
        back = gw.load_config(p)
        self.assertEqual(back["points"]["sat"], [1.0, 2.0])
        self.assertEqual(back["settings"]["px"]["master"], 7)
        self.assertEqual(back["settings"]["px"]["ball"], 3)  # defaults filled in

    def test_front_check_match(self):
        names = {"n": "DaVinci Resolve"}
        ok = gw.make_front_check(lambda: names["n"], "resolve", False)
        self.assertTrue(ok())
        ok2 = gw.make_front_check(lambda: "Resolve.exe", "resolve", False)
        self.assertTrue(ok2())
        ok3 = gw.make_front_check(lambda: "Google Chrome", "resolve", False)
        self.assertFalse(ok3())


if __name__ == "__main__":
    unittest.main(verbosity=1)
