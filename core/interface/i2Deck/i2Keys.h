#ifndef INTERFACE_I2DECK_I2KEYS_H_
#define INTERFACE_I2DECK_I2KEYS_H_

// i2 edition: keyboard shortcuts for i2 DECK.
//
// Turns text like "CMD+SHIFT+K" into USB keyboard codes. CMD means Cmd on Mac
// and Ctrl on Windows; OPT/ALT is Option/Alt; CTRL is Control on both; WIN is
// the Windows/Command (GUI) key on both. Keys: letters, digits, F1-F24,
// SPACE ENTER ESC TAB BKSP DEL INS HOME END PGUP PGDN UP DOWN LEFT RIGHT,
// the punctuation keys - = [ ] ; ' ` , . / and BACKSLASH, and the number pad
// as KP0-KP9 KPPLUS KPMINUS KPENTER.

#include <stddef.h>
#include <stdint.h>

namespace i2ctl
{
// HID modifier bits
constexpr uint8_t kModCtrl = 0x01;
constexpr uint8_t kModShift = 0x02;
constexpr uint8_t kModAlt = 0x04;
constexpr uint8_t kModGui = 0x08;

struct KeyCombo
{
    uint8_t key;  // HID usage ID (page 0x07), 0 = modifiers only
    uint8_t mods; // HID modifier bits
    bool isEmpty() const { return key == 0 && mods == 0; }
};

// Parses one key spec like "CMD+SHIFT+K". Returns false on an unknown token.
bool parseKeySpec(const char* spec, size_t length, bool forMac, KeyCombo& out);

// Writes a short human-readable combo ("Cmd+Sh+K") for display.
void formatCombo(const KeyCombo& combo, bool forMac, char* buf, size_t bufLen);

} // namespace i2ctl

#endif /* INTERFACE_I2DECK_I2KEYS_H_ */
