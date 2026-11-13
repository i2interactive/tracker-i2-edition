#ifndef INTERFACE_I2DECK_I2DOTFONT_H_
#define INTERFACE_I2DECK_I2DOTFONT_H_

// i2 edition: the 5x7 dot-matrix face used by i2 DECK.
//
// One 320-byte table (ASCII 32-95) is scaled at boot into three bitmap fonts
// in the display chip's memory, so dot-matrix text costs a few bytes per
// string to draw and almost no flash.

#include <stdint.h>

namespace i2dot
{
constexpr uint8_t kSizes = 3;
constexpr uint8_t kFirstChar = 32;
constexpr uint8_t kGlyphs = 64;

// pixels per dot, and the lit part of each dot
constexpr uint8_t kPitch[kSizes] = { 2, 3, 6 };
constexpr uint8_t kLit[kSizes] = { 2, 2, 5 };

inline uint8_t cellW(uint8_t size) { return (uint8_t)(6 * kPitch[size]); }
inline uint8_t cellH(uint8_t size) { return (uint8_t)(7 * kPitch[size]); }
inline uint8_t stride(uint8_t size) { return (uint8_t)((cellW(size) + 7) / 8); }

// Column data for a character (5 bytes, bit 0 = top row). Lower case maps to
// upper case; anything outside the table is a space.
const uint8_t* columns(char c);

// Renders one glyph as a 1-bit bitmap (MSB = leftmost pixel) into `out`,
// which must hold stride(size) * cellH(size) bytes.
void renderGlyph(uint8_t size, char c, uint8_t* out);

// Where each size lives in the display chip: bitmap handle and RAM_G address
// (above the Tracker's own fonts, logo and display-list slots).
constexpr uint8_t kHandle[kSizes] = { 8, 9, 10 };
constexpr uint32_t kAddress[kSizes] = { 700000, 704000, 710000 };
constexpr uint16_t kMetricSize = 148;
constexpr uint32_t kMaxFontBytes = 148 + 64 * 5 * 42;

// Builds a complete chip font (148-byte metric block + 64 glyph bitmaps) for
// one size into `out` (kMaxFontBytes is always enough). Returns its length.
uint32_t buildFont(uint8_t size, uint8_t* out);

} // namespace i2dot

#endif /* INTERFACE_I2DECK_I2DOTFONT_H_ */
