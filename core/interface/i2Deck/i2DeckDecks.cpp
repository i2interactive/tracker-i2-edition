#include "i2DeckApp.h"

// The decks that ship in the firmware. Each is written to the SD card
// (i2/Deck/<name>.txt) the first time it is opened; edit the file there to
// change anything. Delete the file to get the built-in version back.
//
//   DECK <name>            name on screen (7 letters)
//   APP <text>             what it drives
//   LINK KEYS|MIDI|RADIO   keyboard shortcuts, MIDI, or the FM tuner
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
//                          dials 1-6 there. In GRADE, button 6 switches
//                          between Resolve and Premiere, so dial 6 is unused.

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
# The labels are only names: the Hub decides what each note does.
DECK HUB
APP Creative Hub
LINK MIDI
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
BLOCK 3 SHAPE + QUALITY
K 1:1  SQUARE
K 16:9 WIDE
K 9:16 TALL
K 4:3  CLASSIC
K DRFT DRAFT
K GOOD GOOD
K BEST BEST
K UPSC UPSCALE
K S1   STYLE 1
K S2   STYLE 2
K S3   STYLE 3
K S4   STYLE 4
K S5   STYLE 5
K S6   STYLE 6
K S7   STYLE 7
K S8   STYLE 8
DIAL 1 STEPS
DIAL 2 CFG
DIAL 3 DENOISE
DIAL 4 SEED
DIAL 5 WIDTH
DIAL 6 HEIGHT
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
} // namespace

const BuiltinDeck kGradePremiereDeck = { "gradepr.txt", "GRADE", "PREMIERE", "Resolve or", "Premiere color.", "USB KEYS", 0xB48CFF, kGradePremiere, nullptr };

const BuiltinDeck kBuiltin[kBuiltinDecks] = {
    { "cut.txt", "CUT", "PREMIERE", "Edit, trim,", "move, mark.", "USB KEYS", 0x7FB2FF, kCut, nullptr },
    { "grade.txt", "GRADE", "RESOLVE", "Resolve or", "Premiere color.", "USB KEYS", 0xFFB454, kGrade, &kGradePremiereDeck },
    { "hub.txt", "HUB", "CREATIVE HUB", "Queue, slots,", "seeds, sizes.", "USB MIDI", 0x5BE3A5, kHub, nullptr },
    { "static.txt", "STATIC", "FM RADIO", "Tune, hear,", "glitch it.", "ANTENNA", 0xFF5A3C, kStatic, nullptr },
};

const BuiltinDeck* const kAll[kAllDecks] = { &kBuiltin[0], &kBuiltin[1], &kGradePremiereDeck, &kBuiltin[2], &kBuiltin[3] };

} // namespace i2deck
