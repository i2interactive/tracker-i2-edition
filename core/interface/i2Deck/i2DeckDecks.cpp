#include "i2DeckApp.h"

// The decks that ship in the firmware. Each is written to the SD card
// (i2/Deck/<name>.txt) the first time it is opened; edit the file there to
// change anything. Delete the file to get the built-in version back.
//
//   DECK <name>            name on screen (7 letters)
//   APP <text>             what it drives
//   LINK KEYS|MIDI|RADIO|MCU
//                          keyboard shortcuts, MIDI, the FM tuner, or a
//                          Mackie Control surface (pads with @ actions;
//                          pads with a shortcut still send keys)
//   REV <n>                version of a built-in deck: when the firmware's
//                          REV is higher than the card's, the card's file
//                          is kept as <name>-old.txt and replaced
//   COLOR <rrggbb>         accent colour
//   PAGE <name>            starts a page (up to 4)
//   BLOCK <1-3> <title>    starts one of the three 4x4 pad blocks
//   K <code> <label> | <combo> [| <Windows combo>]
//                          the next pad in the block, left to right, top
//                          row first. "K -" leaves a pad empty. The label
//                          (up to 23 letters, written as you want it shown)
//                          is the name on the key, in full: it wraps to two
//                          or three lines, and a word too long for the pad
//                          is split with a hyphen. Mixed case fits best.
//                          The code (up to 4 letters) is shown only when
//                          there is no label.
//                          CMD = Cmd on Mac / Ctrl on Windows. OPT = Option / Alt.
//   DIAL <1-7> <name> | <clockwise combo> | <anticlockwise combo>
//                          a job for the jog wheel (screen buttons 1-7 pick it).
//                          Screen button 8 is always HOME. In a deck with more
//                          than one page, button 7 turns the page, so use
//                          dials 1-6 there. In GRADE, MIXER, FILE and SETUP,
//                          button 6 switches between Resolve and Premiere,
//                          so dial 6 is unused.

namespace i2deck
{
namespace
{
const char kCut[] = R"DECK(# i2 DECK - CUT: Adobe Premiere Pro, default keyboard shortcuts
DECK CUT
REV 1
APP Premiere Pro
LINK KEYS
COLOR 7FB2FF

PAGE EDIT
BLOCK 1 PLAY + MARK
K REV  Reverse      | J
K STOP Stop         | K
K FWD  Forward      | L
K LOOP Play Around  | SHIFT+K
K IN   Mark In      | I
K OUT  Mark Out     | O
K CLIP Mark Clip    | X
K CLR  Clear In/Out | OPT+X | CTRL+SHIFT+X
K GOIN Go to In     | SHIFT+I
K GOUT Go to Out    | SHIFT+O
K MTCH Match Frame  | F
K MARK Add Marker   | M
K UNDO Undo         | CMD+Z
K REDO Redo         | CMD+SHIFT+Z
K SNAP Snap         | S
K SAVE Save         | CMD+S
BLOCK 2 CUT + TRIM
K CUT  Add Edit             | CMD+K
K CUTA Add Edit All Tracks  | CMD+SHIFT+K
K RDEL Ripple Delete        | OPT+BKSP | SHIFT+DEL
K LIFT Lift                 | ;
K EXTR Extract              | '
K INS  Insert               | ,
K OVR  Overwrite            | .
K EXT  Extend Edit          | E
K Q    Ripple Trim Previous | Q
K W    Ripple Trim Next     | W
K NUD- Nudge Left           | OPT+LEFT
K NUD+ Nudge Right          | OPT+RIGHT
K SEL  Selection Tool       | V
K RAZR Razor Tool           | C
K RPL  Ripple Edit Tool     | B
K SLIP Slip Tool            | Y
BLOCK 3 MOVE + VIEW
K TOP  Go to Start      | HOME
K END  Go to End        | END
K PRV  Previous Edit    | UP
K NXT  Next Edit        | DOWN
K -5   Back 5 Frames    | SHIFT+LEFT
K +5   Forward 5 Frames | SHIFT+RIGHT
K MK-  Previous Marker  | CMD+SHIFT+M
K MK+  Next Marker      | SHIFT+M
K FIT  Zoom to Fit      | BACKSLASH
K MAX  Maximize Panel   | SHIFT+`
K FULL Full Screen      | CTRL+`
K REND Render In to Out | ENTER
K Z+   Zoom In          | =
K Z-   Zoom Out         | -
K TRK+ Taller Tracks    | CMD+=
K TRK- Shorter Tracks   | CMD+-
DIAL 1 FRAME   | RIGHT | LEFT
DIAL 2 JUMP 5  | SHIFT+RIGHT | SHIFT+LEFT
DIAL 3 CUTS    | DOWN | UP
DIAL 4 ZOOM    | = | -
DIAL 5 MARKERS | SHIFT+M | CMD+SHIFT+M
DIAL 6 VOLUME  | ] | [

PAGE CLIPS
BLOCK 1 CLIPS
K COPY Copy             | CMD+C
K PSTE Paste            | CMD+V
K PINS Paste Insert     | CMD+SHIFT+V
K ATTR Paste Attributes | CMD+OPT+V
K CUT  Cut              | CMD+X
K ALL  Select All       | CMD+A
K NONE Deselect All     | CMD+SHIFT+A
K DEL  Clear            | BKSP
K GRP  Group            | CMD+G
K UGRP Ungroup          | CMD+SHIFT+G
K LINK Link             | CMD+L
K ENBL Enable           | SHIFT+E
K SPD  Speed / Duration | CMD+R
K GAIN Audio Gain       | G
K SUB  Make Subclip     | CMD+U
K FIND Find             | CMD+F
BLOCK 2 FX + FILE
K VTRN Video Transition    | CMD+D
K ATRN Audio Transition    | CMD+SHIFT+D
K DTRN Default Transitions | SHIFT+D
K -
K +1DB Volume +1 dB        | ]
K -1DB Volume -1 dB        | [
K +6DB Volume +6 dB        | SHIFT+]
K -6DB Volume -6 dB        | SHIFT+[
K IMP  Import              | CMD+I
K EXP  Export Media        | CMD+M
K SAVE Save                | CMD+S
K BIN  New Bin             | CMD+B
K ESC  Escape              | ESC
K ENT  Enter               | ENTER
K TAB  Tab                 | TAB
K -
BLOCK 3 PANELS
K PROJ Project           | SHIFT+1
K SRC  Source Monitor    | SHIFT+2
K TIME Timeline          | SHIFT+3
K PGM  Program Monitor   | SHIFT+4
K FXC  Effect Controls   | SHIFT+5
K MIX  Audio Track Mixer | SHIFT+6
K FX   Effects           | SHIFT+7
K MED  Media Browser     | SHIFT+8
DIAL 1 FRAME   | RIGHT | LEFT
DIAL 2 JUMP 5  | SHIFT+RIGHT | SHIFT+LEFT
DIAL 3 CUTS    | DOWN | UP
DIAL 4 ZOOM    | = | -
DIAL 5 MARKERS | SHIFT+M | CMD+SHIFT+M
DIAL 6 VOLUME  | ] | [
)DECK";

const char kGrade[] = R"DECK(# i2 DECK - GRADE: DaVinci Resolve colour page, default keyboard shortcuts.
# Resolve has no shortcuts or MIDI for Lift / Gamma / Gain, so the wheel
# trims OFFSET through Resolve's printer lights (the number pad keys).
# 1. Press Printer Lights (LITE: Color > Printer Light Hotkeys). Resolve
#    turns them off again when it restarts or you change project, so press
#    it again then.
# 2. Pick OFS ALL / OFS RED / OFS GRN / OFS BLU and turn. The Offset wheel
#    and the printer lights readout move.
# On Windows the deck turns Num Lock on by itself (with Num Lock off the
# number pad sends arrows and Home/End instead).
# Screen button 6 switches GRADE to Premiere (that side is gradepr.txt).
DECK GRADE
REV 5
APP DaVinci Resolve
LINK KEYS
COLOR FFB454

PAGE COLOR
BLOCK 1 NODES
K SER  Serial Node          | OPT+S
K PAR  Parallel Node        | OPT+P
K LAYR Layer Node           | OPT+L
K OUTS Outside Node         | OPT+O
K CIRC Circle Window        | OPT+C
K LIN  Linear Window        | OPT+Q
K POLY Polygon Window       | OPT+G
K CURV Curve Window         | OPT+B
K ND-  Previous Node        | SHIFT+OPT+;
K ND+  Next Node            | SHIFT+OPT+'
K NODE Node On / Off        | CMD+D
K BYP  Bypass Grades        | SHIFT+D
K RST  Reset Node           | SHIFT+HOME
K RSTG Reset Grade          | CMD+SHIFT+HOME
K RALL Reset Grades + Nodes | CMD+HOME
K AUTO Auto Color           | OPT+SHIFT+C
BLOCK 2 COMPARE
K GRAB Grab Still        | CMD+OPT+G
K WIPE Wipe On / Off     | CMD+W
K SPLT Split Screen      | CMD+OPT+W
K HILT Highlight         | SHIFT+H
K VER+ Add Version       | CMD+Y
K VNXT Next Version      | CMD+N
K VPRV Previous Version  | CMD+B
K VDEF Default Version   | CMD+U
K =1   Grade from 1 Back | =
K =2   Grade from 2 Back | -
K COPY Copy Grade        | CMD+C
K PSTE Paste Grade       | CMD+V
K LDA  Load Memory A     | CMD+1
K LDB  Load Memory B     | CMD+2
K SVA  Save Memory A     | OPT+1
K SVB  Save Memory B     | OPT+2
BLOCK 3 LIGHTS + PLAY
K R+   Red +          | KP7
K G+   Green +        | KP8
K B+   Blue +         | KP9
K M+   Master +       | KPPLUS
K R-   Red -          | KP4
K G-   Green -        | KP5
K B-   Blue -         | KP6
K M-   Master -       | KPENTER
K LITE Printer Lights | CMD+OPT+`
K FULL Cinema Viewer  | P
K UNDO Undo           | CMD+Z
K REDO Redo           | CMD+SHIFT+Z
K CLP- Previous Clip  | UP
K CLP+ Next Clip      | DOWN
K REV  Reverse        | J
K FWD  Forward        | L
DIAL 1 OFS ALL | KPPLUS | KPENTER
DIAL 2 OFS RED | KP7 | KP4
DIAL 3 OFS GRN | KP8 | KP5
DIAL 4 OFS BLU | KP9 | KP6
DIAL 5 CLIPS   | DOWN | UP
DIAL 7 NODES   | SHIFT+OPT+' | SHIFT+OPT+;
)DECK";

const char kGradePremiere[] = R"DECK(# i2 DECK - GRADE, Premiere side: Premiere's Color Mode (Premiere 27).
# Screen button 6 switches GRADE back to Resolve (that side is grade.txt).
#
# Color Mode has its OWN keyboard layout, separate from Edit Mode (Keyboard
# Shortcuts dialog: pick Color Mode at the top). Adobe documents only a few
# of its defaults, so every key below is marked:
#   [OK]  checked in Adobe's help pages (tools/i2deck/PREMIERE_COLOR.md)
#   [??]  a guess: check it in Premiere's Keyboard Shortcuts (Color Mode)
#         and fix the line here if it differs
#   [SET] Premiere has no default, or the default is not published: assign
#         this key to the command in Premiere (Color Mode layout), or put
#         Premiere's own key here instead
#
# How the keys work in Color Mode [OK]: with no control latched, Up / Down
# go to the previous / next clip. Click a control (or press its latch key)
# to latch it; then the arrow keys nudge it, like printer points, and
# Return or Esc accepts. Hold H to hide the HUD. The Tracker's Shift adds
# Shift to the wheel (Premiere's Shift Nudge, a bigger step).
DECK GRADE
REV 4
APP Premiere Color Mode
LINK KEYS
COLOR B48CFF

PAGE COLOR
# MODE: Window > Mode; Option/Alt + 1-4 by default [??]. The order is a
# guess (1 Import, 2 Edit, 3 Color, 4 Export): open Window > Mode and fix
# EDIT / COLR / EXPT here to match.
# CMPR: Cmd/Ctrl+3 toggles Comparison View [OK].
# PREV / NEXT: Up / Down [OK]. OK: Return accepts and unlatches [OK].
# HUD: hold the pad to hide the HUD [OK]. PLAY, UNDO, REDO [??].
# MARK: Add Marker, Cmd/Ctrl+M [??]. REF / SWAP / SPLH / SPLV: Add Color
# Reference Marker, Swap Sides, Split Horizontal, Split Vertical have no
# default keys [OK]; the keys here are [SET].
BLOCK 1 MODE + VIEW
K EDIT Edit Mode        | OPT+2
K COLR Color Mode       | OPT+3
K EXPT Export Mode      | OPT+4
K CMPR Comparison View  | CMD+3
K PREV Previous Clip    | UP
K NEXT Next Clip        | DOWN
K OK   Accept           | ENTER
K ESC  Escape           | ESC
K HUD  Hide HUD         | H
K PLAY Play             | SPACE
K UNDO Undo             | CMD+Z
K REDO Redo             | CMD+SHIFT+Z
K MARK Add Marker       | CMD+M
K REF  Reference Marker | CTRL+OPT+M
K SWAP Swap Sides       | CTRL+OPT+S
K SPLT Split Horizontal | CTRL+OPT+H
# NUDGE: the arrow keys move a latched control [OK]; Shift + arrow is the
# bigger Shift Nudge step (set in Color Control options) [??].
# J / K / L shuttle in Color Mode [??].
BLOCK 2 NUDGE + PLAY
K UP   Nudge Up        | UP
K DOWN Nudge Down      | DOWN
K LEFT Nudge Left      | LEFT
K RGHT Nudge Right     | RIGHT
K UP+  Big Nudge Up    | SHIFT+UP
K DN+  Big Nudge Down  | SHIFT+DOWN
K LT+  Big Nudge Left  | SHIFT+LEFT
K RT+  Big Nudge Right | SHIFT+RIGHT
K REV  Reverse         | J
K STOP Stop            | K
K FWD  Forward         | L
K -
K -
K -
K -
K -
# LATCH: Color Mode has keys that latch each control (Adobe: "pre-mapped
# keyboard shortcuts for latching into the different controls"), but the
# defaults are not published. [SET] all 16: in Premiere's Keyboard
# Shortcuts (Color Mode), give each control the key shown, or replace the
# key here with Premiere's. Control names are from Adobe's Adjust page.
BLOCK 3 LATCH A CONTROL
K CONT Contrast    | CTRL+OPT+1
K EXPO Exposure    | CTRL+OPT+2
K TEMP Temperature | CTRL+OPT+3
K BAL  Balance     | CTRL+OPT+4
K SAT  Saturation  | CTRL+OPT+5
K HUE  Hue         | CTRL+OPT+6
K LUM  Lum Shift   | CTRL+OPT+7
K TEXT Texture     | CTRL+OPT+8
K SHRP Sharpness   | CTRL+OPT+9
K PIVT Pivot       | CTRL+OPT+0
K BLK  Black       | CTRL+OPT+B
K TINT Tint        | CTRL+OPT+T
K SATS Sat Shift   | CTRL+OPT+U
K ZONE Zones       | CTRL+OPT+Z
K GLOB Global      | CTRL+OPT+G
K AUTO Auto Color  | CTRL+OPT+A
# The wheel: latch a control first, then NUDGE V / NUDGE H move it [OK].
# With nothing latched, NUDGE V steps clips (Up / Down) [OK].
DIAL 1 NUDGE V | UP | DOWN
DIAL 2 NUDGE H | RIGHT | LEFT
DIAL 3 CLIPS   | DOWN | UP
DIAL 4 BIG V   | SHIFT+UP | SHIFT+DOWN
DIAL 5 BIG H   | SHIFT+RIGHT | SHIFT+LEFT
)DECK";

const char kHub[] = R"DECK(# i2 DECK - HUB: i2 Creative Hub / ComfyUI over USB MIDI.
# Every pad sends a note (the pad's number, 0-47) on the page's MIDI channel;
# the wheel sends relative control changes 1-7 (65 = up, 63 = down).
# The i2 Creative Hub (1.22+) listens: turn on Settings > MIDI controller
# (i2 DECK) in Chrome or Edge. Full map in the Hub's docs/MIDI.md.
DECK HUB
APP Creative Hub
LINK MIDI
REV 3
COLOR 5BE3A5

PAGE RUN
BLOCK 1 QUEUE
K RUN  Queue
K STOP Interrupt
K CLR  Clear Queue
K AGN  Run Again
K SEED New Seed
K LOCK Lock Seed
K B1   Batch 1
K B4   Batch 4
K PRV  Previous
K NXT  Next
K KEEP Keep
K TOSS Toss
K SAVE Save
K SEND Send
K OPEN Open
K HOME Home
BLOCK 2 WORKFLOWS
K W1   Slot 1
K W2   Slot 2
K W3   Slot 3
K W4   Slot 4
K W5   Slot 5
K W6   Slot 6
K W7   Slot 7
K W8   Slot 8
K W9   Slot 9
K W10  Slot 10
K W11  Slot 11
K W12  Slot 12
K W13  Slot 13
K W14  Slot 14
K W15  Slot 15
K W16  Slot 16
BLOCK 3 SHAPE + LOOKS
K 1:1  Square 1:1
K 16:9 Wide 16:9
K 9:16 Tall 9:16
K 4:3  Classic 4:3
K DRFT Draft
K GOOD Good
K BEST Best
K UPSC Upscale
K S1   Look 1
K S2   Look 2
K S3   Look 3
K S4   Look 4
K S5   Look 5
K S6   Look 6
K S7   Look 7
K S8   Look 8
DIAL 1 STEPS
DIAL 2 CFG
DIAL 3 DENOISE
DIAL 4 SEED
DIAL 5 SIZE W
DIAL 6 SIZE H
DIAL 7 BATCH
)DECK";

const char kStatic[] = R"DECK(# i2 DECK - STATIC: the Tracker's FM radio as a sound and signal source.
# You hear the radio while STATIC is open (headphones are the antenna).
# Tuner @ACTIONS: @SEEK+ @SEEK- @STEP+ @STEP- @P1-@P8 (Shift = store)
#   @SEND @RECORDER @MON (radio on/off)
# Glitch @ACTIONS (held = on): @STUTTER @CRUSH @STOP @SCAN @FILTER
#   @R2 @R4 @R8 @R16 @R32 @R64 (repeat that note value at the project
#   tempo: @R4 = one beat)
#   latches: @FREEZE @RSSI (signal strength closes the filter) @MON
#   @KILL (latches off)
# Dials: @TUNE @AMOUNT (last effect touched) @LENGTH (stutter) @VOLUME
# Turn the wheel while holding an effect to set its strength.
DECK STATIC
REV 4
APP FM radio
LINK RADIO
COLOR FF5A3C

PAGE TUNER
BLOCK 1 TUNE
K SK-  Seek Down | @SEEK-
K SK+  Seek Up   | @SEEK+
K ST-  Step Down | @STEP-
K ST+  Step Up   | @STEP+
BLOCK 2 STATIONS
K P1   - | @P1
K P2   - | @P2
K P3   - | @P3
K P4   - | @P4
K P5   - | @P5
K P6   - | @P6
K P7   - | @P7
K P8   - | @P8
BLOCK 3 OUTPUT
K SEND Signal to CC  | @SEND
K REC  Record Sample | @RECORDER
K MON  Hear Radio    | @MON
DIAL 1 TUNE   | @TUNE
DIAL 2 VOLUME | @VOLUME

PAGE GLITCH
BLOCK 1 GLITCH
K STUT Stutter       | @STUTTER
K CRSH Crush         | @CRUSH
K STOP Tape Stop     | @STOP
K SCAN Scan          | @SCAN
K FILT Filter        | @FILTER
K FRZ  Freeze        | @FREEZE
K RSSI Signal Filter | @RSSI
K KILL All Off       | @KILL
K 1/4  Repeat 1/4    | @R4
K 1/8  Repeat 1/8    | @R8
K 1/16 Repeat 1/16   | @R16
K 1/32 Repeat 1/32   | @R32
BLOCK 2 STATIONS
K P1   -         | @P1
K P2   -         | @P2
K P3   -         | @P3
K P4   -         | @P4
K P5   -         | @P5
K P6   -         | @P6
K P7   -         | @P7
K P8   -         | @P8
K SK-  Seek Down | @SEEK-
K SK+  Seek Up   | @SEEK+
BLOCK 3 OUTPUT
K MON  Hear Radio    | @MON
K REC  Record Sample | @RECORDER
K SEND Signal to CC  | @SEND
DIAL 1 TUNE   | @TUNE
DIAL 2 AMOUNT | @AMOUNT
DIAL 3 LENGTH | @LENGTH
DIAL 4 VOLUME | @VOLUME
)DECK";
const char kMixerRes[] = R"DECK(# i2 DECK - MIXER, Resolve side: the Fairlight mixer as a Mackie Control
# surface (MCU) over the Tracker's USB MIDI port, plus a few keys.
# Screen button 6 switches MIXER, FILE and SETUP to Premiere (mixerpr.txt).
# Notes: tools/i2deck/APP_DECKS.md.  [OK] checked in Blackmagic's manual,
# [??] a guess: check it, [SET] no default: assign it in Resolve.
#
# Set up once [OK]: Preferences > System > Control Panels: tick Use MIDI
# Audio Console; MIDI Protocol MCU; MIDI Input and MIDI Output: the Tracker.
# Resolve's MCU support [OK]: faders, pan, rec arm, solo, mute, select,
# bank, transport. MARK and UNDO depend on the panel [??]; JOG [OK].
#
# Pads: @REC1-8 @SOLO1-8 @MUTE1-8 @SEL1-8 are the strip buttons. @BANK-
# and @BANK+ move 8 tracks, @CH- and @CH+ one. @REW @FF @STOP @PLAY
# @RECORD @CYCLE @MARKER @UNDO; @N<0-127> sends any Mackie Control note.
# A lit pad is a lit LED in Resolve. A strip pad also picks the strip the
# wheel works on. Wheel: @FADER @PAN @STRIP (pick the strip) @JOG @MASTER.
# Keys: FAIR / EDIT pages (Shift+7 / Shift+4) [OK]; clip level +/- 1 dB
# (Option+Cmd+= / -) [??].
DECK MIXER
REV 2
APP Resolve Fairlight
LINK MCU
COLOR 46D2E6

PAGE MIX
BLOCK 1 TRACKS 1-4
K REC1 Rec Arm 1 | @REC1
K REC2 Rec Arm 2 | @REC2
K REC3 Rec Arm 3 | @REC3
K REC4 Rec Arm 4 | @REC4
K SOL1 Solo 1    | @SOLO1
K SOL2 Solo 2    | @SOLO2
K SOL3 Solo 3    | @SOLO3
K SOL4 Solo 4    | @SOLO4
K MUT1 Mute 1    | @MUTE1
K MUT2 Mute 2    | @MUTE2
K MUT3 Mute 3    | @MUTE3
K MUT4 Mute 4    | @MUTE4
K SEL1 Select 1  | @SEL1
K SEL2 Select 2  | @SEL2
K SEL3 Select 3  | @SEL3
K SEL4 Select 4  | @SEL4
BLOCK 2 TRACKS 5-8
K REC5 Rec Arm 5 | @REC5
K REC6 Rec Arm 6 | @REC6
K REC7 Rec Arm 7 | @REC7
K REC8 Rec Arm 8 | @REC8
K SOL5 Solo 5    | @SOLO5
K SOL6 Solo 6    | @SOLO6
K SOL7 Solo 7    | @SOLO7
K SOL8 Solo 8    | @SOLO8
K MUT5 Mute 5    | @MUTE5
K MUT6 Mute 6    | @MUTE6
K MUT7 Mute 7    | @MUTE7
K MUT8 Mute 8    | @MUTE8
K SEL5 Select 5  | @SEL5
K SEL6 Select 6  | @SEL6
K SEL7 Select 7  | @SEL7
K SEL8 Select 8  | @SEL8
BLOCK 3 BANK + PLAY
K BNK- Bank -8        | @BANK-
K BNK+ Bank +8        | @BANK+
K CH-  Bank -1        | @CH-
K CH+  Bank +1        | @CH+
K REW  Rewind         | @REW
K FF   Fast Forward   | @FF
K STOP Stop           | @STOP
K PLAY Play           | @PLAY
K REC  Record         | @RECORD
K LOOP Cycle          | @CYCLE
K MARK Add Marker     | @MARKER
K UNDO Undo           | @UNDO
K FAIR Fairlight Page | SHIFT+7
K EDIT Edit Page      | SHIFT+4
K VOL+ Clip +1 dB     | CMD+OPT+=
K VOL- Clip -1 dB     | CMD+OPT+-
DIAL 1 FADER    | @FADER
DIAL 2 PAN      | @PAN
DIAL 3 TRACK    | @STRIP
DIAL 4 JOG      | @JOG
DIAL 5 CLIP VOL | CMD+OPT+= | CMD+OPT+-
DIAL 7 MASTER   | @MASTER
)DECK";

const char kMixerPr[] = R"DECK(# i2 DECK - MIXER, Premiere side: the Audio Track Mixer as a Mackie
# Control surface over the Tracker's USB MIDI port, plus a few keys.
# Screen button 6 switches MIXER, FILE and SETUP to Resolve (mixerres.txt).
# Notes: tools/i2deck/APP_DECKS.md.  [OK] checked in Adobe's pages, [??] a
# guess: check it, [SET] no default: assign it in Premiere.
#
# Set up once [OK]: Premiere Pro > Settings (Windows: Edit > Preferences)
# > Control Surface > Add > Mackie; select it, Edit, and pick the Tracker
# as MIDI Input Device and MIDI Output Device.
# Premiere's Mackie support drives the Audio Track Mixer [OK]: faders,
# pan, mute, solo, play/record [OK]; banks work in the Track Mixer [OK].
# REC arm, SELECT, MARK, UNDO, CYCLE, JOG and MASTER: [??].
#
# Pads: @REC1-8 @SOLO1-8 @MUTE1-8 @SEL1-8 are the strip buttons. @BANK-
# and @BANK+ move 8 tracks, @CH- and @CH+ one. @REW @FF @STOP @PLAY
# @RECORD @CYCLE @MARKER @UNDO; @N<0-127> sends any Mackie Control note.
# A lit pad is a lit LED in Premiere. A strip pad also picks the strip the
# wheel works on. Wheel: @FADER @PAN @STRIP (pick the strip) @JOG @MASTER.
# Keys [OK]: Audio Track Mixer Shift+6, Audio Clip Mixer Shift+9, clip
# volume +1 dB ] and -1 dB [ (Shift = 6 dB).
DECK MIXER
REV 2
APP Premiere mixer
LINK MCU
COLOR 46D2E6

PAGE MIX
BLOCK 1 TRACKS 1-4
K REC1 Rec Arm 1 | @REC1
K REC2 Rec Arm 2 | @REC2
K REC3 Rec Arm 3 | @REC3
K REC4 Rec Arm 4 | @REC4
K SOL1 Solo 1    | @SOLO1
K SOL2 Solo 2    | @SOLO2
K SOL3 Solo 3    | @SOLO3
K SOL4 Solo 4    | @SOLO4
K MUT1 Mute 1    | @MUTE1
K MUT2 Mute 2    | @MUTE2
K MUT3 Mute 3    | @MUTE3
K MUT4 Mute 4    | @MUTE4
K SEL1 Select 1  | @SEL1
K SEL2 Select 2  | @SEL2
K SEL3 Select 3  | @SEL3
K SEL4 Select 4  | @SEL4
BLOCK 2 TRACKS 5-8
K REC5 Rec Arm 5 | @REC5
K REC6 Rec Arm 6 | @REC6
K REC7 Rec Arm 7 | @REC7
K REC8 Rec Arm 8 | @REC8
K SOL5 Solo 5    | @SOLO5
K SOL6 Solo 6    | @SOLO6
K SOL7 Solo 7    | @SOLO7
K SOL8 Solo 8    | @SOLO8
K MUT5 Mute 5    | @MUTE5
K MUT6 Mute 6    | @MUTE6
K MUT7 Mute 7    | @MUTE7
K MUT8 Mute 8    | @MUTE8
K SEL5 Select 5  | @SEL5
K SEL6 Select 6  | @SEL6
K SEL7 Select 7  | @SEL7
K SEL8 Select 8  | @SEL8
BLOCK 3 BANK + PLAY
K BNK- Bank -8           | @BANK-
K BNK+ Bank +8           | @BANK+
K CH-  Bank -1           | @CH-
K CH+  Bank +1           | @CH+
K REW  Rewind            | @REW
K FF   Fast Forward      | @FF
K STOP Stop              | @STOP
K PLAY Play              | @PLAY
K REC  Record            | @RECORD
K LOOP Cycle             | @CYCLE
K MARK Add Marker        | @MARKER
K UNDO Undo              | @UNDO
K MIX  Audio Track Mixer | SHIFT+6
K CMIX Audio Clip Mixer  | SHIFT+9
K VOL+ Clip +1 dB        | ]
K VOL- Clip -1 dB        | [
DIAL 1 FADER    | @FADER
DIAL 2 PAN      | @PAN
DIAL 3 TRACK    | @STRIP
DIAL 4 JOG      | @JOG
DIAL 5 CLIP VOL | ] | [
DIAL 7 MASTER   | @MASTER
)DECK";

const char kFileRes[] = R"DECK(# i2 DECK - FILE, Resolve side: DaVinci Resolve's File menu and Deliver.
# Screen button 6 switches MIXER, FILE and SETUP to Premiere (filepr.txt).
# Notes: tools/i2deck/APP_DECKS.md.
#   [OK] PMGR (Shift+1), SAVE, IMTL (Shift+Cmd+I), MEDP DLVR EDIT CUTP
#        (Shift+2 / 8 / 4 / 3), UNDO REDO
#   [??] SVAS NTL NBIN IMP EXPJ EXTL QUIT: from third-party lists; check
#        them in DaVinci Resolve > Keyboard Customization
#   [SET] no default: NPRJ IMPJ MGMT RLNK QEXP ADDQ RNDR send
#         Control+Option+Shift (Windows: Ctrl+Alt+Shift) + a letter. Give
#         each command that key in Keyboard Customization (File > New
#         Project, Import Project, Media Management, Relink Media, Quick
#         Export; Deliver: Add to Render Queue, Render All).
DECK FILE
REV 2
APP DaVinci Resolve
LINK KEYS
COLOR F2D25C

PAGE FILE
BLOCK 1 PROJECT
K PMGR Project Manager | SHIFT+1
K NPRJ New Project     | CTRL+OPT+SHIFT+N
K SAVE Save            | CMD+S
K SVAS Save As         | CMD+SHIFT+S
K IMPJ Import Project  | CTRL+OPT+SHIFT+J
K EXPJ Export Project  | CMD+E
K QUIT Quit            | CMD+Q
K -
K -
K -
K -
K -
K UNDO Undo            | CMD+Z
K REDO Redo            | CMD+SHIFT+Z
K ENT  Enter           | ENTER
K ESC  Escape          | ESC
BLOCK 2 NEW + IMPORT
K NTL  New Timeline     | CMD+N
K NBIN New Bin          | CMD+SHIFT+N
K IMP  Import Media     | CMD+I
K IMTL Import Timeline  | CMD+SHIFT+I
K MEDP Media Page       | SHIFT+2
K MGMT Media Management | CTRL+OPT+SHIFT+M
K RLNK Relink Media     | CTRL+OPT+SHIFT+L
K -
K -
K -
K -
K -
K TAB  Next Field       | TAB
K STAB Previous Field   | SHIFT+TAB
K UP   Up               | UP
K DOWN Down             | DOWN
BLOCK 3 EXPORT + DELIVER
K DLVR Deliver Page        | SHIFT+8
K EXTL Export Timeline     | CMD+SHIFT+O
K QEXP Quick Export        | CTRL+OPT+SHIFT+Q
K ADDQ Add to Render Queue | CTRL+OPT+SHIFT+A
K RNDR Render All          | CTRL+OPT+SHIFT+R
K -
K EDIT Edit Page           | SHIFT+4
K CUTP Cut Page            | SHIFT+3
DIAL 1 LIST     | DOWN | UP
DIAL 2 FIELDS   | TAB | SHIFT+TAB
DIAL 3 LEFT/RT  | RIGHT | LEFT
)DECK";

const char kFilePr[] = R"DECK(# i2 DECK - FILE, Premiere side: Premiere Pro's File menu.
# Screen button 6 switches MIXER, FILE and SETUP to Resolve (fileres.txt).
# Notes: tools/i2deck/APP_DECKS.md. Every key is Premiere's default
# [OK, Adobe's default shortcuts page] except:
#   [??] FRAM (Export Frame), EXPM (Export mode, Option/Alt+4)
#   [SET] no default: REVT MGMT LINK OFFL EDL XML AAF RAUD DREN send
#         Control+Option+Shift (Windows: Ctrl+Alt+Shift) + a letter. In
#         Premiere's Keyboard Shortcuts, give each command the key shown
#         here (File > Revert, File > Project Manager, Clip > Link Media,
#         Clip > Make Offline, File > Export > EDL / Final Cut Pro XML /
#         AAF, Sequence > Render Audio, Sequence > Delete Render Files).
DECK FILE
REV 2
APP Premiere Pro
LINK KEYS
COLOR F2D25C

PAGE FILE
BLOCK 1 PROJECT
K NPRJ New Project     | CMD+OPT+N
K OPEN Open Project    | CMD+O
K SAVE Save            | CMD+S
K SVAS Save As         | CMD+SHIFT+S
K COPY Save a Copy     | CMD+OPT+S
K CLOS Close           | CMD+W
K CLPJ Close Project   | CMD+SHIFT+W
K QUIT Quit            | CMD+Q
K REVT Revert          | CTRL+OPT+SHIFT+R
K MGMT Project Manager | CTRL+OPT+SHIFT+P
K PROP Properties      | CMD+SHIFT+H
K -
K UNDO Undo            | CMD+Z
K REDO Redo            | CMD+SHIFT+Z
K ENT  Enter           | ENTER
K ESC  Escape          | ESC
BLOCK 2 NEW + IMPORT
K NSEQ New Sequence        | CMD+N
K NBIN New Bin             | CMD+B
K IMP  Import              | CMD+I
K IMB  Import from Browser | CMD+OPT+I
K PROJ Project Panel       | SHIFT+1
K MBRW Media Browser       | SHIFT+8
K SRC  Open in Source      | SHIFT+O
K -
K LINK Link Media          | CTRL+OPT+SHIFT+L
K OFFL Make Offline        | CTRL+OPT+SHIFT+O
K -
K -
K TAB  Next Field          | TAB
K STAB Previous Field      | SHIFT+TAB
K UP   Up                  | UP
K DOWN Down                | DOWN
BLOCK 3 EXPORT + RENDER
K EXP  Export Media        | CMD+M
K AME  Send to Encoder     | OPT+SHIFT+M
K FRAM Export Frame        | CMD+SHIFT+E
K EXPM Export Mode         | OPT+4
K EDL  Export EDL          | CTRL+OPT+SHIFT+E
K XML  Export XML          | CTRL+OPT+SHIFT+X
K AAF  Export AAF          | CTRL+OPT+SHIFT+F
K -
K REND Render In to Out    | ENTER
K RAUD Render Audio        | CTRL+OPT+SHIFT+A
K DREN Delete Render Files | CTRL+OPT+SHIFT+D
DIAL 1 LIST     | DOWN | UP
DIAL 2 FIELDS   | TAB | SHIFT+TAB
DIAL 3 LEFT/RT  | RIGHT | LEFT
)DECK";

const char kSetupRes[] = R"DECK(# i2 DECK - SETUP, Resolve side: DaVinci Resolve's settings and caches.
# Screen button 6 switches MIXER, FILE and SETUP to Premiere (setuppr.txt).
# Notes: tools/i2deck/APP_DECKS.md.
#   [OK] KEYS (Keyboard Customization, Cmd+Option+K), PSET (Project
#        Settings, Shift+9), PMGR (Shift+1), the page keys (Shift+2-8),
#        the dialog keys
#   [??] PREF (Preferences, Cmd+comma), CNMA (Cinema viewer, P)
#   [SET] no default: TLST RCOF RCSM RCUS RCCO DRC PRXY OPTM send
#         Control+Option+Shift (Windows: Ctrl+Alt+Shift) + a letter. Give
#         each command that key in Keyboard Customization (Timeline
#         Settings; Playback > Render Cache > None / Smart / User; Clip >
#         Render Cache Color Output; Playback > Delete Render Cache > All;
#         Playback > Use Proxy Media if Available; Use Optimized Media if
#         Available).
DECK SETUP
REV 2
APP DaVinci Resolve
LINK KEYS
COLOR F28FB8

PAGE SETUP
BLOCK 1 SETTINGS
K PREF Preferences         | CMD+,
K KEYS Keyboard Shortcuts  | CMD+OPT+K
K PSET Project Settings    | SHIFT+9
K PMGR Project Manager     | SHIFT+1
K TLST Timeline Settings   | CTRL+OPT+SHIFT+T
K -
K -
K -
K RCOF Render Cache None   | CTRL+OPT+SHIFT+O
K RCSM Render Cache Smart  | CTRL+OPT+SHIFT+S
K RCUS Render Cache User   | CTRL+OPT+SHIFT+U
K RCCO Cache Clip Output   | CTRL+OPT+SHIFT+C
K DRC  Delete Render Cache | CTRL+OPT+SHIFT+D
K PRXY Use Proxy Media     | CTRL+OPT+SHIFT+P
K OPTM Use Optimized Media | CTRL+OPT+SHIFT+Z
BLOCK 2 PAGES
K MEDP Media Page     | SHIFT+2
K CUTP Cut Page       | SHIFT+3
K EDIT Edit Page      | SHIFT+4
K FUSN Fusion Page    | SHIFT+5
K COLR Color Page     | SHIFT+6
K FAIR Fairlight Page | SHIFT+7
K DLVR Deliver Page   | SHIFT+8
K CNMA Cinema Viewer  | P
BLOCK 3 DIALOG
K ENT  Enter          | ENTER
K ESC  Escape         | ESC
K TAB  Next Field     | TAB
K STAB Previous Field | SHIFT+TAB
K UP   Up             | UP
K DOWN Down           | DOWN
K LEFT Left           | LEFT
K RGHT Right          | RIGHT
K SPC  Space          | SPACE
K -
K UNDO Undo           | CMD+Z
K SAVE Save           | CMD+S
DIAL 1 LIST     | DOWN | UP
DIAL 2 FIELDS   | TAB | SHIFT+TAB
DIAL 3 LEFT/RT  | RIGHT | LEFT
)DECK";

const char kSetupPr[] = R"DECK(# i2 DECK - SETUP, Premiere side: Premiere Pro's settings dialogs.
# Screen button 6 switches MIXER, FILE and SETUP to Resolve (setupres.txt).
# Notes: tools/i2deck/APP_DECKS.md.
#   [OK] KEYS (Keyboard Shortcuts, Cmd+Option+K), REND, MAX, FULL and the
#        dialog keys
#   [??] EDIT COLR EXPT (Window > Mode, Option/Alt+2/3/4), WSRS (Reset to
#        Saved Layout, Option/Alt+Shift+0)
#   [SET] Premiere has no default for the settings pages: PREF AUD AHW
#         MEDI MCAC PLAY TIME AUTO SURF PSET SCR SSET PMGR RAUD DREN send
#         Control+Option+Shift (Windows: Ctrl+Alt+Shift) + a letter. Give
#         each command that key in Keyboard Shortcuts (Premiere Pro >
#         Settings > General / Audio / Audio Hardware / Media / Media
#         Cache / Playback / Timeline / Auto Save / Control Surface; File >
#         Project Settings > General / Scratch Disks; Sequence > Sequence
#         Settings; File > Project Manager; Sequence > Render Audio /
#         Delete Render Files). On Windows these are under Edit >
#         Preferences.
DECK SETUP
REV 2
APP Premiere Pro
LINK KEYS
COLOR F28FB8

PAGE SETUP
BLOCK 1 SETTINGS
K PREF General            | CTRL+OPT+SHIFT+G
K KEYS Keyboard Shortcuts | CMD+OPT+K
K AUD  Audio              | CTRL+OPT+SHIFT+U
K AHW  Audio Hardware     | CTRL+OPT+SHIFT+H
K MEDI Media              | CTRL+OPT+SHIFT+I
K MCAC Media Cache        | CTRL+OPT+SHIFT+C
K PLAY Playback           | CTRL+OPT+SHIFT+B
K TIME Timeline           | CTRL+OPT+SHIFT+T
K AUTO Auto Save          | CTRL+OPT+SHIFT+V
K SURF Control Surface    | CTRL+OPT+SHIFT+S
BLOCK 2 PROJECT + VIEW
K PSET Project Settings    | CTRL+OPT+SHIFT+J
K SCR  Scratch Disks       | CTRL+OPT+SHIFT+W
K SSET Sequence Settings   | CTRL+OPT+SHIFT+Q
K PMGR Project Manager     | CTRL+OPT+SHIFT+P
K REND Render In to Out    | ENTER
K RAUD Render Audio        | CTRL+OPT+SHIFT+A
K DREN Delete Render Files | CTRL+OPT+SHIFT+D
K -
K EDIT Edit Mode           | OPT+2
K COLR Color Mode          | OPT+3
K EXPT Export Mode         | OPT+4
K WSRS Reset Workspace     | OPT+SHIFT+0
K MAX  Maximize Panel      | SHIFT+`
K FULL Full Screen         | CTRL+`
BLOCK 3 DIALOG
K ENT  Enter          | ENTER
K ESC  Escape         | ESC
K TAB  Next Field     | TAB
K STAB Previous Field | SHIFT+TAB
K UP   Up             | UP
K DOWN Down           | DOWN
K LEFT Left           | LEFT
K RGHT Right          | RIGHT
K SPC  Space          | SPACE
K -
K UNDO Undo           | CMD+Z
K SAVE Save           | CMD+S
DIAL 1 LIST     | DOWN | UP
DIAL 2 FIELDS   | TAB | SHIFT+TAB
DIAL 3 LEFT/RT  | RIGHT | LEFT
)DECK";

} // namespace

const BuiltinDeck kGradePremiereDeck = { "gradepr.txt", "GRADE", "PREMIERE", "Resolve or", "Premiere color.", "USB KEYS", 0xB48CFF, kGradePremiere, nullptr, 0 };

// MIXER, FILE and SETUP: Resolve first (the top of the switch), Premiere
// second, like GRADE. They share app= in settings.txt (Premiere unless set).
const BuiltinDeck kMixerPrDeck = { "mixerpr.txt", "MIXER", "PREMIERE", "Faders, mute,", "solo, banks.", "MACKIE MCU", 0x46D2E6, kMixerPr, nullptr, 1 };
const BuiltinDeck kFilePrDeck = { "filepr.txt", "FILE", "PREMIERE", "New, open,", "save, export.", "USB KEYS", 0xF2D25C, kFilePr, nullptr, 1 };
const BuiltinDeck kSetupPrDeck = { "setuppr.txt", "SETUP", "PREMIERE", "Settings,", "caches, keys.", "USB KEYS", 0xF28FB8, kSetupPr, nullptr, 1 };

const BuiltinDeck kBuiltin[kBuiltinDecks] = {
    { "cut.txt", "CUT", "PREMIERE", "Edit, trim,", "move, mark.", "USB KEYS", 0x7FB2FF, kCut, nullptr, 0 },
    { "grade.txt", "GRADE", "RESOLVE", "Resolve or", "Premiere color.", "USB KEYS", 0xFFB454, kGrade, &kGradePremiereDeck, 0 },
    { "hub.txt", "HUB", "CREATIVE HUB", "Queue, slots,", "seeds, sizes.", "USB MIDI", 0x5BE3A5, kHub, nullptr, 0 },
    { "static.txt", "STATIC", "FM RADIO", "Tune, hear,", "glitch it.", "ANTENNA", 0xFF5A3C, kStatic, nullptr, 0 },
    { "mixerres.txt", "MIXER", "RESOLVE", "Faders, mute,", "solo, banks.", "MACKIE MCU", 0x46D2E6, kMixerRes, &kMixerPrDeck, 1 },
    { "fileres.txt", "FILE", "RESOLVE", "New, open,", "save, export.", "USB KEYS", 0xF2D25C, kFileRes, &kFilePrDeck, 1 },
    { "setupres.txt", "SETUP", "RESOLVE", "Settings,", "caches, keys.", "USB KEYS", 0xF28FB8, kSetupRes, &kSetupPrDeck, 1 },
};

const BuiltinDeck* const kAll[kAllDecks] = { &kBuiltin[0], &kBuiltin[1], &kGradePremiereDeck, &kBuiltin[2], &kBuiltin[3], &kBuiltin[4],
                                             &kMixerPrDeck, &kBuiltin[5], &kFilePrDeck, &kBuiltin[6], &kSetupPrDeck };

} // namespace i2deck
