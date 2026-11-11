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
