# Tracker i2 edition

This is i2 interactive's fork of [polyend/tracker-ce](https://github.com/polyend/tracker-ce), used for R&D on the **original Polyend Tracker** (the 2020 desktop unit). Polyend's own README follows in `README.md`.

## Our target: the original Tracker (MK66)

| Part | What it is | Notes for us |
|---|---|---|
| CPU | NXP Kinetis MK66FX1M0, Cortex-M4F, 180 MHz | About 1/3 the speed of Tracker+ / Mini |
| Flash | 1 MB, about 153 KB free | New code has to stay lean |
| RAM | 256 KB internal, about 128 KB free | |
| SDRAM | 16 MB, about 1.1 MB free (up to about 4.6 MB if `SAMPLE_MEMORY_SIZE` is lowered) | Game or app assets go here or on SD |
| Display | 7" 800×480, FTDI FT812 graphics chip | Hardware JPEG decode, bitmaps in about 1 MB of `RAM_G`. No video out |
| Input | 48 RGB-backlit pads (4×12), 33 buttons, jog wheel | Pads have per-pad LED control |
| Audio | SGTL5000 codec, mono line in, mic in, stereo out | Samples are mono on this model |
| Radio | Si4703 FM **receiver** with RDS (station name, radio text, clock) | Receive only, no transmit. The headphone/line-out cable is the antenna |
| I/O | MIDI in/out (minijack), USB-C MIDI, microSD | No Wi-Fi, Bluetooth or USB audio |

Not on the original: the CHIP / DIRT synths and the Open Synth SDK (`FEATURE_SYNTH` is Tracker+ / Mini only). The `.ptfx` Sample Editor plugins **do** run here, with a 32 KB code limit.

Games: stock firmware had an NES emulator (`/Games/*.nes`). CE removed it because it was proprietary, and its UI slot (`mtInterface.modules[11]` in `core/interface/core/interface.cpp`) is left empty. That slot is where an i2 app or game module goes. `core/interface/imageViewer/` (about 550 lines) is the smallest module to copy as a template.

## Controller mode (i2 edition)

Turns the Tracker into a shortcut controller for Premiere Pro (or any app) over USB-C. Nothing needs installing on the computer: the Tracker shows up as a standard USB keyboard, next to its usual MIDI port.

**Open it:** Config > **i2** > **Controller**. Press **Close** (the far-right screen button) or any module button to leave.

| Control | Does |
|---|---|
| Pads 1–48 | Send the shortcut shown on screen. Keys stay held while the pad is held, so J/L shuttle works. |
| Shift (hold) | Adds Shift to pads, the jog wheel and the buttons below |
| Jog wheel | Steps by frame, by 5 frames, between edit points, or zooms (screen button 3 cycles) |
| Play | Space (play/stop) |
| Arrows | Arrow keys |
| Delete / Copy / Insert | Delete, Copy, Paste |
| Screen buttons | 1 Layer back, 2 Layer forward, 3 Jog mode, 4 Mac/Win, 5 Reload profile, 8 Close |

**Mac or Windows:** screen button 4 switches between them. CMD in a profile means Cmd on Mac and Ctrl on Windows. The choice and jog mode are saved in `i2/Controller/settings.txt`.

**Profiles:** on first launch the Tracker writes `i2/Controller/premiere.txt` to the SD card. It has 2 layers (Edit, then Clips & Panels) using **Premiere's default shortcuts**, so nothing has to be imported into Premiere. Edit the file on a computer to change any pad, then press Reload:

```text
NAME Premiere Pro
LAYER Edit
1  Play/Stop    | SPACE
13 Add Edit     | CMD+K
9  Clear In/Out | OPT+X | CTRL+SHIFT+X     <- optional separate Windows combo
```

- Pads are numbered 1–48, left to right, top row first.
- Modifiers: CMD, CTRL, SHIFT, OPT (or ALT), WIN.
- Keys: A–Z, 0–9, F1–F24, SPACE, ENTER, ESC, TAB, BKSP, DEL, INS, HOME, END, PGUP, PGDN, UP, DOWN, LEFT, RIGHT, BACKSLASH, and the symbols - = [ ] ; ' , . / and the grave accent.
- Up to 8 layers. Labels up to 15 characters.

**First time on Windows:** if the keys don't work, unplug and replug the Tracker once. The USB device version was bumped so Windows reads the new keyboard interface.

**Not yet:** Tracker+ and Mini build fine but have no USB keyboard, so the menu entry only appears on the original Tracker.

### Hardware test checklist

1. Tracker boots, plays a project, and still shows up as MIDI on the computer.
2. Config > i2 > Controller opens; the grid and bottom labels look right.
3. Pads light where a shortcut exists; a pressed pad turns red on screen.
4. In a text editor: pads type the expected keys (e.g. pad 6 types `i`).
5. In Premiere, with the timeline focused: Play, J/K/L, Add Edit, Ripple Delete, markers.
6. Mac/Win switch: Add Edit sends Cmd+K on Mac and Ctrl+K on Windows.
7. Jog wheel in each mode; Shift + jog.
8. Leave and reopen Controller mode; settings stick after a reboot.

## Build

GitHub Actions builds firmware on every push to `main` (`.github/workflows/build.yml`). Get the `.ptf` from the run's artifacts.

Local build: GNU Arm Embedded Toolchain 10.3-2021.10, then `make original`. The output is `build/PolyendTracker_*.ptf`.

## Flash and recover

1. Copy the `.ptf` to `/Firmware/` on the SD card. Keep the filename to 31 characters or fewer.
2. On the Tracker, go to Config > Firmware > Firmware update.
3. **Recovery:** put a known-good `.ptf` in `/Firmware/`, then hold the recessed reset pinhole while powering on. Keep a stock CE `.ptf` on the card at all times.

## License

MIT, plus Polyend's restriction on selling a wholesale commercial clone of the full Tracker firmware. The precompiled `synth/lib/*.a` files have their own license. Individual components can be reused commercially. See `LICENSE`.

## Contributing upstream

Polyend's PR check rejects AI `Co-authored-by` trailers. Strip them before opening a PR to `polyend/tracker-ce`.
