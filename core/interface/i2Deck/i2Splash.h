#ifndef INTERFACE_I2DECK_I2SPLASH_H_
#define INTERFACE_I2DECK_I2SPLASH_H_

// i2 edition: the boot splash of the original Tracker.
//
// Drawn in place of Polyend's logo and loading bar while the Tracker opens
// its last project (cStartScreen). Everything is procedural dots on the
// 5x7 grid, so it costs no bitmaps. Like the decks, it only talks to the
// Gfx interface, so the PC harness renders it too.
//
// Time runs on the start screen's own 40 ms ticks, and the whole animation
// fits inside the ~2 s the Tracker already waits there, so it adds no boot
// time.

#include <stdint.h>

#include "i2Gfx.h"

namespace i2splash
{

// The i2 DECK release this firmware carries.
constexpr const char* kDeckVersion = "0.5";

enum Style : uint8_t
{
    kFull = 0,  // the whole sequence
    kShort = 1, // quicker; when the Tracker boots to MUSIC, or splash=short
    kStill = 2, // no motion and no pad sweep (splash=off)
};

// What the status line may report; set only what is true.
enum : uint8_t
{
    kStatusSd = 1,
    kStatusUsb = 2,   // a computer has configured the USB port
    kStatusRadio = 4, // the FM receiver answered at start-up
};

struct Frame
{
    uint32_t ms = 0;               // time into the splash
    uint8_t style = kFull;
    uint8_t progress = 0;          // real project-load progress, 0-100
    uint8_t status = 0;            // kStatus* bits
    const char* version = nullptr; // the Tracker firmware, e.g. "1.0.0 CE"
    const char* build = nullptr;   // build id (commit)
    const char* line = nullptr;    // what the Tracker is doing
};

// The splash at this point of its timeline.
void draw(i2arc::Gfx& g, const Frame& f);

// Black plus the mark's unlit grid: the frames before the start screen
// (power-on, font load). Identical to draw() at ms 0.
void drawGrid(i2arc::Gfx& g);

// True while something still moves (redraw every tick until then).
bool animating(const Frame& f);

// Pad light 0-255 for pad 0-47 (12 per row, top left first): one soft sweep
// across the grid in step with the colour band. 0 for every pad once done.
uint8_t padLevel(const Frame& f, uint8_t pad);

// Milliseconds until the sweep is over (pads can be released after this).
uint32_t padsDoneAt(uint8_t style);

} // namespace i2splash

#endif /* INTERFACE_I2DECK_I2SPLASH_H_ */
