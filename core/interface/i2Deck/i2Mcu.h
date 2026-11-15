#ifndef INTERFACE_I2DECK_I2MCU_H_
#define INTERFACE_I2DECK_I2MCU_H_

// i2 edition: the Mackie Control (MCU) protocol, as the MIXER deck speaks it
// over the Tracker's USB MIDI port. Sources: tools/i2deck/APP_DECKS.md.
//
//   buttons   note on, channel 1, velocity 127 = press, 0 = release.
//             REC 0-7, SOLO 8-15, MUTE 16-23, SELECT 24-31 (strips 1-8),
//             bank left/right 46/47, channel left/right 48/49, transport
//             91-95, fader touch 104-111 (master 112)
//   faders    pitch bend, channel 1-8 (master 9), 14 bits, 0 = bottom
//   V-Pots    control change 16-23, relative: 1-63 clockwise ticks,
//             65-127 anticlockwise (64 + ticks); the jog wheel is CC 60
//   from the host: button LEDs as notes (127 on, 1 blink, 0 off), fader
//             positions as pitch bend, meters as channel pressure (strip in
//             the high nibble, level 0-12 in the low), V-Pot rings as CC
//             48-55, the 2x56 character display as sysex 12.

#include <stdint.h>

namespace i2mcu
{
constexpr uint8_t kRec = 0x00, kSolo = 0x08, kMute = 0x10, kSelect = 0x18;
constexpr uint8_t kBankLeft = 0x2E, kBankRight = 0x2F, kChannelLeft = 0x30, kChannelRight = 0x31;
constexpr uint8_t kTouch = 0x68, kMasterTouch = 0x70;
constexpr uint8_t kVpot = 0x10, kVpotRing = 0x30, kJog = 0x3C;
constexpr uint8_t kStrips = 8;
constexpr uint8_t kMasterChannel = 8; // pitch bend on MIDI channel 9
constexpr uint16_t kFaderMax = 16383;
constexpr uint8_t kLcdChars = 112;    // two rows of 56
constexpr uint8_t kMeterMax = 12;
constexpr uint8_t kModel = 0x14;      // Mackie Control (main unit)

// Relative encoder value for `ticks` clicks (+ clockwise, - anticlockwise).
inline uint8_t relative(int ticks)
{
    if (ticks > 63) ticks = 63;
    if (ticks < -63) ticks = -63;
    return ticks >= 0 ? (uint8_t)ticks : (uint8_t)(0x40 | -ticks);
}

// Pitch-bend bytes for a fader position.
inline void fader(uint16_t pos, uint8_t channel, uint8_t out[3])
{
    if (pos > kFaderMax) pos = kFaderMax;
    out[0] = (uint8_t)(0xE0 | (channel & 0x0F));
    out[1] = (uint8_t)(pos & 0x7F);
    out[2] = (uint8_t)((pos >> 7) & 0x7F);
}

// Is this sysex for a Mackie Control (or Logic Control) main unit?
inline bool ours(const uint8_t* d, uint16_t n)
{
    return n >= 7 && d[0] == 0xF0 && d[1] == 0x00 && d[2] == 0x00 && d[3] == 0x66 && (d[4] == kModel || d[4] == 0x10);
}

// Reply to the host's sysex, if it asks for one. Returns the reply length
// (0 = none). `out` must hold 20 bytes.
inline uint16_t reply(const uint8_t* d, uint16_t n, uint8_t* out)
{
    static const uint8_t kSerial[7] = { 'I', '2', 'D', 'E', 'C', 'K', '1' };
    if (!ours(d, n)) return 0;
    uint16_t k = 0;
    out[k++] = 0xF0;
    out[k++] = 0x00;
    out[k++] = 0x00;
    out[k++] = 0x66;
    out[k++] = d[4];
    switch (d[5])
    {
        case 0x00: // device query: "host connection query" with serial + challenge
            out[k++] = 0x01;
            for (uint8_t b : kSerial) out[k++] = b;
            for (int i = 0; i < 4; i++) out[k++] = 0x00;
            break;
        case 0x02: // host connection reply: confirm
            out[k++] = 0x03;
            for (uint8_t b : kSerial) out[k++] = b;
            break;
        case 0x13: // version request
            out[k++] = 0x14;
            out[k++] = 'V';
            out[k++] = '1';
            out[k++] = '.';
            out[k++] = '0';
            out[k++] = '0';
            break;
        default: return 0;
    }
    out[k++] = 0xF7;
    return k;
}
} // namespace i2mcu

#endif /* INTERFACE_I2DECK_I2MCU_H_ */
