# i2 DECK changes

Polyend's own changes stay in `CHANGELOG.md`; this file only lists the i2 edition.

## 0.5.0 (not released yet)
- Every key shows its full name on the screen (Ripple Delete, Match Frame, Previous Marker) instead of a 4-letter code. Short names are big and bold; longer ones wrap to two or three lines, and a word too long for the pad is split with a hyphen. Names in deck files can be up to 23 characters and keep their case.
- The live band shows the pressed key's name and the shortcut it sent.
- All built-in decks are REV +1 (CUT gets REV 1), so the card's files are replaced once and kept as `<name>-old.txt`. `grade-wheels/grade.txt` is REV 10 with the same names: copy it to the card by hand.
- Hold Shift in a deck to see how full the screen's display memory is. If a frame gets close to the Tracker's limit, key blocks fall back to short codes (right-hand block first) instead of the screen dropping layers.
- The right-hand key blocks are drawn with a display-chip shift (VERTEX_TRANSLATE_X), which halves what their text costs.
- Test harness: real Tracker font widths and Liberation Sans rendering; checks that every built-in key name fits.

## 0.4.1 (released Nov 18, 2026)
- i2 GRADE WHEELS (`tools/i2deck/grade-wheels`): Lift / Gamma / Gain / Offset, Contrast and Sat in DaVinci Resolve (free and Studio, Mac and Windows) from GRADE's new WHEELS page, through a helper app that drives Resolve's controls with the mouse. The new `grade.txt` goes on the SD card by hand (REV 9).
- GRADE, Resolve: on Windows the deck turns Num Lock on before number pad keys (the printer lights moved the playhead with Num Lock off).
- GRADE, Resolve: LITE flashes PRINTER LIGHTS ON / OFF; turning an offset dial while they're off flashes a reminder. Dials renamed OFS ALL / OFS RED / OFS GRN / OFS BLU. Built-in `grade.txt` REV 4.

## 0.4.0 (released Nov 17, 2026; runs on Ian's Tracker)
- MIXER deck on MASTER: a Mackie Control surface for the Premiere and Resolve mixers.
- FILE and SETUP decks on FILE and CONFIG.
- Boot splash on the original Tracker.
- HUB deck labels match the Creative Hub 1.22 MIDI map.
- Fix: FX1 opens HUB and FX2 opens STATIC, as printed on the Tracker (they were swapped).

## 0.3
- GRADE: Premiere Color Mode side, with a Resolve / Premiere switch.
- STATIC: the radio is audible, plus a GLITCH page.
- Lock: File and Config are locked; hold Shift + Rec for 2 seconds to unlock.
- NOTE, INSTRUMENT, FX1, FX2 open CUT, GRADE, HUB, STATIC. PATTERN opens the normal Tracker.

## 0.2 (tested on hardware, Nov 14, 2026)
- New full-screen interface: home screen, CUT, GRADE, HUB and STATIC decks.
- HOME on screen button 8 in every deck. Mac / Win switch.
- Fixes the black screen from the first build.

## 0.1
- Controller mode and the first experiments.
