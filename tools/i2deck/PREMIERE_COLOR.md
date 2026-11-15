# GRADE, Premiere side: work notes on Premiere's Color Mode

Research for the PREMIERE half of the GRADE deck (`gradepr.txt`), November 2026.
Premiere 27.0 shipped Color Mode (public beta from April 2026): a grading
workspace next to Edit, with its own keyboard layout.

Legend: **OK** = stated in an Adobe help page or by Adobe staff (source given).
**??** = a guess, marked `[??]` in the deck file. **SET** = no default exists
or none is published, marked `[SET]` in the deck file: Ian assigns the key in
Premiere, or replaces it with Premiere's.

## Keys

| Deck key | Command | Mac / Win | Status | Source |
|---|---|---|---|---|
| PREV / NEXT, dial CLIPS | Previous / next clip | Up / Down | OK | [Color mode workflow](https://helpx.adobe.com/premiere/desktop/correct-color/color-mode-fundamentals/color-mode-workflow.html): "press the Down or Up arrow keys (Go to next or Previous clip)"; [Clip Grid](https://helpx.adobe.com/premiere/desktop/correct-color/color-mode-fundamentals/working-with-the-clip-grid.html) |
| NUDGE pads, dials NUDGE V / H | Nudge a latched control | Arrow keys | OK | [Color Controls panel](https://helpx.adobe.com/premiere/desktop/correct-color/color-mode-fundamentals/color-tools-panel-overview.html): "Use the Arrow keys to nudge the vertical or horizontal parameters for that control." Adobe staff (A. Van Hurkman, Sep 30 2026): the arrows give "keyboard driven adjustment similar to printer point controls" ([forum](https://community.adobe.com/announcements-732/available-now-in-27-0-introducing-color-mode-1557519)) |
| BIG pads, dials BIG V / H | Shift Nudge (bigger step) | Shift + arrows | ?? | "Nudge" and "Shift Nudge" are settings in Color Control options (Color Controls panel page); that Shift + arrow uses the Shift Nudge step is an inference |
| OK | Accept and unlatch | Return | OK | Color mode workflow: "click again (or press Return or Escape) to unlatch the control you were working on and accept the change" |
| ESC | Accept and unlatch | Esc | OK | same |
| HUD (hold) | Hide the HUD while held | H | OK | [Adjust operation](https://helpx.adobe.com/premiere/desktop/correct-color/color-mode-fundamentals/default-adjust-operation.html): "hold the H key down while making the adjustment" |
| CMPR | Toggle Comparison View | Cmd+3 / Ctrl+3 | OK | [Comparison View](https://helpx.adobe.com/premiere/desktop/correct-color/color-mode-fundamentals/about-comparison-view.html): "Ctrl + 3 (Windows) or Command + 3 (macOS)" |
| REF, SWAP, SPLT | Add Color Reference Marker, Swap Sides, Split | (none) | SET | Comparison View page: these "don't have shortcuts assigned by default". Deck sends Ctrl+Opt+M / S / H (Mac: Control+Option; Win: Ctrl+Alt) |
| EDIT / COLR / EXPT | Window > Mode | Opt/Alt+2 / 3 / 4 | ?? | ProVideo Coalition ([Burning Questions](https://www.provideocoalition.com/burning-questions-about-adobe-premiere-betas-new-dedicated-color-mode/)): modes "default to option+ 1, 2, 3, 4" (beta). Which number is which mode is not stated anywhere I could reach; the deck assumes 1 Import, 2 Edit, 3 Color, 4 Export. Check Window > Mode |
| LATCH block (16 pads) | Latch Contrast, Exposure, Temperature, Balance, Saturation, Hue, Lum Shift, Texture, Sharpness, Pivot, Black, Tint, Saturation Shift, Zones, Global, Auto Color | (unknown) | SET | Latch keys exist: Color Controls page ("Press the keyboard shortcut corresponding to the control you want to use"); Adobe staff: "pre-mapped keyboard shortcuts for latching into the different controls". The defaults are not in any page I could read. Deck sends Ctrl+Opt+1-0, B, T, U, Z, G, A. Control names: Adjust operation page |
| PLAY | Play / stop | Space | ?? | Edit Mode default; Color Mode layout unconfirmed |
| REV / STOP / FWD | Shuttle | J / K / L | ?? | Edit Mode defaults |
| UNDO / REDO | Undo / redo | Cmd+Z / Cmd+Shift+Z | ?? | Edit Mode defaults |
| MARK | Add Marker | Cmd+M / Ctrl+M | ?? | The Comparison View page (as summarised by the fetch tool) mentions Add Marker on Ctrl/Cmd+M; not confirmed in Premiere |

Other facts used:
- Color Mode has its own keyboard layout, separate from Edit Mode, under the
  same preset; the same key can do different things in each mode. (ProVideo
  Coalition, [Adobe ships Premiere 27 with Color Mode](https://www.provideocoalition.com/adobe-ships-premiere-27-with-color-mode/), Nov 10 2026.)
- Shift while dragging constrains a 2D control; Option changes drag speed.
  Those are mouse modifiers, so the deck doesn't use them. (Color Controls page.)
- You enter Color Mode by clicking the Color tab next to Edit in the header.
  (Adobe help pages; [Premiere 27 announcement](https://community.adobe.com/announcements-727/new-in-adobe-premiere-27-0-meet-color-mode-1655034).)
- Adobe's default-shortcuts page has no Color Mode table. Its sections are the
  File through Window menus and the classic panels
  ([Default keyboard shortcuts](https://helpx.adobe.com/premiere/desktop/get-started/keyboard-shortcuts/default-keyboard-shortcuts.html)).
- Not found anywhere: shortcuts for stills (Color Mode may not have a stills
  gallery as Resolve does; it uses reference markers plus Comparison View),
  operation bypass (the eye icon in the Color Controls panel), and
  adding Adjust / Style / LUT operations.

## Control surfaces: can the wheel drive Color Mode directly?

No, not from the Tracker. It sends keys only.

- Premiere's built-in control-surface support is **EUCON** and **Mackie
  Control** (Preferences > Control Surface). The [Premiere SDK guide](https://premiere-plugin-sdk-guide.readthedocs.io/control-surfaces/control-surfaces.html)
  describes that support as "EUCON and Mackie devices to control audio mixing
  and basic transport controls". The Tracker could act as a Mackie device over
  USB MIDI, but Mackie Control reaches the mixer and transport, not colour.
- Colour panels connect through a control-surface **plugin**. The SDK has a
  Lumetri Color base class, and that is how Tangent's mapping works through
  Tangent Hub. Adobe staff said in Sep 2026 that support for "color control
  surfaces such as those made by Tangent" would ship with Premiere 27. In the
  27.0 announcement thread, a user reports Tangent has released a mapping for
  Elements and Wave. ProVideo Coalition reports that "third-party hardware
  control services will be supported".
- The Tracker can't use that path. A Tangent panel talks to Tangent's own Hub
  software, and Premiere talks to the Hub. Pretending to be a Tangent panel
  would need Tangent's USB protocol and IDs. A Premiere control-surface plugin
  that read the Tracker's MIDI is possible in principle (C++ SDK), but it is a
  native plugin per OS and is out of scope here.
- **So the wheel uses keys:** latch a control (pad or click), then NUDGE V /
  NUDGE H send the arrow keys. Adobe describes that as working like printer
  points. With nothing latched, NUDGE V steps through clips.

## What Ian should check in Premiere (5 minutes)

1. Window > Mode: note the keys next to Edit, Color and Export, and fix the
   EDIT / COLR / EXPT lines in `i2/Deck/gradepr.txt` if they differ.
2. Keyboard Shortcuts > pick the Color Mode layout. Search for the latch
   commands (Contrast, Exposure, ...). Either give them Ctrl+Opt+1 ... as in the
   deck, or copy Premiere's keys into the LATCH lines of `gradepr.txt`.
3. In the same layout, give Swap Sides, Split, and Add Color Reference
   Marker the keys in the deck (Ctrl+Opt+S / H / M), or change the deck.
4. Check Space, J/K/L, Cmd+Z and Cmd+M in Color Mode; fix any that differ.
