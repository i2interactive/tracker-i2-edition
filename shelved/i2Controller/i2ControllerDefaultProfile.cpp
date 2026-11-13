#include "i2ControllerProfile.h"

// Default profile, written to i2/Controller/premiere.txt on first launch.
// It uses Premiere Pro's *default* shortcuts, so nothing has to be imported
// into Premiere. Edit the file on the SD card to change anything; the Tracker
// re-reads it with the "Reload" button.

namespace i2ctl
{
const char kDefaultPremiereProfile[] = R"PROFILE(# i2 Controller profile - Premiere Pro (default shortcuts)
# Pads 1-48: left to right, top row first.
# <pad> <label> | <combo> [| <Windows combo if different>]
# CMD = Cmd on Mac / Ctrl on Windows.  OPT = Option / Alt.
NAME Premiere Pro

LAYER Edit
1  Play/Stop    | SPACE
2  J Reverse    | J
3  K Stop       | K
4  L Forward    | L
5  Play Around  | SHIFT+K
6  Mark In      | I
7  Mark Out     | O
8  Mark Clip    | X
9  Clear In/Out | OPT+X | CTRL+SHIFT+X
10 Go to In     | SHIFT+I
11 Go to Out    | SHIFT+O
12 Match Frame  | F
13 Add Edit     | CMD+K
14 Add Edit All | CMD+SHIFT+K
15 Ripple Del   | OPT+BKSP
16 Lift         | ;
17 Extract      | '
18 Insert       | ,
19 Overwrite    | .
20 Trim Prev Q  | Q
21 Trim Next W  | W
22 Extend Edit  | E
23 Undo         | CMD+Z
24 Redo         | CMD+SHIFT+Z
25 Prev Edit    | UP
26 Next Edit    | DOWN
27 Back 1       | LEFT
28 Fwd 1        | RIGHT
29 Back 5       | SHIFT+LEFT
30 Fwd 5        | SHIFT+RIGHT
31 Seq Start    | HOME
32 Seq End      | END
33 Add Marker   | M
34 Next Marker  | SHIFT+M
35 Prev Marker  | CMD+SHIFT+M
36 Snap         | S
37 Select       | V
38 Track Sel    | A
39 Ripple Tool  | B
40 Roll Tool    | N
41 Rate Stretch | R
42 Razor        | C
43 Slip         | Y
44 Slide        | U
45 Pen          | P
46 Hand         | H
47 Zoom In      | =
48 Zoom Out     | -

LAYER Clips & Panels
1  Copy         | CMD+C
2  Paste        | CMD+V
3  Paste Insert | CMD+SHIFT+V
4  Paste Attr   | CMD+OPT+V
5  Cut          | CMD+X
6  Duplicate    | CMD+SHIFT+/
7  Select All   | CMD+A
8  Deselect All | CMD+SHIFT+A
9  Group        | CMD+G
10 Ungroup      | CMD+SHIFT+G
11 Link/Unlink  | CMD+L
12 Enable Clip  | SHIFT+E
13 Video Trans  | CMD+D
14 Audio Trans  | CMD+SHIFT+D
15 Default Tran | SHIFT+D
16 Speed/Dur    | CMD+R
17 Subclip      | CMD+U
18 Clear        | BKSP
19 Find         | CMD+F
20 Zoom to Seq  | BACKSLASH
21 Maximize     | SHIFT+`
22 Full Screen  | CTRL+`
23 Audio Gain   | G
24 Vol +1 dB    | ]
25 Project      | SHIFT+1
26 Source       | SHIFT+2
27 Timeline     | SHIFT+3
28 Program      | SHIFT+4
29 Effect Ctrls | SHIFT+5
30 Track Mixer  | SHIFT+6
31 Effects      | SHIFT+7
32 Media Brwsr  | SHIFT+8
33 Clip Mixer   | SHIFT+9
34 Vol -1 dB    | [
35 Vol +6 dB    | SHIFT+]
36 Vol -6 dB    | SHIFT+[
37 Import       | CMD+I
38 Export Media | CMD+M
39 Save         | CMD+S
40 Save As      | CMD+SHIFT+S
41 New Bin      | CMD+B
42 Escape       | ESC
43 Enter        | ENTER
44 Tab          | TAB
)PROFILE";

const size_t kDefaultPremiereProfileLength = sizeof(kDefaultPremiereProfile) - 1;

} // namespace i2ctl
