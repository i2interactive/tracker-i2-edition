# Tracker i2 edition

Custom firmware for the **original Polyend Tracker** by [i2 interactive](https://i2interactive.com), built on Polyend's open-source [Tracker Community Edition](https://github.com/polyend/tracker-ce).

It adds **Controller mode** and the **i2 Arcade**. Controller mode: plug the Tracker into a Mac or PC over USB-C and its 48 pads, jog wheel and buttons become a hardware controller for Premiere Pro. The screen shows what every pad does. Nothing needs installing on the computer, because the Tracker shows up as a standard USB keyboard next to its usual MIDI port. Everything else on the Tracker works as it does in Community Edition.

> **Status: v0.1, not yet tested on hardware.** It builds cleanly for all three Trackers, but Controller mode and the Arcade only appear on the original Tracker (2020). See the [test checklist](#hardware-test-checklist).

## Install

**Step-by-step guide: [INSTALL.md](INSTALL.md)**. It goes from a stock Tracker to Controller mode and the i2 Arcade, with recovery steps.

The short version:
1. Back up your SD card.
2. Put the official Tracker 1.9.2 and CE 1.0 `.ptf` files from [polyend.com/downloads](https://polyend.com/downloads/) in `Firmware/` as a safety net.
3. Download the newest **polyend-tracker-ce-firmware** artifact from the [Actions tab](https://github.com/i2interactive/tracker-i2-edition/actions/workflows/build.yml). You need to be signed in to GitHub.
4. Copy `PolyendTracker_i2_<version>.ptf` to `Firmware/` without renaming it.
5. On the Tracker, run **Config > Firmware > Firmware update**.
6. Find the new **Config > i2** menu.

## Controller mode

**Open it:** Config > **i2** > **Controller**. To leave, press **Close** (the far-right screen button) or any module button.

| Control | What it does |
|---|---|
| Pads 1–48 | Send the shortcut shown on screen. Keys stay held while the pad is held, so J/K/L shuttling works |
| Shift (hold) | Adds Shift to pads, the jog wheel and the buttons below |
| Jog wheel | Steps 1 frame, steps 5 frames, jumps between edit points, or zooms. Screen button 3 changes the mode |
| Play | Play/stop (Space) |
| Arrow buttons | Frame back/forward (left/right), previous/next edit (up/down) |
| Delete / Copy / Insert | Clear, copy, paste |
| Screen buttons | 1 Layer back · 2 Layer forward · 3 Jog mode · 4 Mac/Win · 5 Reload profile · 8 Close |

Pads with a shortcut are backlit, and a pressed pad turns red on screen. The original Tracker's pad lights are one color with adjustable brightness.

**Mac or Windows:** screen button 4 switches between them, swapping Cmd for Ctrl and Option for Alt. Your choice and jog mode are saved in `i2/Controller/settings.txt`.

### Premiere Pro profile

The profile uses **Premiere's default shortcuts**, so nothing has to be imported into Premiere.

**Layer 1: Edit**
- **Playback:** Play/Stop, J, K, L, Play Around
- **In/Out:** Mark In, Mark Out, Mark Clip, Clear In/Out, Go to In, Go to Out, Match Frame
- **Editing:** Add Edit, Add Edit to All Tracks, Ripple Delete, Lift, Extract, Insert, Overwrite, Ripple Trim Previous/Next (Q/W), Extend Edit, Undo, Redo
- **Moving around:** Previous/Next Edit, back/forward 1 frame and 5 frames, Sequence Start/End
- **Markers:** Add, Next, Previous, plus the Snap toggle
- **Tools:** Selection, Track Select Forward, Ripple Edit, Rolling Edit, Rate Stretch, Razor, Slip, Slide, Pen, Hand
- **Zoom:** in, out

**Layer 2: Clips & Panels**
- **Clipboard:** Copy, Paste, Paste Insert, Paste Attributes, Cut, Duplicate, Select All, Deselect All
- **Clips:** Group, Ungroup, Link/Unlink, Enable Clip, Speed/Duration, Make Subclip, Clear
- **Transitions:** video, audio, default
- **Audio:** Audio Gain, clip volume ±1 dB and ±6 dB
- **Panels:** Project, Source, Timeline, Program, Effect Controls, Audio Track Mixer, Effects, Media Browser, Audio Clip Mixer
- **View:** Zoom to Sequence, Maximize Panel, Full Screen
- **File:** Import, Export Media, Save, Save As, New Bin, Find
- **Keys:** Esc, Enter, Tab

### Editing a profile

On first launch the Tracker writes `i2/Controller/premiere.txt` to the SD card. Edit it on a computer, put the card back, and press **Reload**. No new firmware is needed.

```text
NAME Premiere Pro
LAYER Edit
1  Play/Stop    | SPACE
13 Add Edit     | CMD+K
9  Clear In/Out | OPT+X | CTRL+SHIFT+X     <- optional separate Windows combo
```

- Pads are numbered 1–48, left to right, top row first.
- **Modifiers:** `CMD` (Cmd on Mac, Ctrl on Windows), `CTRL`, `SHIFT`, `OPT` or `ALT`, `WIN`.
- **Keys:**
  - letters and numbers: A–Z, 0–9
  - function keys: F1–F24 (F13–F24 are handy for custom Premiere shortcuts)
  - other keys: SPACE, ENTER, ESC, TAB, BKSP, DEL, INS, HOME, END, PGUP, PGDN, UP, DOWN, LEFT, RIGHT, BACKSLASH
  - symbols: `- = [ ] ; ' , . /` and the grave accent
- Limits: up to 8 layers, labels up to 15 characters.

**First time on Windows:** if the pads don't type, unplug and replug the Tracker once. The device version was bumped so Windows reads the new keyboard.

### Hardware test checklist

1. The Tracker boots, plays a project, and still shows up as MIDI on the computer.
2. Config > i2 > Controller opens, and the grid and bottom labels look right.
3. Pads light where a shortcut exists, and a pressed pad turns red on screen.
4. In a text editor, pads type the expected keys. For example, pad 6 types `i`.
5. In Premiere, with the timeline focused, try Play, J/K/L, Add Edit, Ripple Delete and markers.
6. The Mac/Win switch makes Add Edit send Cmd+K on Mac and Ctrl+K on Windows.
7. Try the jog wheel in each mode, and Shift + jog.
8. Leave and reopen Controller mode, and check that settings survive a reboot.

## i2 Arcade

Two KAGEMORI CLASSICS boards from i2 OS 2000, rebuilt for the Tracker. **Open it:** Config > **i2** > **Arcade**. Pick a game with pad row 1 or 2 (or the jog wheel + Play). **Menu** (screen button 8) goes back; **Exit** leaves the arcade. Scores save to `i2/Arcade/` on the SD card.

### Quiz Kagemori (BRAIN BANZAI!)

The host MR. MARU, you and three CPU contestants (PROF PIP, DEE-DEE, BIG BORO, NANA SUMI), six rounds:

1. **Buzzer round:** buzz in early for double points.
2. **Odd One Out, True Story or Category Blitz:** everyone answers at once, and faster answers score more.
3. **Bonus round:** i2 World questions, buzzer style.
4. **The second** of Odd One Out, True Story or Category Blitz.
5. **Fast Finger:** put four things in order.
6. **Final Wager:** the trailing player picks the category, and everyone bets.

**Controls:**
- The **four pad rows are the four answers**.
- Play or any pad buzzes in.
- Screen button 1 is the 50:50 lifeline.
- The jog wheel sets your wager.

**Question bank:** the 977 questions (plus 50 Fast Finger, 44 Odd One Out and 33 True Story sets) are i2 interactive's own content and are **not in this repo**. Copy the `Quiz` folder you were given to `i2/Arcade/Quiz/` on the SD card. `tools/i2arcade/extract_quiz.py` rebuilds it from i2 OS 2000's `kquiz.js`.

The Picture and Music rounds aren't in this edition yet.

### Medal Mountain

A two-tier medal pusher, with the same tuning as the original:

- **Shelves and legs:** shelf stroke, five 40-second legs, and tickets ×1/×1/×2/×2/×3.
- **CHECK gate:** drop a medal through it to spin the mountain reels.
- **Reel prizes:** medal rain, bonus balls, the Sky Roulette, the Cable Car Bonus and THE AVALANCHE.
- **Other payouts:** waterfall bonuses, the Lucky Lane and the Medal Shower.

**Controls:**
- **Moving the chute:** the jog wheel, or the left/right buttons.
- **Dropping medals:** Play drops one, and holding Play keeps feeding.
- **Pads:** press any pad to send the chute to that column and drop.
- **Bonuses:** Play or a pad stops the roulette and the cable car.
- **Pad lights:** the lit column shows where the chute is.

Leftover medals go back to your bank. If you run dry, the Medal Shower gives you 10 free.

Medals slide in a single layer (the original also stacks them), so the physics fits the Tracker's 180 MHz chip.

**Not in this edition yet:** sound effects, 2-player mode, the Prize Counter, and the Picture and Music rounds.

### Testing the games on a computer

`tools/i2arcade/host/` runs the same game code on a PC with scripted input. It renders every frame to PNG, and checks each layer against the graphics chip's limits: 4 KB of commands and 8 KB of display list per control.

```sh
python3 tools/i2arcade/extract_quiz.py path/to/kquiz.js /tmp/sd     # quiz bank for the fake SD card
sh tools/i2arcade/host/run.sh quiz  /tmp/sd tools/i2arcade/host/quiz_phases.txt
sh tools/i2arcade/host/run.sh medal /tmp/sd tools/i2arcade/host/medal_shots.txt
```

### Arcade test checklist (on the Tracker)

1. Config > i2 > Arcade opens the menu. Pad rows 1 and 2 open each game, and Menu/Exit work.
2. **Quiz:**
   - With no bank on the card, it shows "QUESTION BANK NOT FOUND".
   - With the bank, a full show plays through to the result.
   - Text wraps inside the boxes, the timer bar shrinks, and podium scores update.
3. **Medal Mountain:**
   - The shelves glide and medals pour off the front.
   - The CHECK gate lights and the reels spin.
   - Roulette and cable car stop when you press.
   - The frame rate feels smooth enough. If it's choppy, note which moments.
4. Rounded boxes and lines look right (corner radius and line thickness), and nothing flickers.
5. After a session, the medal bank and quiz best score are still there following a reboot.

## Roadmap

- **Arcade:**
  - sound effects
  - more KAGEMORI ports (Safari Snap with the pads as a light-gun grid, Tanuki Tumble, Boneyard Bumper)
  - the pixel engine for the 384×224 games
- **Controller:**
  - a profile switcher
  - profiles for i2 OS 2000 and i2 Creative Hub
- **MIDI mode,** for two-way control with web apps (Web MIDI)
- **i2 look:** theme, font and boot screen, plus i2 Sample Editor effects
- **A small helper app** that shows Premiere's timecode and clip name on the Tracker's screen

## Where the i2 code lives

| Path | What |
|---|---|
| [`core/interface/i2Controller/`](../core/interface/i2Controller/) | Controller mode: screen, pad grid, profile parser, default Premiere profile |
| [`core/interface/i2Arcade/`](../core/interface/i2Arcade/) | i2 Arcade: the module, the FT812 renderer, Quiz Kagemori and Medal Mountain |
| [`tools/i2arcade/`](../tools/i2arcade/) | Quiz bank extractor and the host test harness |
| [`platforms/mk66/source/UsbKeyboardImpl.*`](../platforms/mk66/source/) | USB keyboard driver for the original Tracker |
| [`platforms/mk66/3rdparty/teensy_core/usb_desc.h`](../platforms/mk66/3rdparty/teensy_core/usb_desc.h) | USB layout. Keyboard is interface 3, RawHID moves to 4. Build with `-DI2_NO_USB_KEYBOARD` for stock |
| [`core/include/Platform.h`](../core/include/Platform.h) | `UsbKeyboard` interface. Tracker+ and Mini return `nullptr` |
| [`core/interface/configEditor/`](../core/interface/configEditor/) | The "i2" group in the Config menu (Controller, Arcade) |
| [`.github/workflows/build.yml`](workflows/build.yml) | CI. Builds are named `i2_<commit>` |

## Hardware notes: original Tracker (MK66)

| Part | What it is | Notes |
|---|---|---|
| CPU | NXP Kinetis MK66FX1M0, Cortex-M4F, 180 MHz | About ⅓ the speed of Tracker+ / Mini |
| Flash | 1 MB, about 78 KB free with Controller mode and the Arcade | New code has to stay lean |
| RAM | 256 KB internal, about 87 KB free | |
| SDRAM | 16 MB, about 1.1 MB free | Controller mode borrows the shared 768 KB scratch area instead |
| Display | 7" 800×480, FTDI FT812 | 65 display controls, 8 KB of display memory each |
| Input | 48 backlit pads (4×12), 33 buttons, jog wheel | Pad lights are one color, brightness only |
| Audio | SGTL5000 codec, mono line in, mic in, stereo out | Mono samples on this model |
| Radio | Si4703 FM **receiver** with RDS | Receive only. The headphone cable is the antenna |
| I/O | MIDI in/out (minijack), USB-C MIDI, microSD | No Wi-Fi, Bluetooth or USB audio |

## Staying in sync with Polyend

This repo is a **fork**: a copy of Polyend's repo that keeps a link back to it. When Polyend pushes updates, GitHub shows "This branch is N commits behind polyend/tracker-ce" on the repo page. Click **Sync fork** to pull them in. If Polyend changed a file that i2 also changed, GitHub asks for the conflict to be fixed first.

The i2 changes stay in their own files where possible, and this README lives in `.github/` so Polyend's `README.md` never conflicts.

## Build

GitHub Actions builds all three models on every push to `main`. To build locally, install GNU Arm Embedded Toolchain 10.3-2021.10 and run `make original`. The file appears as `build/PolyendTracker_*.ptf`. Polyend's full developer docs are in [`README.md`](../README.md) and [`docs/`](../docs/).

## License

MIT, inherited from Polyend's Tracker CE, plus Polyend's restriction on selling a wholesale commercial clone of the full Tracker firmware. The precompiled `synth/lib/*.a` files have their own license. See [`LICENSE`](../LICENSE).

Not affiliated with or endorsed by Polyend.

## Contributing upstream

Polyend's PR check rejects AI `Co-authored-by` trailers. Strip them before opening a PR to `polyend/tracker-ce`.
