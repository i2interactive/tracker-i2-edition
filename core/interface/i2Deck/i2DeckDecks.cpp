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
//                          row first. "K -" leaves a pad empty.
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
APP Premiere Pro
LINK KEYS
COLOR 7FB2FF

PAGE EDIT
BLOCK 1 PLAY + MARK
K REV  REVERSE  | J
K STOP STOP     | K
K FWD  FORWARD  | L
K LOOP AROUND   | SHIFT+K
K IN   MARK IN  | I
K OUT  MARK OUT | O
K CLIP MARK CLIP| X
K CLR  CLEAR IO | OPT+X | CTRL+SHIFT+X
K GOIN TO IN    | SHIFT+I
K GOUT TO OUT   | SHIFT+O
K MTCH MATCH FR | F
K MARK MARKER   | M
K UNDO UNDO     | CMD+Z
K REDO REDO     | CMD+SHIFT+Z
K SNAP SNAP     | S
K SAVE SAVE     | CMD+S
BLOCK 2 CUT + TRIM
K CUT  ADD EDIT | CMD+K
K CUTA ALL TRKS | CMD+SHIFT+K
K RDEL RIPPLE   | OPT+BKSP | SHIFT+DEL
K LIFT LIFT     | ;
K EXTR EXTRACT  | '
K INS  INSERT   | ,
K OVR  OVERWRT  | .
K EXT  EXTEND   | E
K Q    TRIM PRV | Q
K W    TRIM NXT | W
K NUD- NUDGE L  | OPT+LEFT
K NUD+ NUDGE R  | OPT+RIGHT
K SEL  SELECT   | V
K RAZR RAZOR    | C
K RPL  RIPPLE T | B
K SLIP SLIP     | Y
BLOCK 3 MOVE + VIEW
K TOP  START    | HOME
K END  END      | END
K PRV  PREV CUT | UP
K NXT  NEXT CUT | DOWN
K -5   BACK 5   | SHIFT+LEFT
K +5   FWD 5    | SHIFT+RIGHT
K MK-  PREV MRK | CMD+SHIFT+M
K MK+  NEXT MRK | SHIFT+M
K FIT  ZOOM FIT | BACKSLASH
K MAX  MAXIMIZE | SHIFT+`
K FULL FULLSCRN | CTRL+`
K REND RENDER   | ENTER
K Z+   ZOOM IN  | =
K Z-   ZOOM OUT | -
K TRK+ TALLER   | CMD+=
K TRK- SHORTER  | CMD+-
DIAL 1 FRAME   | RIGHT | LEFT
DIAL 2 JUMP 5  | SHIFT+RIGHT | SHIFT+LEFT
DIAL 3 CUTS    | DOWN | UP
DIAL 4 ZOOM    | = | -
DIAL 5 MARKERS | SHIFT+M | CMD+SHIFT+M
DIAL 6 VOLUME  | ] | [

PAGE CLIPS
BLOCK 1 CLIPS
K COPY COPY     | CMD+C
K PSTE PASTE    | CMD+V
K PINS PASTE IN | CMD+SHIFT+V
K ATTR ATTRIBS  | CMD+OPT+V
K CUT  CUT      | CMD+X
K ALL  SEL ALL  | CMD+A
K NONE DESELECT | CMD+SHIFT+A
K DEL  CLEAR    | BKSP
K GRP  GROUP    | CMD+G
K UGRP UNGROUP  | CMD+SHIFT+G
K LINK LINK     | CMD+L
K ENBL ENABLE   | SHIFT+E
K SPD  SPEED    | CMD+R
K GAIN GAIN     | G
K SUB  SUBCLIP  | CMD+U
K FIND FIND     | CMD+F
BLOCK 2 FX + FILE
K VTRN VIDEO TR | CMD+D
K ATRN AUDIO TR | CMD+SHIFT+D
K DTRN BOTH TR  | SHIFT+D
K -
K +1DB LOUDER   | ]
K -1DB QUIETER  | [
K +6DB LOUD 6   | SHIFT+]
K -6DB QUIET 6  | SHIFT+[
K IMP  IMPORT   | CMD+I
K EXP  EXPORT   | CMD+M
K SAVE SAVE     | CMD+S
K BIN  NEW BIN  | CMD+B
K ESC  ESCAPE   | ESC
K ENT  ENTER    | ENTER
K TAB  TAB      | TAB
K -
BLOCK 3 PANELS
K PROJ PROJECT  | SHIFT+1
K SRC  SOURCE   | SHIFT+2
K TIME TIMELINE | SHIFT+3
K PGM  PROGRAM  | SHIFT+4
K FXC  FX CTRLS | SHIFT+5
K MIX  TRK MIX  | SHIFT+6
K FX   EFFECTS  | SHIFT+7
K MED  MEDIA    | SHIFT+8
DIAL 1 FRAME   | RIGHT | LEFT
DIAL 2 JUMP 5  | SHIFT+RIGHT | SHIFT+LEFT
DIAL 3 CUTS    | DOWN | UP
DIAL 4 ZOOM    | = | -
DIAL 5 MARKERS | SHIFT+M | CMD+SHIFT+M
DIAL 6 VOLUME  | ] | [
)DECK";

const char kGrade[] = R"DECK(# i2 DECK - GRADE: DaVinci Resolve colour page, default keyboard shortcuts.
# The wheel trims colour through Resolve's printer lights: press LITE once
# to switch them on, then pick MASTER / RED / GREEN / BLUE and turn.
# Screen button 6 switches GRADE to Premiere (that side is gradepr.txt).
DECK GRADE
REV 3
APP DaVinci Resolve
LINK KEYS
COLOR FFB454

PAGE COLOR
BLOCK 1 NODES
K SER  SERIAL   | OPT+S
K PAR  PARALLEL | OPT+P
K LAYR LAYER    | OPT+L
K OUTS OUTSIDE  | OPT+O
K CIRC + CIRCLE | OPT+C
K LIN  + LINEAR | OPT+Q
K POLY + POLY   | OPT+G
K CURV + CURVE  | OPT+B
K ND-  PREV ND  | SHIFT+OPT+;
K ND+  NEXT ND  | SHIFT+OPT+'
K NODE NODE OFF | CMD+D
K BYP  BYPASS   | SHIFT+D
K RST  RESET ND | SHIFT+HOME
K RSTG RESET GR | CMD+SHIFT+HOME
K RALL RESET ALL| CMD+HOME
K AUTO AUTO     | OPT+SHIFT+C
BLOCK 2 COMPARE
K GRAB GRAB STL | CMD+OPT+G
K WIPE WIPE     | CMD+W
K SPLT SPLIT    | CMD+OPT+W
K HILT HILITE   | SHIFT+H
K VER+ NEW VER  | CMD+Y
K VNXT NEXT VER | CMD+N
K VPRV PREV VER | CMD+B
K VDEF DEFAULT  | CMD+U
K =1   GRADE -1 | =
K =2   GRADE -2 | -
K COPY COPY     | CMD+C
K PSTE PASTE    | CMD+V
K LDA  LOAD A   | CMD+1
K LDB  LOAD B   | CMD+2
K SVA  SAVE A   | OPT+1
K SVB  SAVE B   | OPT+2
BLOCK 3 LIGHTS + PLAY
K R+   RED +    | KP7
K G+   GREEN +  | KP8
K B+   BLUE +   | KP9
K M+   MASTER + | KPPLUS
K R-   RED -    | KP4
K G-   GREEN -  | KP5
K B-   BLUE -   | KP6
K M-   MASTER - | KPENTER
K LITE LIGHTS   | CMD+OPT+`
K FULL CINEMA   | P
K UNDO UNDO     | CMD+Z
K REDO REDO     | CMD+SHIFT+Z
K CLP- PREV CLP | UP
K CLP+ NEXT CLP | DOWN
K REV  REVERSE  | J
K FWD  FORWARD  | L
DIAL 1 MASTER  | KPPLUS | KPENTER
DIAL 2 RED     | KP7 | KP4
DIAL 3 GREEN   | KP8 | KP5
DIAL 4 BLUE    | KP9 | KP6
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
REV 3
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
K EDIT EDIT MODE | OPT+2
K COLR COLOR MODE| OPT+3
K EXPT EXPORT MOD| OPT+4
K CMPR COMPARE   | CMD+3
K PREV PREV CLIP | UP
K NEXT NEXT CLIP | DOWN
K OK   ACCEPT    | ENTER
K ESC  ESCAPE    | ESC
K HUD  HIDE HUD  | H
K PLAY PLAY      | SPACE
K UNDO UNDO      | CMD+Z
K REDO REDO      | CMD+SHIFT+Z
K MARK MARKER    | CMD+M
K REF  REF MARKER| CTRL+OPT+M
K SWAP SWAP SIDES| CTRL+OPT+S
K SPLT SPLIT     | CTRL+OPT+H
# NUDGE: the arrow keys move a latched control [OK]; Shift + arrow is the
# bigger Shift Nudge step (set in Color Control options) [??].
# J / K / L shuttle in Color Mode [??].
BLOCK 2 NUDGE + PLAY
K UP   NUDGE UP  | UP
K DOWN NUDGE DN  | DOWN
K LEFT NUDGE LT  | LEFT
K RGHT NUDGE RT  | RIGHT
K UP+  BIG UP    | SHIFT+UP
K DN+  BIG DOWN  | SHIFT+DOWN
K LT+  BIG LEFT  | SHIFT+LEFT
K RT+  BIG RIGHT | SHIFT+RIGHT
K REV  REVERSE   | J
K STOP STOP      | K
K FWD  FORWARD   | L
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
K CONT CONTRAST  | CTRL+OPT+1
K EXPO EXPOSURE  | CTRL+OPT+2
K TEMP TEMPERATUR| CTRL+OPT+3
K BAL  BALANCE   | CTRL+OPT+4
K SAT  SATURATION| CTRL+OPT+5
K HUE  HUE       | CTRL+OPT+6
K LUM  LUM SHIFT | CTRL+OPT+7
K TEXT TEXTURE   | CTRL+OPT+8
K SHRP SHARPNESS | CTRL+OPT+9
K PIVT PIVOT     | CTRL+OPT+0
K BLK  BLACK     | CTRL+OPT+B
K TINT TINT      | CTRL+OPT+T
K SATS SAT SHIFT | CTRL+OPT+U
K ZONE ZONES     | CTRL+OPT+Z
K GLOB GLOBAL    | CTRL+OPT+G
K AUTO AUTO COLOR| CTRL+OPT+A
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
REV 2
COLOR 5BE3A5

PAGE RUN
BLOCK 1 QUEUE
K RUN  QUEUE
K STOP INTERRUPT
K CLR  CLEAR Q
K AGN  AGAIN
K SEED NEW SEED
K LOCK LOCK SEED
K B1   BATCH 1
K B4   BATCH 4
K PRV  PREVIOUS
K NXT  NEXT
K KEEP KEEP
K TOSS TOSS
K SAVE SAVE
K SEND SEND
K OPEN OPEN
K HOME HOME
BLOCK 2 WORKFLOWS
K W1   SLOT 1
K W2   SLOT 2
K W3   SLOT 3
K W4   SLOT 4
K W5   SLOT 5
K W6   SLOT 6
K W7   SLOT 7
K W8   SLOT 8
K W9   SLOT 9
K W10  SLOT 10
K W11  SLOT 11
K W12  SLOT 12
K W13  SLOT 13
K W14  SLOT 14
K W15  SLOT 15
K W16  SLOT 16
BLOCK 3 SHAPE + LOOKS
K 1:1  SQUARE
K 16:9 WIDE
K 9:16 TALL
K 4:3  CLASSIC
K DRFT DRAFT
K GOOD GOOD
K BEST BEST
K UPSC UPSCALE IT
K S1   LOOK 1
K S2   LOOK 2
K S3   LOOK 3
K S4   LOOK 4
K S5   LOOK 5
K S6   LOOK 6
K S7   LOOK 7
K S8   LOOK 8
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
REV 3
APP FM radio
LINK RADIO
COLOR FF5A3C

PAGE TUNER
BLOCK 1 TUNE
K SK-  SEEK DOWN | @SEEK-
K SK+  SEEK UP   | @SEEK+
K ST-  STEP DOWN | @STEP-
K ST+  STEP UP   | @STEP+
BLOCK 2 STATIONS
K P1 - | @P1
K P2 - | @P2
K P3 - | @P3
K P4 - | @P4
K P5 - | @P5
K P6 - | @P6
K P7 - | @P7
K P8 - | @P8
BLOCK 3 OUTPUT
K SEND SIG TO CC | @SEND
K REC  SAMPLE IT | @RECORDER
K MON  HEAR IT   | @MON
DIAL 1 TUNE   | @TUNE
DIAL 2 VOLUME | @VOLUME

PAGE GLITCH
BLOCK 1 GLITCH
K STUT STUTTER   | @STUTTER
K CRSH CRUSH     | @CRUSH
K STOP TAPE STOP | @STOP
K SCAN SCAN      | @SCAN
K FILT FILTER    | @FILTER
K FRZ  FREEZE    | @FREEZE
K RSSI SIG FILTER| @RSSI
K KILL ALL OFF   | @KILL
K 1/4  REPEAT 1/4| @R4
K 1/8  REPEAT 1/8| @R8
K 1/16 REPEAT16TH| @R16
K 1/32 REPEAT32ND| @R32
BLOCK 2 STATIONS
K P1 - | @P1
K P2 - | @P2
K P3 - | @P3
K P4 - | @P4
K P5 - | @P5
K P6 - | @P6
K P7 - | @P7
K P8 - | @P8
K SK-  SEEK DOWN | @SEEK-
K SK+  SEEK UP   | @SEEK+
BLOCK 3 OUTPUT
K MON  HEAR IT   | @MON
K REC  SAMPLE IT | @RECORDER
K SEND SIG TO CC | @SEND
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
REV 1
APP Resolve Fairlight
LINK MCU
COLOR 46D2E6

PAGE MIX
BLOCK 1 TRACKS 1-4
K REC1 REC ARM 1 | @REC1
K REC2 REC ARM 2 | @REC2
K REC3 REC ARM 3 | @REC3
K REC4 REC ARM 4 | @REC4
K SOL1 SOLO 1 | @SOLO1
K SOL2 SOLO 2 | @SOLO2
K SOL3 SOLO 3 | @SOLO3
K SOL4 SOLO 4 | @SOLO4
K MUT1 MUTE 1 | @MUTE1
K MUT2 MUTE 2 | @MUTE2
K MUT3 MUTE 3 | @MUTE3
K MUT4 MUTE 4 | @MUTE4
K SEL1 SELECT 1 | @SEL1
K SEL2 SELECT 2 | @SEL2
K SEL3 SELECT 3 | @SEL3
K SEL4 SELECT 4 | @SEL4
BLOCK 2 TRACKS 5-8
K REC5 REC ARM 5 | @REC5
K REC6 REC ARM 6 | @REC6
K REC7 REC ARM 7 | @REC7
K REC8 REC ARM 8 | @REC8
K SOL5 SOLO 5 | @SOLO5
K SOL6 SOLO 6 | @SOLO6
K SOL7 SOLO 7 | @SOLO7
K SOL8 SOLO 8 | @SOLO8
K MUT5 MUTE 5 | @MUTE5
K MUT6 MUTE 6 | @MUTE6
K MUT7 MUTE 7 | @MUTE7
K MUT8 MUTE 8 | @MUTE8
K SEL5 SELECT 5 | @SEL5
K SEL6 SELECT 6 | @SEL6
K SEL7 SELECT 7 | @SEL7
K SEL8 SELECT 8 | @SEL8
BLOCK 3 BANK + PLAY
K BNK- BANK -8   | @BANK-
K BNK+ BANK +8   | @BANK+
K CH-  BANK -1   | @CH-
K CH+  BANK +1   | @CH+
K REW  REWIND    | @REW
K FF   FAST FWD  | @FF
K STOP STOP      | @STOP
K PLAY PLAY      | @PLAY
K REC  RECORD    | @RECORD
K LOOP CYCLE     | @CYCLE
K MARK MARKER    | @MARKER
K UNDO UNDO      | @UNDO
K FAIR FAIRLIGHT | SHIFT+7
K EDIT EDIT PAGE | SHIFT+4
K VOL+ CLIP +1DB | CMD+OPT+=
K VOL- CLIP -1DB | CMD+OPT+-
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
REV 1
APP Premiere mixer
LINK MCU
COLOR 46D2E6

PAGE MIX
BLOCK 1 TRACKS 1-4
K REC1 REC ARM 1 | @REC1
K REC2 REC ARM 2 | @REC2
K REC3 REC ARM 3 | @REC3
K REC4 REC ARM 4 | @REC4
K SOL1 SOLO 1 | @SOLO1
K SOL2 SOLO 2 | @SOLO2
K SOL3 SOLO 3 | @SOLO3
K SOL4 SOLO 4 | @SOLO4
K MUT1 MUTE 1 | @MUTE1
K MUT2 MUTE 2 | @MUTE2
K MUT3 MUTE 3 | @MUTE3
K MUT4 MUTE 4 | @MUTE4
K SEL1 SELECT 1 | @SEL1
K SEL2 SELECT 2 | @SEL2
K SEL3 SELECT 3 | @SEL3
K SEL4 SELECT 4 | @SEL4
BLOCK 2 TRACKS 5-8
K REC5 REC ARM 5 | @REC5
K REC6 REC ARM 6 | @REC6
K REC7 REC ARM 7 | @REC7
K REC8 REC ARM 8 | @REC8
K SOL5 SOLO 5 | @SOLO5
K SOL6 SOLO 6 | @SOLO6
K SOL7 SOLO 7 | @SOLO7
K SOL8 SOLO 8 | @SOLO8
K MUT5 MUTE 5 | @MUTE5
K MUT6 MUTE 6 | @MUTE6
K MUT7 MUTE 7 | @MUTE7
K MUT8 MUTE 8 | @MUTE8
K SEL5 SELECT 5 | @SEL5
K SEL6 SELECT 6 | @SEL6
K SEL7 SELECT 7 | @SEL7
K SEL8 SELECT 8 | @SEL8
BLOCK 3 BANK + PLAY
K BNK- BANK -8   | @BANK-
K BNK+ BANK +8   | @BANK+
K CH-  BANK -1   | @CH-
K CH+  BANK +1   | @CH+
K REW  REWIND    | @REW
K FF   FAST FWD  | @FF
K STOP STOP      | @STOP
K PLAY PLAY      | @PLAY
K REC  RECORD    | @RECORD
K LOOP CYCLE     | @CYCLE
K MARK MARKER    | @MARKER
K UNDO UNDO      | @UNDO
K MIX  TRACK MIX | SHIFT+6
K CMIX CLIP MIX  | SHIFT+9
K VOL+ CLIP +1DB | ]
K VOL- CLIP -1DB | [
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
REV 1
APP DaVinci Resolve
LINK KEYS
COLOR F2D25C

PAGE FILE
BLOCK 1 PROJECT
K PMGR PROJ MGR  | SHIFT+1
K NPRJ NEW PROJ  | CTRL+OPT+SHIFT+N
K SAVE SAVE      | CMD+S
K SVAS SAVE AS   | CMD+SHIFT+S
K IMPJ IMPORT PRJ| CTRL+OPT+SHIFT+J
K EXPJ EXPORT PRJ| CMD+E
K QUIT QUIT      | CMD+Q
K -
K -
K -
K -
K -
K UNDO UNDO      | CMD+Z
K REDO REDO      | CMD+SHIFT+Z
K ENT  ENTER     | ENTER
K ESC  ESCAPE    | ESC
BLOCK 2 NEW + IMPORT
K NTL  NEW TIMELN| CMD+N
K NBIN NEW BIN   | CMD+SHIFT+N
K IMP  IMPORT    | CMD+I
K IMTL IMPORT TL | CMD+SHIFT+I
K MEDP MEDIA PAGE| SHIFT+2
K MGMT MEDIA MGMT| CTRL+OPT+SHIFT+M
K RLNK RELINK    | CTRL+OPT+SHIFT+L
K -
K -
K -
K -
K -
K TAB  NEXT FIELD| TAB
K STAB PREV FIELD| SHIFT+TAB
K UP   UP        | UP
K DOWN DOWN      | DOWN
BLOCK 3 EXPORT + DELIVER
K DLVR DELIVER PG| SHIFT+8
K EXTL EXPORT TL | CMD+SHIFT+O
K QEXP QUICK EXP | CTRL+OPT+SHIFT+Q
K ADDQ ADD TO Q  | CTRL+OPT+SHIFT+A
K RNDR RENDER ALL| CTRL+OPT+SHIFT+R
K -
K EDIT EDIT PAGE | SHIFT+4
K CUTP CUT PAGE  | SHIFT+3
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
REV 1
APP Premiere Pro
LINK KEYS
COLOR F2D25C

PAGE FILE
BLOCK 1 PROJECT
K NPRJ NEW PROJ  | CMD+OPT+N
K OPEN OPEN PROJ | CMD+O
K SAVE SAVE      | CMD+S
K SVAS SAVE AS   | CMD+SHIFT+S
K COPY SAVE COPY | CMD+OPT+S
K CLOS CLOSE     | CMD+W
K CLPJ CLOSE PRJ | CMD+SHIFT+W
K QUIT QUIT      | CMD+Q
K REVT REVERT    | CTRL+OPT+SHIFT+R
K MGMT PROJ MGR  | CTRL+OPT+SHIFT+P
K PROP PROPERTIES| CMD+SHIFT+H
K -
K UNDO UNDO      | CMD+Z
K REDO REDO      | CMD+SHIFT+Z
K ENT  ENTER     | ENTER
K ESC  ESCAPE    | ESC
BLOCK 2 NEW + IMPORT
K NSEQ NEW SEQ   | CMD+N
K NBIN NEW BIN   | CMD+B
K IMP  IMPORT    | CMD+I
K IMB  IMPORT MB | CMD+OPT+I
K PROJ PROJECTS  | SHIFT+1
K MBRW MEDIA BRWS| SHIFT+8
K SRC  TO SOURCE | SHIFT+O
K -
K LINK LINK MEDIA| CTRL+OPT+SHIFT+L
K OFFL OFFLINE   | CTRL+OPT+SHIFT+O
K -
K -
K TAB  NEXT FIELD| TAB
K STAB PREV FIELD| SHIFT+TAB
K UP   UP        | UP
K DOWN DOWN      | DOWN
BLOCK 3 EXPORT + RENDER
K EXP  EXPORT    | CMD+M
K AME  TO ENCODER| OPT+SHIFT+M
K FRAM EXPORT FRM| CMD+SHIFT+E
K EXPM EXPORT MOD| OPT+4
K EDL  EXPORT EDL| CTRL+OPT+SHIFT+E
K XML  EXPORT XML| CTRL+OPT+SHIFT+X
K AAF  EXPORT AAF| CTRL+OPT+SHIFT+F
K -
K REND RENDER I/O| ENTER
K RAUD RENDER AUD| CTRL+OPT+SHIFT+A
K DREN DEL RENDER| CTRL+OPT+SHIFT+D
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
REV 1
APP DaVinci Resolve
LINK KEYS
COLOR F28FB8

PAGE SETUP
BLOCK 1 SETTINGS
K PREF PREFS     | CMD+,
K KEYS SHORTCUTS | CMD+OPT+K
K PSET PROJ SET  | SHIFT+9
K PMGR PROJ MGR  | SHIFT+1
K TLST TL SETTING| CTRL+OPT+SHIFT+T
K -
K -
K -
K RCOF CACHE NONE| CTRL+OPT+SHIFT+O
K RCSM CACHE SMRT| CTRL+OPT+SHIFT+S
K RCUS CACHE USER| CTRL+OPT+SHIFT+U
K RCCO CACHE CLIP| CTRL+OPT+SHIFT+C
K DRC  DEL CACHE | CTRL+OPT+SHIFT+D
K PRXY PROXY     | CTRL+OPT+SHIFT+P
K OPTM OPTIMIZED | CTRL+OPT+SHIFT+Z
BLOCK 2 PAGES
K MEDP MEDIA     | SHIFT+2
K CUTP CUT       | SHIFT+3
K EDIT EDIT      | SHIFT+4
K FUSN FUSION    | SHIFT+5
K COLR COLOR     | SHIFT+6
K FAIR FAIRLIGHT | SHIFT+7
K DLVR DELIVER   | SHIFT+8
K CNMA CINEMA    | P
BLOCK 3 DIALOG
K ENT  ENTER     | ENTER
K ESC  ESCAPE    | ESC
K TAB  NEXT FIELD| TAB
K STAB PREV FIELD| SHIFT+TAB
K UP   UP        | UP
K DOWN DOWN      | DOWN
K LEFT LEFT      | LEFT
K RGHT RIGHT     | RIGHT
K SPC  SPACE     | SPACE
K -
K UNDO UNDO      | CMD+Z
K SAVE SAVE      | CMD+S
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
REV 1
APP Premiere Pro
LINK KEYS
COLOR F28FB8

PAGE SETUP
BLOCK 1 SETTINGS
K PREF GENERAL   | CTRL+OPT+SHIFT+G
K KEYS SHORTCUTS | CMD+OPT+K
K AUD  AUDIO     | CTRL+OPT+SHIFT+U
K AHW  AUDIO HW  | CTRL+OPT+SHIFT+H
K MEDI MEDIA     | CTRL+OPT+SHIFT+I
K MCAC MEDIA CACH| CTRL+OPT+SHIFT+C
K PLAY PLAYBACK  | CTRL+OPT+SHIFT+B
K TIME TIMELINE  | CTRL+OPT+SHIFT+T
K AUTO AUTO SAVE | CTRL+OPT+SHIFT+V
K SURF CTRL SURF | CTRL+OPT+SHIFT+S
BLOCK 2 PROJECT + VIEW
K PSET PROJ SET  | CTRL+OPT+SHIFT+J
K SCR  SCRATCH   | CTRL+OPT+SHIFT+W
K SSET SEQ SET   | CTRL+OPT+SHIFT+Q
K PMGR PROJ MGR  | CTRL+OPT+SHIFT+P
K REND RENDER I/O| ENTER
K RAUD RENDER AUD| CTRL+OPT+SHIFT+A
K DREN DEL RENDER| CTRL+OPT+SHIFT+D
K -
K EDIT EDIT MODE | OPT+2
K COLR COLOR MODE| OPT+3
K EXPT EXPORT MOD| OPT+4
K WSRS RESET WS  | OPT+SHIFT+0
K MAX  MAXIMIZE  | SHIFT+`
K FULL FULLSCRN  | CTRL+`
BLOCK 3 DIALOG
K ENT  ENTER     | ENTER
K ESC  ESCAPE    | ESC
K TAB  NEXT FIELD| TAB
K STAB PREV FIELD| SHIFT+TAB
K UP   UP        | UP
K DOWN DOWN      | DOWN
K LEFT LEFT      | LEFT
K RGHT RIGHT     | RIGHT
K SPC  SPACE     | SPACE
K -
K UNDO UNDO      | CMD+Z
K SAVE SAVE      | CMD+S
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
