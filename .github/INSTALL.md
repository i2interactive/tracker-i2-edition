# Installing Tracker i2 edition

A start-to-finish guide: from a stock Tracker to i2 DECK. Plan on 20–30 minutes the first time.

> **Which Tracker?** This firmware is for the **original Polyend Tracker** (the 2020 desktop model). Builds for Tracker+ and Tracker Mini exist, but i2 DECK only appears on the original.

> **v0.4.0 runs on a real Tracker** (released Nov 17, 2026). It's still unofficial firmware: do Part 2 (backup) and Part 3a (official firmware on the card) before anything else.

> **From v0.3 the Tracker's File and Config screens are locked, and from v0.4 the Config button in i2 DECK opens the SETUP deck.** Firmware updates live in Polyend's Config, so the way there is: **unlock** (hold Shift + Rec for 2 seconds in i2 DECK), press **PATTERN**, **then Config**. See [Unlocking](#unlocking-file-config-and-firmware-update).

> **Is it safe?** The Tracker's bootloader sits on its own chip and is never touched by a firmware update. If a build ever misbehaves, Part 7 gets you back to official firmware in a minute.

---

## What you need

- An original Polyend Tracker and its microSD card
- A computer with an SD card reader (or the USB adapter that came with the Tracker)
- A **data-rated** USB-C cable to your computer. A charge-only cable powers the Tracker, but the deck shows **NO USB** and the computer sees nothing. If in doubt, use the cable that syncs your phone

---

## Part 1: Check your current firmware

1. Turn on the Tracker.
2. Press **Config** and open **Firmware**. **Current version** shows what you're running, for example 1.9.1.

Write it down. Any 1.9.x version is fine to update from.

---

## Part 2: Back up and prepare the SD card

1. Turn off the Tracker and take out the microSD card.
2. Put the card in your computer and **copy everything on it to a folder on your computer**. That's your backup: projects, samples, everything.
3. Make sure the card has a folder called **`Firmware`** at the top level. Create it if it's missing.

**Only if you're using a new or blank card:** it must be formatted **FAT32** with an MBR partition.
- **Mac:** open Disk Utility and choose View > Show All Devices. Select the card itself (not the volume under it), click Erase, and pick Format **MS-DOS (FAT)** and Scheme **Master Boot Record**.
- **Windows, cards 32 GB or smaller:** right-click the card, choose Format, and pick FAT32.
- **Windows, cards over 32 GB:** use a FAT32 formatting tool such as *fat32format* (GUI version), because Windows' own tool won't offer FAT32.

Then download the **factory SD card content** from [polyend.com/downloads](https://polyend.com/downloads/) (Tracker section), unzip it, and copy its folders to the card.

---

## Part 3: Download the files

You need three firmware files, all ending in `.ptf`. Put all three in the card's `Firmware` folder.

### 3a. Two official files, your safety net

From [polyend.com/downloads](https://polyend.com/downloads/), Tracker section, download:
- **Tracker 1.9.2**: the last official firmware, your way back to stock
- **Tracker Community Edition 1.0** (CE): Polyend's open-source firmware that the i2 edition is built on

Unzip them if they're zipped, and copy the `.ptf` files to the card's `Firmware` folder.

### 3b. The i2 edition

1. Open the **[Releases page](https://github.com/i2interactive/tracker-i2-edition/releases)**. No GitHub account needed.
2. Under the newest release, click **`PolyendTracker_i2_<version>.ptf`** (for example `PolyendTracker_i2_v0.4.0b1.ptf`) to download it. Releases marked **Pre-release** haven't been tested on hardware yet.
3. Test builds of every change are also on the [Actions tab](https://github.com/i2interactive/tracker-i2-edition/actions/workflows/build.yml) (sign in, open a run with a green check, download the **polyend-tracker-ce-firmware** artifact and use the `PolyendTracker_i2_` file). Use those only if asked to test something.
4. Copy that file to the card's `Firmware` folder.

> **Don't rename the file.** The Tracker only lists files whose names start with `PolyendTrac`, end in `.ptf`, and are **31 characters or shorter**.

Your `Firmware` folder should now hold three files:
```
Firmware/
  PolyendTracker_1.9.2.ptf          (official, name may differ slightly)
  PolyendTracker_CE_1.0.0.ptf       (official Community Edition)
  PolyendTracker_i2_xxxxxxx.ptf     (i2 edition)
```

---

## Part 4: Install official Community Edition first (recommended, first time only)

This step confirms your Tracker takes Community Edition before any i2 code is involved. If something goes wrong later, you'll know whether it was the update itself or the i2 build.

1. Put the card back in the Tracker and turn it on.
2. Go to **Config > Firmware > Firmware update**.
3. Select the **CE 1.0** file and confirm.
4. Wait for it to finish and restart. **Don't turn the Tracker off during the update.**
5. Check that it boots, a project loads, and it plays.

---

## Part 5: Install the i2 edition

1. Go to **Config > Firmware > Firmware update** again.
2. Select **`PolyendTracker_i2_...`** and confirm.
3. Wait for it to finish and restart.
4. The Tracker should start on the **i2 DECK home screen** ("PICK A DECK"). Press **screen button 5 (MUSIC)** to check the normal Tracker is still there, then press **Config** to come back (while locked, Config leads to i2 DECK).

**Already on v0.2?** Config isn't locked in v0.2, so update the usual way. After the update, the Tracker is locked.

**Already on v0.3?** On the i2 DECK home screen hold Shift + Rec for 2 seconds (UNLOCKED), press Config (in v0.3 it opens Polyend's Config straight from i2 DECK once unlocked), then Firmware update. From v0.4 on, use the route above: unlock, PATTERN, Config.

---

## Part 6: Use i2 DECK

1. Connect the Tracker to your computer with a data-rated USB-C cable. It appears as a USB keyboard alongside its normal MIDI port, and needs no driver.
   - **Mac:** if a "Keyboard Setup Assistant" window pops up, close it. Nothing needs setting up.
   - **Windows:** the first time, if the pads don't type anything, unplug the Tracker and plug it back in once.
2. On the home screen, press **screen button 7** until the top-right corner shows your computer: **MAC** or **WIN**. It is remembered.
3. Press **screen button 1** to open **CUT**.
4. Quick test: open a text editor and press the pad labelled **IN** (second row, far left). It should type `i`.
5. Open Premiere Pro, **click on the timeline** so it has focus, and press **Play** on the Tracker.

**Finding your way around**

| To do this | Press |
|---|---|
| Send a key | The pad in the same position as the key on screen |
| Change what the jog wheel does | The screen button under the job's name (bottom of the screen) |
| Go back to the home screen | Screen button 8 (HOME), in every deck |
| Jump to CUT, GRADE, HUB or STATIC | NOTE, INSTRUMENT, FX1 or FX2 (from anywhere in i2 DECK) |
| Jump to MIXER, FILE or SETUP | MASTER, FILE or CONFIG (from anywhere in i2 DECK) |
| See MIXER, FILE and SETUP on the home screen | Screen button 8 (MORE); button 8 again (BACK) |
| Turn the page (CUT) | Screen button 7 |
| Add Shift to a key or wheel step | Hold Shift |
| Open the normal Tracker | PATTERN, or MUSIC on the home screen (button 5). Params, Performance, the Sample buttons and Song do nothing in i2 DECK |
| Come back to i2 DECK | Config or File (while locked); Config > i2 > Deck (when unlocked) |
| Switch GRADE between Resolve and Premiere | Screen button 6 in GRADE |
| Switch MIXER, FILE and SETUP between Resolve and Premiere | Screen button 6 in any of them (they switch together) |
| Turn the page (STATIC: TUNER / GLITCH) | Screen button 7 |
| Open Polyend's Config (firmware update) | Hold Shift + Rec together for 2 seconds (unlocks until restart), then PATTERN, then Config |

**The decks**

| Tracker button | Home screen | Deck | For | Notes |
|---|---|---|---|---|
| NOTE | page 1, button 1 | CUT | Premiere Pro | Premiere's default shortcuts. Two pages |
| INSTRUMENT | page 1, button 2 | GRADE | DaVinci Resolve colour page **or** Premiere Color Mode | Button 6 switches. Resolve: press **LITE** once, then the wheel trims MASTER / RED / GREEN / BLUE. Premiere: latch a control, then the wheel nudges it. Some Premiere keys need checking (see below) |
| FX1 | page 1, button 3 | HUB | i2 Creative Hub / ComfyUI | Drives the i2 Creative Hub 1.22+ over MIDI: turn on Settings > MIDI controller (i2 DECK) in the Hub |
| FX2 | page 1, button 4 | STATIC | The built-in FM radio | Plug in headphones (the cable is the antenna); you hear the station. Seek, store stations with Shift + a station pad. Page 2 (GLITCH): hold pads to stutter, crush, tape-stop, scan, filter or freeze the radio |
| MASTER | page 2, button 1 | MIXER | The Premiere Audio Track Mixer **or** the Resolve Fairlight mixer | A Mackie Control surface over USB MIDI: mute / solo / arm / select for 8 tracks, banks, transport; the wheel moves faders and pan. Set the app up once (below) |
| FILE | page 2, button 2 | FILE | Premiere **or** Resolve | New, open, save, import, export, render |
| CONFIG | page 2, button 3 | SETUP | Premiere **or** Resolve | Settings pages, project / sequence settings, keyboard shortcuts, render cache. Some keys need assigning once (below) |
| PATTERN | page 1 or 2, button 5 | MUSIC | The normal Tracker | |

**MIXER, one-time setup in the app**
- **Premiere:** Premiere Pro > Settings (Windows: Edit > Preferences) > **Control Surface** > **Add** > **Mackie**. Select it, click **Edit**, and pick the Tracker as **MIDI Input Device** and **MIDI Output Device**. Open Window > Audio Track Mixer.
- **Resolve:** DaVinci Resolve > Preferences > System > **Control Panels**: tick **Use MIDI Audio Console**, set MIDI Protocol to **MCU**, and pick the Tracker as MIDI Input and MIDI Output. Open the Fairlight page.

**FILE and SETUP, one-time setup in the app (optional).** Keys the apps don't have by default are marked `[SET]` in the deck files. They send **Control + Option + Shift + a letter** (Windows: Ctrl + Alt + Shift + the letter). In Premiere's **Keyboard Shortcuts** (Cmd/Ctrl + Option/Alt + K) or Resolve's **Keyboard Customization** (same keys), give each command the key shown in [`tools/i2deck/APP_DECKS.md`](../tools/i2deck/APP_DECKS.md). Keys marked `[??]` are guesses to check.

**Premiere Color Mode keys (one-time check).** Adobe hasn't published most of Color Mode's default keys, so `i2/Deck/gradepr.txt` marks each key `[OK]`, `[??]` or `[SET]`. In Premiere: check **Window > Mode** against the EDIT / COLR / EXPT pads, and in **Keyboard Shortcuts** (pick the Color Mode layout) give the latch commands (Contrast, Exposure...) the keys the deck sends, or put Premiere's keys in the file. Details: [`tools/i2deck/PREMIERE_COLOR.md`](../tools/i2deck/PREMIERE_COLOR.md).

**Changing what a pad does (optional)**
1. The first time a deck opens, it writes itself to the SD card: `i2/Deck/cut.txt`, `grade.txt` (Resolve), `gradepr.txt` (Premiere), `hub.txt`, `static.txt`, `mixerres.txt` / `mixerpr.txt`, `fileres.txt` / `filepr.txt`, `setupres.txt` / `setuppr.txt` (Resolve / Premiere).
2. Edit the file on a computer. The format is explained at the top of the file and in the [README](README.md#making-your-own-deck).
3. Put the card back and open the deck again.

If you've changed shortcuts in Premiere or Resolve, edit the deck to match.

**Starting in the normal Tracker instead:** open `i2/Deck/settings.txt` on the card and change `boot=deck` to `boot=music`. This only applies with `lock=off`: a locked Tracker always starts in i2 DECK.

### Unlocking File, Config and Firmware update

The Tracker shows only i2 DECK, its decks and MUSIC. Polyend's **File** and **Config** screens are locked. In i2 DECK the File and Config buttons always open the FILE and SETUP decks; Polyend's screens are reached from the normal Tracker.

- **Until the next restart:** in i2 DECK (the home screen is easiest), **hold Shift and Rec together for 2 seconds** (bottom row of the right-hand buttons, side by side). The screen says **UNLOCKED UNTIL RESTART**, and an UNLOCKED chip stays in the top bar. Then press **PATTERN** for the normal Tracker, and **Config** (or File) opens Polyend's screen: **Config > Firmware > Firmware update**. Config > i2 > Deck goes back to i2 DECK. Hold Shift + Rec for 2 seconds again in i2 DECK to lock.
- **For good:** in `i2/Deck/settings.txt` on the card, change `lock=on` to `lock=off`. Then Config in the normal Tracker always opens Polyend's Config (still via PATTERN from i2 DECK).

---

## Part 7: If something goes wrong

**The i2 file isn't in the Firmware update list**
- It must be inside the `Firmware` folder at the top of the card.
- Its name must start with `PolyendTracker_`, end in `.ptf`, and be 31 characters or shorter. Don't rename it.

**The Tracker freezes or won't start after an update (emergency recovery)**
1. Put a known-good `.ptf` (official CE 1.0 or 1.9.2) in the card's `Firmware` folder. Ideally it's the only `.ptf` there.
2. Put the card in the Tracker and turn it on.
3. Press the **recessed pinhole button on the bottom of the Tracker** with a paperclip or SIM tool. The bootloader picks up the file and flashes it.

**Going back to official firmware**
1. Go to **Config > Firmware > Firmware update**.
2. Pick the official **CE 1.0** or **1.9.2** file.

Projects saved under Community Edition may use a newer format that 1.9.2 can't open, so keep your Part 2 backup.

**The Tracker starts but i2 DECK looks wrong or is stuck**
- Press **Pattern** (or any Tracker button) to get to the normal Tracker.
- To skip i2 DECK at start-up, put a file `i2/Deck/settings.txt` on the card containing the two lines `lock=off` and `boot=music`.

**I can't get into Config (locked)**
- Do the unlock combo on the i2 DECK home screen: hold Shift + Rec together for 2 seconds. Then PATTERN, then Config. (Config pressed in i2 DECK opens the SETUP deck, not Polyend's Config.)
- Or put the card in a computer and change `lock=on` to `lock=off` in `i2/Deck/settings.txt` (create the file with just that line if it's missing).
- Or use the pinhole recovery above. It doesn't need Config at all.

**STATIC is silent**
- Plug in headphones: the cable is the antenna. Seek to a strong station.
- Check MON is lit and VOLUME isn't 0.
- Still quiet: open the sample recorder, pick Radio, raise its Gain (saved with the project), and reopen STATIC.

**Pads type the wrong keys**
- Check the top-right corner of the screen matches your computer (MAC or WIN). Screen button 7 on the home screen switches it.
- Click on the timeline first, because many shortcuts only work when the timeline has focus.
- The decks assume the app's default shortcuts.

**MIXER does nothing in Premiere or Resolve**
- Check the one-time setup above: the Tracker must be both the MIDI input and the MIDI output of the Mackie device (Premiere) or MIDI audio console (Resolve). Restart the app after changing it.
- The top-right chip says **MCU LINKED** once the app has sent anything back. **MCU OUT** means the Tracker is sending but has heard nothing yet; pads may still work one-way.
- Premiere: the Audio Track Mixer is what it drives (not the Audio Clip Mixer). Resolve: the Fairlight mixer.

**The top-right corner says NO USB**
- The computer hasn't picked up the Tracker as a keyboard. Most often it's a **charge-only USB-C cable**: it powers the Tracker but carries no data. Swap it for a data-rated cable (one that syncs a phone).
- Then unplug and replug once.

---

## Updating later

1. Download the newest build (Part 3b).
2. Copy it to `Firmware`. You can delete the older `PolyendTracker_i2_` file.
3. On the i2 DECK home screen, unlock: hold Shift + Rec together for 2 seconds.
4. Press **PATTERN**, then run **Config > Firmware > Firmware update** (Part 5).

Your decks and settings stay on the SD card. When an update brings a newer version of a built-in deck, the Tracker keeps your old file as `<name>-old.txt` (for example `static-old.txt`) and writes the new one; copy any edits across. To get a built-in deck back at any time, delete its file in `i2/Deck/`.

---

Found a problem? Open an issue on this repo. Include your firmware build (Config > Firmware) and what you pressed.
