# Installing Tracker i2 edition

A start-to-finish guide: from a stock Tracker to i2 DECK. Plan on 20–30 minutes the first time.

> **Which Tracker?** This firmware is for the **original Polyend Tracker** (the 2020 desktop model). Builds for Tracker+ and Tracker Mini exist, but i2 DECK only appears on the original.

> **Not yet tested on hardware.** This build has only been run on a PC test harness. Do Part 2 (backup) and Part 3a (official firmware on the card) before anything else.

> **Is it safe?** The Tracker's bootloader sits on its own chip and is never touched by a firmware update. If a build ever misbehaves, Part 7 gets you back to official firmware in a minute.

---

## What you need

- An original Polyend Tracker and its microSD card
- A computer with an SD card reader (or the USB adapter that came with the Tracker)
- A USB-C cable to your computer
- A free GitHub account, needed to download builds from the Actions tab (Part 3)

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

1. Sign in to GitHub.
2. Open the repo's **[Actions tab](https://github.com/i2interactive/tracker-i2-edition/actions/workflows/build.yml)**.
3. Click the newest run with a **green check mark**.
4. Scroll down to **Artifacts** and click **polyend-tracker-ce-firmware**. A zip file downloads.
5. Unzip it. Inside are three files. Yours is the one starting with **`PolyendTracker_i2_`**, for example `PolyendTracker_i2_9267e94.ptf`. Ignore the `TrackerPlus_` and `TrackerMini_` files.
6. Copy that file to the card's `Firmware` folder.

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
4. The Tracker should start on the **i2 DECK home screen** ("PICK A DECK"). Press **screen button 5 (MUSIC)** to check the normal Tracker is still there, then **Config > i2 > Deck** to come back.

---

## Part 6: Use i2 DECK

1. Connect the Tracker to your computer with a USB-C cable. It appears as a USB keyboard alongside its normal MIDI port, and needs no driver.
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
| Turn the page (CUT) | Screen button 7 |
| Add Shift to a key or wheel step | Hold Shift |
| Open the normal Tracker | MUSIC on the home screen, or any Tracker button (Pattern, Song, Config...) |
| Come back to i2 DECK | Config > i2 > Deck |

**The four decks**

| Button | Deck | For | Notes |
|---|---|---|---|
| 1 | CUT | Premiere Pro | Premiere's default shortcuts. Two pages |
| 2 | GRADE | DaVinci Resolve, colour page | Resolve's default shortcuts. Press **LITE** once, then the wheel trims MASTER / RED / GREEN / BLUE |
| 3 | HUB | i2 Creative Hub / ComfyUI | Sends MIDI notes. The Hub doesn't listen yet, so this one is waiting on the Hub |
| 4 | STATIC | The built-in FM radio | Plug in headphones (the cable is the antenna). Seek, store stations with Shift + a station pad |

**Changing what a pad does (optional)**
1. The first time a deck opens, it writes itself to the SD card: `i2/Deck/cut.txt`, `grade.txt`, `hub.txt`, `static.txt`.
2. Edit the file on a computer. The format is explained at the top of the file and in the [README](README.md#making-your-own-deck).
3. Put the card back and open the deck again.

If you've changed shortcuts in Premiere or Resolve, edit the deck to match.

**Starting in the normal Tracker instead:** open `i2/Deck/settings.txt` on the card and change `boot=deck` to `boot=music`.

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
- To skip i2 DECK at start-up, put a file `i2/Deck/settings.txt` on the card containing the line `boot=music`.

**Pads type the wrong keys**
- Check the top-right corner of the screen matches your computer (MAC or WIN). Screen button 7 on the home screen switches it.
- Click on the timeline first, because many shortcuts only work when the timeline has focus.
- The decks assume the app's default shortcuts.

**The top-right corner says NO USB**
- The computer hasn't picked up the Tracker as a keyboard. Check the cable, then unplug and replug once.

---

## Updating later

1. Download the newest build (Part 3b).
2. Copy it to `Firmware`. You can delete the older `PolyendTracker_i2_` file.
3. Run **Config > Firmware > Firmware update** (Part 5).

Your decks and settings stay on the SD card. If an update changes a built-in deck, delete its file in `i2/Deck/` to get the new version.

---

Found a problem? Open an issue on this repo. Include your firmware build (Config > Firmware) and what you pressed.
