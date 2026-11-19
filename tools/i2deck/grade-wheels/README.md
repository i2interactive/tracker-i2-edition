# i2 GRADE WHEELS

Lift, Gamma, Gain, Offset, Contrast and Sat in DaVinci Resolve from the jog wheel of i2 DECK's GRADE deck. Works with Resolve and Resolve Studio, on Mac and Windows.

Resolve has no keyboard shortcuts and no MIDI for its colour wheels. So the Tracker sends MIDI, and this small helper on the computer turns it into mouse drags on Resolve's own controls, the way your hand would. You show it where each control is once.

## What's in here

| File | What it is |
|---|---|
| `grade.txt` | The new GRADE deck for the Tracker's SD card (works with the firmware already on it) |
| `i2_grade_wheels.py` | The helper |
| `start-mac.command` / `start-windows.bat` | Double-click to start the helper. The first run sets itself up (needs Python 3). |
| `test_grade_wheels.py` | Tests for the helper's logic |

## Set up

1. **Tracker:** copy `grade.txt` to the SD card as `i2/Deck/grade.txt`, replacing the one there. GRADE now has two pages. Screen button 7 turns the page; page 2 is **WHEELS**.
2. **Mac:** double-click `start-mac.command`. The first time, macOS may block it. If so, right-click it, choose Open, then Open. Then allow mouse control: System Settings > Privacy & Security > **Accessibility**, turn on **Terminal**, and start it again.
3. **Windows:** double-click `start-windows.bat` (install Python 3 from python.org first if you don't have it).
4. Leave the helper's window open while you grade. It only moves the mouse while DaVinci Resolve is the front app.

## Learn the positions (once per screen layout)

In Resolve, open the Color page with the Primaries **Color Wheels** showing. On the Tracker, go to GRADE (Resolve side), page 2 WHEELS.

1. Press **LRN**. The helper says LEARN ON.
2. Press **LIFT**.
   - Hover over the strip under the Lift wheel (its master dial) and turn **WHEEL** one click.
   - Hover over the middle of the Lift wheel and turn **BAL X** one click.
   - Hover over the Lift wheel's small reset arrow and press **RSTW**.
3. Do the same after pressing **GAMM**, **GAIN** and **OFST**.
4. Hover over the **Contrast** number and turn **CONTRST**; hover over **Sat** and turn **SAT**.
5. Press **LRN** again to save. The positions are kept in `~/.i2deck/grade-wheels.json` (on Windows, in your user folder).

Learning doesn't move anything in Resolve. If you change Resolve's layout, its window size or your screens, learn again. Run with `--forget` to clear everything.

## Use

| Tracker | Does |
|---|---|
| LIFT / GAMM / GAIN / OFST | pick the wheel the dials work on |
| WHEEL dial | that wheel's master (luminance) |
| BAL X / BAL Y dials | that wheel's colour balance, left-right / down-up |
| CONTRST / SAT dials | Contrast / Sat |
| FINE | smaller steps, on and off |
| RSTW | reset the picked wheel |
| Shift | not used by the helper |

The rest of page 2 (nodes, clips, undo, compare, play) and all of page 1 send Resolve's keyboard shortcuts as before. While the wheel turns, the helper holds the mouse button down on the control; it lets go 0.35 s after you stop and puts the cursor back where it was.

GRADE is now a MIDI deck, so the Tracker's Play, arrow, Copy and Insert buttons don't send keys in it. Page 2 has a PLAY pad (Space) instead.

## Tuning

Edit `~/.i2deck/grade-wheels.json` while the helper is stopped:

- `settings.px`: pixels of drag per dial click for `master`, `ball` and `field` (Contrast, Sat). Raise for bigger steps.
- `settings.fine_div`: how much FINE divides the step.
- `settings.invert`: flip `master`, `field` or `ball_y` if a direction feels backwards.
- `settings.release_ms`: how long after the wheel stops the mouse lets go.

## Things to check on the first run

- **Colour balance (BAL X / BAL Y).** This assumes that dragging inside a Resolve colour wheel moves the balance from where it is. If the balance jumps to where you hovered when learning, tell Claude, and it will switch BAL to drag the balance indicator itself.
- **Step size.** 4 px per click on the master strip is a guess; adjust `px` to taste.
- **Windows:** only one app can open the Tracker's MIDI at a time. If the helper can't open it, close the Creative Hub tab using i2 DECK HUB, or turn off Resolve's MIDI audio console (MIXER) while grading.

## Options

```
start-mac.command --list        # list MIDI inputs
start-mac.command --port NAME   # use another MIDI input
start-mac.command --any-app     # don't wait for Resolve to be in front
start-mac.command --forget      # clear the learned positions
```

i2 interactive LLC, MIT licence.
