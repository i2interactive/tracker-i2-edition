#ifndef INTERFACE_I2DECK_I2GFX_H_
#define INTERFACE_I2DECK_I2GFX_H_

// i2 edition: shared drawing and app API for i2 screens (i2 DECK).
//
// Screens are written against these small interfaces only (no Tracker
// headers), so the same code runs in the firmware and in the PC test harness
// in tools/i2deck/host/, which renders frames to PNG.

#include <stddef.h>
#include <stdint.h>

namespace i2arc
{

// The canvas is the whole screen.
constexpr int16_t kCanvasX = 0;
constexpr int16_t kCanvasY = 0;
constexpr int16_t kCanvasW = 800;
constexpr int16_t kCanvasH = 480;

// Fonts. 0-4 are the Tracker's own fonts (exact metrics); 26-31 are the
// graphics chip's built-in ROM fonts (use only with alignment flags, not for
// measuring).
enum Font : uint8_t
{
    kFont12 = 4,     // 12 regular
    kFont14 = 0,     // 14 regular
    kFont14B = 2,    // 14 bold
    kFont17 = 1,     // 17 regular
    kFont17B = 3,    // 17 bold
    kRom20 = 27,     // ~20 px
    kRom25 = 28,     // ~25 px
    kRom28 = 29,     // ~28 px
    kRom36 = 30,     // ~36 px
    kRom49 = 31,     // ~49 px
};

enum Align : uint8_t
{
    kLeft = 0,
    kCenterX = 1,
    kRight = 2,
    kCenterY = 4,
};

// Drawing interface. Coordinates are screen pixels.
class Gfx
{
  public:
    virtual ~Gfx() {}
    virtual void color(uint32_t rgb) = 0;
    virtual void alpha(uint8_t a) = 0;
    // Filled rectangle, optional corner radius.
    virtual void rect(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint8_t radius = 0) = 0;
    // Filled circle.
    virtual void circle(int16_t cx, int16_t cy, int16_t r) = 0;
    // Many filled circles of one size and colour: xy = x0, y0, x1, y1, ...
    virtual void points(const int16_t* xy, int n, int16_t r) = 0;
    // Fills everything below the polyline xy (n points, left to right) down
    // to the bottom of the current clip rectangle. Used for skylines.
    virtual void fillBelow(const int16_t* xy, int n) = 0;
    virtual void line(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint8_t width) = 0;
    virtual void text(int16_t x, int16_t y, uint8_t font, uint8_t align, const char* s, int len = 0) = 0;
    // Exact for fonts 0-4; an estimate for ROM fonts.
    virtual int16_t textWidth(uint8_t font, const char* s, int len = -1) = 0;
    virtual int16_t fontHeight(uint8_t font) = 0;
    // Dot-matrix text in i2's 5x7 face. size 0 small, 1 medium, 2 large.
    // Upper case, plus a small i for "i2" (other lower case is drawn as upper).
    virtual void dot(int16_t x, int16_t y, uint8_t size, uint8_t align, const char* s, int len = 0) = 0;
    // Clip drawing to a rectangle (until the next clip / unclip).
    virtual void clip(int16_t x, int16_t y, int16_t w, int16_t h) = 0;
    virtual void unclip() = 0;
};

enum class Button : uint8_t
{
    Play,
    Left,
    Right,
    Up,
    Down,
    Shift,
    Screen0, // the 8 buttons under the screen
    Screen1,
    Screen2,
    Screen3,
    Screen4,
    Screen5,
    Screen6,
    Screen7,
    Delete,
    Copy,
    Insert,
};

enum class Sfx : uint8_t
{
    Select,
    Confirm,
    Buzz,
    Right,
    Wrong,
    Tick,
    Coin,
    Win,
    Big,
};

// What the Tracker (or the host harness) provides to games.
class Services
{
  public:
    virtual ~Services() {}
    virtual uint32_t random32() = 0;
    // Reads a text file from the SD card (paths under i2/Deck/). Returns the
    // length, or -1 if missing.
    virtual int32_t loadText(const char* path, char* buf, size_t cap) = 0;
    virtual bool saveText(const char* path, const char* data, size_t len) = 0;
    virtual void sfx(Sfx s) = 0;
    // Pad lights: level 0 off, 1 dim, 2 bright.
    virtual void padLight(uint8_t pad, uint8_t level) = 0;
    virtual void padsOff() = 0;
    // Large shared work memory (valid while the i2 screen is open).
    virtual uint8_t* scratch(size_t& size) = 0;
};

// Each layer is its own display control, redrawn only when dirty. A layer's
// drawing must fit the graphics chip's 4 KB command buffer (and its 8 KB
// display-list slot); split busy screens across layers.
constexpr uint8_t kLayerCount = 6; // 0 = background, 1-5 = foreground

class Game
{
  public:
    virtual ~Game() {}
    virtual const char* name() const = 0;
    virtual void begin(Services& sv) = 0;
    virtual void end() {}
    // Called every main-loop pass with the time in milliseconds.
    virtual void tick(uint32_t nowMs) = 0;
    virtual void draw(Gfx& g, uint8_t layer) = 0;
    // True when that layer changed since it was last drawn (draw() clears it).
    virtual bool dirty(uint8_t layer) const = 0;
    virtual void pad(uint8_t pad, bool down) { (void)pad; (void)down; }
    virtual void button(Button b, bool down) { (void)b; (void)down; }
    virtual void jog(int16_t steps) { (void)steps; }
    // Label for screen button 0-6 (button 7 is always Exit). nullptr = blank.
    virtual const char* buttonLabel(uint8_t i) const { (void)i; return nullptr; }
    // True when the game wants to return to the arcade menu.
    virtual bool wantsExit() const { return false; }
};

// Small helpers shared by games
inline uint32_t mix(uint32_t a, uint32_t b, float t)
{
    if (t < 0) t = 0;
    if (t > 1) t = 1;
    uint32_t r = (uint32_t)(((a >> 16) & 255) + (((int)((b >> 16) & 255) - (int)((a >> 16) & 255)) * t));
    uint32_t g = (uint32_t)(((a >> 8) & 255) + (((int)((b >> 8) & 255) - (int)((a >> 8) & 255)) * t));
    uint32_t bl = (uint32_t)((a & 255) + (((int)(b & 255) - (int)(a & 255)) * t));
    return (r << 16) | (g << 8) | bl;
}

// Uniform float in [0, 1)
inline float frand(Services& sv)
{
    return (sv.random32() >> 8) * (1.0f / 16777216.0f);
}

inline int irand(Services& sv, int n)
{
    return n > 0 ? (int)(sv.random32() % (uint32_t)n) : 0;
}

} // namespace i2arc

#endif /* INTERFACE_I2DECK_I2GFX_H_ */
