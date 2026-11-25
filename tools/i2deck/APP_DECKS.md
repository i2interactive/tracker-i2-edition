# MIXER, FILE and SETUP: research notes

Work notes for the three decks on MASTER, FILE and CONFIG (v0.4, November
2026): what each key sends in Premiere Pro (26 / 27) and DaVinci Resolve
(20 / 21), where it comes from, and how sure it is. The same marks appear in
the deck files on the card:

- **OK**: stated in Adobe's or Blackmagic's own pages, or (Resolve page and
  panel keys) in two independent shortcut lists that agree.
- **??**: a guess, or from a single third-party list. Check it once in the
  app's keyboard shortcut editor and fix the deck file if it differs.
- **SET**: the app has no default key for the command (or none is
  published). The deck sends **Control + Option + Shift + a letter**
  (Windows: **Ctrl + Alt + Shift + the letter**). Give the command that key
  in Premiere's Keyboard Shortcuts or Resolve's Keyboard Customization, or put
  the app's own key in the deck file.

The three decks share one setting, `app=premiere` or `app=resolve` in
`i2/Deck/settings.txt`, switched with screen button 6 in any of them. GRADE
keeps its own `grade=` (you might grade in Resolve and cut in Premiere).

Files on the card (written the first time each side opens, `REV 2` since v0.5):
`mixerpr.txt`, `mixerres.txt`, `filepr.txt`, `fileres.txt`, `setuppr.txt`,
`setupres.txt`.

## MIXER: Mackie Control over USB MIDI

Both apps take a Mackie Control (MCU) surface, so MIXER **is** one: the
Tracker's existing USB MIDI port speaks the MCU protocol while MIXER is open.
No driver, no extra software.

### Does the host take it?

| Host | Mackie Control? | What it drives | Source |
|---|---|---|---|
| Premiere Pro | Yes: Settings (Windows: Preferences) > **Control Surface** > Add > **Mackie**, then Edit and pick the MIDI Input Device and MIDI Output Device | Audio Track Mixer: faders, pan, mute, solo, transport (play, record), banks. "Faders, pan/balance, mute, and solo controls" | [Adobe: Control Surface preferences](https://helpx.adobe.com/premiere-pro-next/learn-the-basics/configure-preferences/control-surface-preferences.html) (EUCON or Mackie; MIDI in/out); [Premiere SDK](https://premiere-plugin-sdk-guide.readthedocs.io/control-surfaces/control-surfaces.html) ("EUCON and Mackie devices to control audio mixing and basic transport controls"); [X-Touch thread](https://community.adobe.com/t5/premiere-pro-discussions/pp-and-the-behringer-x-touch-universal-control-surface/td-p/9935117) (faders and pan per track, Play, Record writes fader moves, Track Mixer only); [bank thread](https://community.adobe.com/questions-729/control-surface-bug-no-bank-controls-in-clip-mixer-1406962) (Next/Previous Bank work in the Track Mixer, not the Clip Mixer; motor faders follow the selected track in clip mixer mode) |
| DaVinci Resolve | Yes: Preferences > System > **Control Panels**: tick **Use MIDI Audio Console**, MIDI Protocol **MCU**, MIDI Input and MIDI Output = the Tracker | Fairlight mixer (the same mixer the Edit page shows): faders, pan, track names, rec enable, solo, mute, select, bank switching; transport (rewind, fast forward, stop, play, record, jog/scrub); markers; depending on the panel: undo/redo, automation modes, nudge | [Resolve 18.6 manual, third-party panels](https://www.steakunderwater.com/VFXPedia/__man/Resolve18-6/DaVinciResolve18_Manual_files/part3715.htm) ("supports HUI- and MCU-compatible third-party mixing control panels with up to eight faders ... connected via USB MIDI"); [Resolve 18.6 manual, preferences](https://www.steakunderwater.com/VFXPedia/__man/Resolve18-6/DaVinciResolve18_Manual_files/part128.htm) (Use MIDI Audio Console, MIDI Protocol HUI or MCU, MIDI Input, MIDI Output); [BMD forum](https://forum.blackmagicdesign.com/viewtopic.php?p=1154375) ("faders will work in both directions"); [BMD forum, X-Touch](https://forum.blackmagicdesign.com/viewtopic.php?p=743041) (faders, pan and transport over MCU; Resolve 19 user: fader automation works, some buttons mapped oddly) |

Not checked for Resolve 20 / 21: the manuals I could reach are 18.6. The
preference has been there since Resolve 14 ([forum](https://forum.blackmagicdesign.com/viewtopic.php?p=533550)); assumed unchanged.

So MIXER's pads, wheel and screen are MCU on both sides, plus one row of
keyboard shortcuts.

### What MIXER sends (Mackie Control protocol)

| Control | Message | Source |
|---|---|---|
| REC 1-8, SOLO 1-8, MUTE 1-8, SELECT 1-8 pads | Note on, channel 1: notes 0-7, 8-15, 16-23, 24-31; velocity 127 on press, 0 on release | [Ardour, device_info.cc](https://github.com/Ardour/ardour/blob/master/libs/surfaces/mackie/device_info.cc) (strip buttons 0x00, 0x08, 0x10, 0x18); [Mackie Control MIDI map](https://www.rncbc.org/drupal/files/Mackie%20Control%20MIDI%20Map.pdf) (127 on press, 0 on release) |
| BANK -8 / +8, BANK -1 / +1 | Notes 46 / 47, 48 / 49 | Ardour (Bank Left 0x2e, Bank Right 0x2f, Channel Left 0x30, Channel Right 0x31) |
| REW FF STOP PLAY REC | Notes 91-95 | Ardour (0x5b-0x5f) |
| LOOP (Cycle), MARK (Marker), UNDO | Notes 86, 84, 81 | Ardour (0x56, 0x54, 0x51) |
| Wheel: FADER (the picked strip), MASTER | Touch note (104 + strip; master 112) on, then pitch bend on channel = strip (master: channel 9), 14 bits, 0 = bottom; touch off 400 ms after the wheel stops. 128 steps bottom to top, Shift 16 | Ardour fader.cc (`0xe0 + id`, low 7 bits, high 7 bits), device_info.cc (Fader Touch 0x68, Master Fader Touch 0x70); MIDI map (pitch wheel per channel, master on 9) |
| Wheel: PAN | CC 16 + strip, relative: 1-63 clockwise ticks, 65-127 anticlockwise (64 + ticks) | Ardour pot.cc / surface.cc (V-Pot base 0x10; bit 6 = direction, bits 0-5 = ticks); MIDI map (V-Pots, "65 as a sign indicator") |
| Wheel: JOG | CC 60, same relative values | MIDI map (scrub wheel CC 60) |
| Wheel: TRACK | Picks the strip FADER and PAN work on (also picked by any strip pad). Sends nothing | |

From the host, while i2 DECK is open (all optional; the deck works one-way):

| Host sends | MIXER shows | Source |
|---|---|---|
| Note on, channel 1 (127 on, 1 blink, 0 off) | The pad and its tile light up: mute, solo, arm, select, transport LEDs | Ardour (LED states) |
| Pitch bend, channels 1-9 | Fader positions (the strips on screen, the FADER / MASTER value); the wheel moves on from there | Ardour fader.cc |
| Channel pressure: strip in the high nibble, level 0-12 in the low | Meters; they fall one step every 120 ms between updates, as on an MCU | Ardour meter.cc (`0xd0, (id << 4) \| level`) |
| CC 48-55 | Pan position on the PAN value (L5 .. C .. R5) | Ardour pot.cc (`0xb0, 0x20 + id` with id from 0x10) |
| Sysex `F0 00 00 66 14 12 <offset> <text> F7` | Track names: the top row of the 2 x 56 display, 7 characters per strip | Ardour surface.cc (`display_line`: sysex header, 0x12, offset 0 or 0x38) |
| Sysex device query `F0 00 00 66 14 00 F7` | Answers `... 01 <serial> <challenge> F7` (device ready); also answers 02 (with 03) and the version request 13 (with 14) | Ardour surface.cc (`say_hello`, 0x01 = MCU device ready, Logic Control challenge / confirmation) |

Without a position from the host a fader starts at 0, and the deck says so
once ("NO FADER FROM HOST: FROM 0"). Track numbers on screen count bank moves
from 1 and are only an estimate; the host's own track names replace them
when it sends its display.

While i2 DECK is open, notes and CCs that arrive over USB go to the deck, not
to the Tracker's instruments (a Mackie host sends its LEDs as notes, which
would otherwise play the Tracker). MIDI clock and song position still reach
the Tracker. Leaving i2 DECK puts the Tracker's own MIDI handling back.

### MIXER keys (no MCU needed)

| Pad / dial | Premiere | Status | Resolve | Status |
|---|---|---|---|---|
| MIX / FAIR | Audio Track Mixer, Shift+6 | OK | Fairlight page, Shift+7 | OK |
| CMIX / EDIT | Audio Clip Mixer, Shift+9 | OK | Edit page, Shift+4 | OK |
| VOL+ / VOL-, dial CLIP VOL | Clip volume +1 / -1 dB: `]` / `[` (Shift: 6 dB) | OK | Clip audio level +1 / -1 dB: Option+Cmd+`=` / `-` (Ctrl+Alt) | ?? |

Sources: [Adobe default shortcuts](https://helpx.adobe.com/premiere/desktop/get-started/keyboard-shortcuts/default-keyboard-shortcuts.html)
(Window menu: Audio Clip Mixer Shift+9, Audio Track Mixer Shift+6),
[Adobe printable Mac list](https://helpx.adobe.com/au/premiere-pro/using/print-premiere-pro-cc-shortcuts-mac.chromeless.html)
(Increase / Decrease Clip Volume `]` `[`, Many: Shift),
[Editors Keys, Resolve 21](https://www.editorskeys.com/blogs/news/davinci-resolve-keyboard-shortcuts-pdf-cheat-sheet)
and [DaVinci Resolve Club](https://davinciresolveclub.com/davinci-resolve-keyboard-shortcuts-guide.md)
(Shift+1-9 pages; clip level Option+Cmd+= / -, one list only).

## FILE

### Premiere (`filepr.txt`)

| Pad | Command | Mac | Windows | Status |
|---|---|---|---|---|
| NPRJ | New Project | Opt+Cmd+N | Ctrl+Alt+N | OK |
| OPEN | Open Project | Cmd+O | Ctrl+O | OK |
| SAVE / SVAS / COPY | Save / Save As / Save a Copy | Cmd+S / Shift+Cmd+S / Opt+Cmd+S | Ctrl+S / Ctrl+Shift+S / Ctrl+Alt+S | OK |
| CLOS / CLPJ | Close / Close Project | Cmd+W / Shift+Cmd+W | Ctrl+W / Ctrl+Shift+W | OK |
| QUIT | Quit (Exit) | Cmd+Q | Ctrl+Q | OK |
| PROP | Get Media File Properties for Selection | Shift+Cmd+H | Ctrl+Shift+H | OK |
| NSEQ | New Sequence | Cmd+N | Ctrl+N | OK |
| NBIN | New Bin (Project panel) | Cmd+B | Ctrl+B | OK |
| IMP / IMB | Import / Import from Media Browser | Cmd+I / Opt+Cmd+I | Ctrl+I / Ctrl+Alt+I | OK |
| PROJ / MBRW | Projects panel / Media Browser | Shift+1 / Shift+8 | same | OK |
| SRC | Open in Source Monitor (Media Browser) | Shift+O | same | OK |
| EXP | Export Media | Cmd+M | Ctrl+M | OK |
| AME | Send to Media Encoder | Opt+Shift+M | Alt+Shift+M | OK |
| REND | Render Effects In to Out | Return | Enter | OK |
| UNDO / REDO, ENT, ESC, TAB, STAB, UP, DOWN | | Cmd+Z / Shift+Cmd+Z, Return, Esc, Tab, Shift+Tab, arrows | Ctrl | OK |
| FRAM | Export Frame | Shift+Cmd+E | Ctrl+Shift+E | ?? (not on Adobe's current page; the same page gives Clip > Enable as Shift+Cmd+E on Mac) |
| EXPM | Export mode (Window > Mode) | Opt+4 | Alt+4 | ?? (see `PREMIERE_COLOR.md`) |
| REVT | File > Revert | Ctrl+Opt+Shift+R | Ctrl+Alt+Shift+R | SET |
| MGMT | File > Project Manager | Ctrl+Opt+Shift+P | Ctrl+Alt+Shift+P | SET |
| LINK / OFFL | Link Media / Make Offline | Ctrl+Opt+Shift+L / O | Ctrl+Alt+Shift+L / O | SET |
| EDL / XML / AAF | File > Export > EDL / Final Cut Pro XML / AAF | Ctrl+Opt+Shift+E / X / F | Ctrl+Alt+Shift+E / X / F | SET |
| RAUD / DREN | Sequence > Render Audio / Delete Render Files | Ctrl+Opt+Shift+A / D | Ctrl+Alt+Shift+A / D | SET |

Source for OK rows: [Adobe default shortcuts](https://helpx.adobe.com/premiere/desktop/get-started/keyboard-shortcuts/default-keyboard-shortcuts.html)
(File, Edit, Sequence, Window, Project panel, Media Browser sections). The
older [printable Mac list](https://helpx.adobe.com/au/premiere-pro/using/print-premiere-pro-cc-shortcuts-mac.chromeless.html)
lists no key for Project Manager, Revert, Render Audio or Delete Render Files.
The SET letters avoid M (Ctrl+Alt+Shift+M is Clear All Markers on Windows).

### Resolve (`fileres.txt`)

| Pad | Command | Mac | Windows | Status |
|---|---|---|---|---|
| PMGR | Project Manager | Shift+1 | Shift+1 | OK |
| SAVE | Save Project | Cmd+S | Ctrl+S | OK |
| IMTL | File > Import Timeline | Shift+Cmd+I | Ctrl+Shift+I | OK ([manual](https://www.steakunderwater.com/VFXPedia/__man/Resolve18-6/DaVinciResolve18_Manual_files/part1403.htm)) |
| MEDP / CUTP / EDIT / DLVR | Media / Cut / Edit / Deliver page | Shift+2 / 3 / 4 / 8 | same | OK |
| UNDO / REDO, dialog keys | | Cmd+Z / Shift+Cmd+Z | Ctrl | OK |
| SVAS | Save Project As | Shift+Cmd+S | Ctrl+Shift+S | ?? |
| NTL / NBIN | New Timeline / New Bin | Cmd+N / Shift+Cmd+N | Ctrl+N / Ctrl+Shift+N | ?? |
| IMP | Import Media | Cmd+I | Ctrl+I | ?? (the list calls Cmd+I "Import Project") |
| EXPJ | Export Project | Cmd+E | Ctrl+E | ?? |
| QUIT | Quit | Cmd+Q | Ctrl+Q | ?? |
| EXTL | Export Timeline (AAF / XML / EDL) | Shift+Cmd+O | Ctrl+Shift+O | ?? (my memory only) |
| NPRJ | New Project | Ctrl+Opt+Shift+N | Ctrl+Alt+Shift+N | SET |
| IMPJ | Import Project | Ctrl+Opt+Shift+J | Ctrl+Alt+Shift+J | SET |
| MGMT / RLNK | Media Management / Relink Media | Ctrl+Opt+Shift+M / L | Ctrl+Alt+Shift+M / L | SET |
| QEXP | Quick Export | Ctrl+Opt+Shift+Q | Ctrl+Alt+Shift+Q | SET |
| ADDQ / RNDR | Add to Render Queue / Render All (Deliver) | Ctrl+Opt+Shift+A / R | Ctrl+Alt+Shift+A / R | SET (Resolve may have defaults; not found) |

Sources: [Editors Keys, Resolve 21](https://www.editorskeys.com/blogs/news/davinci-resolve-keyboard-shortcuts-pdf-cheat-sheet)
and [DaVinci Resolve Club](https://davinciresolveclub.com/davinci-resolve-keyboard-shortcuts-guide.md)
agree on Shift+1-9, Cmd+S, Keyboard Customization; the ?? rows come from one
list ([XP-Pen](https://www.xp-pen.com/blog/davinci-resolve-shortcuts.html)).

## SETUP (the settings deck, on CONFIG)

"SETTINGS" is 8 letters; deck names stop at 7 to fit the home-screen tile,
so the deck is called SETUP.

### Premiere (`setuppr.txt`)

Premiere has no default keys for its settings pages, Project Settings,
Sequence Settings or Project Manager (Adobe's current page and the printable
list show none), so most of this deck is SET: assign the keys once, then
every settings page is one pad away.

| Pad | Command (Mac: Premiere Pro > Settings; Windows: Edit > Preferences) | Key sent (Mac / Windows) | Status |
|---|---|---|---|
| KEYS | Keyboard Shortcuts | Cmd+Opt+K / Ctrl+Alt+K | OK |
| PREF AUD AHW MEDI MCAC PLAY TIME AUTO SURF | General, Audio, Audio Hardware, Media, Media Cache, Playback, Timeline, Auto Save, Control Surface | Ctrl+Opt+Shift+ G U H I C B T V S | SET |
| PSET / SCR | File > Project Settings > General / Scratch Disks | Ctrl+Opt+Shift+J / W | SET |
| SSET | Sequence > Sequence Settings | Ctrl+Opt+Shift+Q | SET |
| PMGR | File > Project Manager | Ctrl+Opt+Shift+P | SET |
| REND / RAUD / DREN | Render In to Out (Return) / Render Audio / Delete Render Files | Return; Ctrl+Opt+Shift+A / D | OK / SET / SET |
| EDIT / COLR / EXPT | Window > Mode: Edit / Color / Export | Opt+2 / 3 / 4 | ?? |
| WSRS | Reset to Saved Layout | Opt+Shift+0 | ?? |
| MAX / FULL | Maximize frame / full screen (as in CUT) | Shift+\` / Ctrl+\` | OK |
| Dialog block | Enter, Esc, Tab, Shift+Tab, arrows, Space, Undo, Save | | OK |

### Resolve (`setupres.txt`)

| Pad | Command | Mac | Windows | Status |
|---|---|---|---|---|
| KEYS | Keyboard Customization | Cmd+Opt+K | Ctrl+Alt+K | OK |
| PSET | Project Settings | Shift+9 | Shift+9 | OK |
| PMGR | Project Manager | Shift+1 | Shift+1 | OK |
| Pages | Media Shift+2, Cut 3, Edit 4, Fusion 5, Color 6, Fairlight 7, Deliver 8 | | same | OK |
| PREF | Preferences | Cmd+, | Ctrl+, | ?? |
| CNMA | Cinema viewer | P | P | ?? |
| TLST | Timeline Settings | Ctrl+Opt+Shift+T | Ctrl+Alt+Shift+T | SET |
| RCOF / RCSM / RCUS | Playback > Render Cache > None / Smart / User | Ctrl+Opt+Shift+O / S / U | Ctrl+Alt+Shift+O / S / U | SET |
| RCCO | Render Cache Color Output (clip) | Ctrl+Opt+Shift+C | Ctrl+Alt+Shift+C | SET |
| DRC | Delete Render Cache > All | Ctrl+Opt+Shift+D | Ctrl+Alt+Shift+D | SET |
| PRXY / OPTM | Use Proxy Media / Use Optimized Media if Available | Ctrl+Opt+Shift+P / Z | Ctrl+Alt+Shift+P / Z | SET |

The wheel in FILE and SETUP moves through dialogs: LIST (Down / Up), FIELDS
(Tab / Shift+Tab), LEFT/RT. On a Mac, Tab only reaches every control with
Keyboard Navigation on (System Settings > Keyboard).

## What to check on the computer (10 minutes)

1. Premiere: Settings > Control Surface > Add > Mackie, Edit: the Tracker as
   MIDI input and output. Open the Audio Track Mixer, open MIXER on the
   Tracker, try MUTE 1, SOLO 1, the FADER wheel, PLAY.
2. Resolve: Preferences > System > Control Panels: Use MIDI Audio Console,
   MCU, the Tracker in and out (restart Resolve if asked). Open Fairlight,
   same test.
3. Keyboard Shortcuts (Premiere) / Keyboard Customization (Resolve): give the
   SET commands their keys (tables above), and check the ?? keys.
