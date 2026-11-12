# Installing Tracker i2 edition

A start-to-finish guide: from a stock Tracker to Controller mode and the i2 Arcade. Plan on 20–30 minutes the first time.

> **Which Tracker?** This firmware is for the **original Polyend Tracker** (the 2020 desktop model). Builds for Tracker+ and Tracker Mini exist, but Controller mode and the i2 Arcade only appear on the original.

> **Is it safe?** The Tracker's bootloader sits on its own chip and is never touched by a firmware update. If a build ever misbehaves, Part 8 gets you back to official firmware in a minute.

---

## What you need

- An original Polyend Tracker and its microSD card
- A computer with an SD card reader (or the USB adapter that came with the Tracker)
- A USB-C cable, for Controller mode
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
4. Check: **Config > Firmware** should show the new build, and the Config menu now has an **i2** group with **Controller** and **Arcade**.

---

## Part 6: Set up Controller mode (Premiere Pro)

1. Connect the Tracker to your computer with a USB-C cable. It appears as a USB keyboard alongside its normal MIDI port, and needs no driver.
   - **Mac:** if a "Keyboard Setup Assistant" window pops up, close it. Nothing needs setting up.
   - **Windows:** the first time, if the pads don't type anything, unplug the Tracker and plug it back in once.
2. On the Tracker, open **Config > i2 > Controller**.
3. Press **screen button 4** until it shows the right system: **OS: Mac** or **OS: Win**. The setting is remembered.
4. Quick test: open a text editor and press **pad 6** (top row, sixth from the left). It should type `i`.
5. Open Premiere Pro and **click on the timeline** so it has focus. Then press **pad 1** (top-left) to play and stop.

**Using it**
- The screen shows what every pad does.
- **Layers:** screen buttons 1 and 2 switch between the **Edit** layer and the **Clips & Panels** layer.
- **Jog wheel:** steps frames. Screen button 3 changes it to 5 frames, edit points or zoom.
- **Shift:** hold it to add Shift to anything you press.
- **Pass-through keys:** Play is play/stop, and the arrow buttons, Delete, Copy and Insert send their usual keys.
- **Leaving:** press **Close** (screen button 8).

**Changing what a pad does (optional)**
1. The first time Controller mode opens, it writes `i2/Controller/premiere.txt` to the SD card.
2. Edit that file on a computer. The format is explained at the top of the file and in the [README](README.md#editing-a-profile).
3. Put the card back and press **Reload** (screen button 5).

The profile uses Premiere's **default** shortcuts. If you've changed shortcuts in Premiere, edit the profile to match.

---

## Part 7: Set up the i2 Arcade

1. **Quiz Kagemori only:** copy the question bank to the SD card so it ends up at:
   ```
   i2/Arcade/Quiz/questions.txt
   i2/Arcade/Quiz/order.txt
   i2/Arcade/Quiz/odd.txt
   i2/Arcade/Quiz/true.txt
   ```
   If you got `i2-Arcade-Quiz-bank.zip`, unzip it at the **top level of the card**. It already contains the `i2/Arcade/Quiz` folders.

   > The question bank is i2 interactive content and is **not included in this repo**. Without it, Quiz Kagemori shows "QUESTION BANK NOT FOUND". Medal Mountain needs nothing extra.

2. On the Tracker, open **Config > i2 > Arcade**.
3. Press a pad in **row 1** for Quiz Kagemori or **row 2** for Medal Mountain. You can also turn the jog wheel and press Play.
4. **Menu** (screen button 8) goes back to the game list, and **Exit** leaves the arcade.

| | Quiz Kagemori | Medal Mountain |
|---|---|---|
| Start | Play or any pad | Play or any pad |
| Main controls | The 4 pad rows = answers 1–4. Play or a pad buzzes in | Jog wheel or arrows move the chute, Play drops a medal, a pad drops in its column |
| Extras | Screen button 1 = 50:50. Jog wheel sets the final wager | Play or a pad stops the roulette and the cable car |

Scores and your medal bank save to `i2/Arcade/` on the SD card.

---

## Part 8: If something goes wrong

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

**Controller mode types the wrong keys**
- Check that **screen button 4** matches your computer (Mac or Win).
- Click on Premiere's timeline first, because many shortcuts only work when the timeline has focus.

**Quiz says "QUESTION BANK NOT FOUND"**
- Check the paths in Part 7. The files must be at `i2/Arcade/Quiz/` on the card, not inside an extra folder.

---

## Updating later

1. Download the newest build (Part 3b).
2. Copy it to `Firmware`. You can delete the older `PolyendTracker_i2_` file.
3. Run **Config > Firmware > Firmware update** (Part 5).

Your Controller profile, settings and arcade scores stay on the SD card.

---

Found a problem? Open an issue on this repo. Include your firmware build (Config > Firmware) and what you pressed.
