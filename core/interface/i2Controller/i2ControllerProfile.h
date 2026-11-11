#ifndef INTERFACE_I2CONTROLLER_I2CONTROLLERPROFILE_H_
#define INTERFACE_I2CONTROLLER_I2CONTROLLERPROFILE_H_

// i2 edition: Controller profiles.
//
// A profile is a plain-text file on the SD card (i2/Controller/*.txt) that maps
// the 48 pads to keyboard shortcuts, in up to 8 layers. Format:
//
//   # comment
//   NAME Premiere Pro
//   LAYER Edit
//   1  Play/Stop   | SPACE
//   13 Add Edit    | CMD+K
//   21 Clear In/Out| OPT+X | CTRL+SHIFT+X      <- optional separate Windows combo
//
// Pads are numbered 1-48, left to right, top row first. CMD means Cmd on Mac
// and Ctrl on Windows; OPT/ALT is Option/Alt; CTRL is Control on both; WIN is
// the Windows/Command (GUI) key on both.

#include <stddef.h>
#include <stdint.h>

namespace i2ctl
{
constexpr uint8_t kPadCount = 48;
constexpr uint8_t kMaxLayers = 8;
constexpr uint8_t kLabelLen = 16;
constexpr uint8_t kNameLen = 24;

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

struct PadAction
{
    char label[kLabelLen];
    KeyCombo mac;
    KeyCombo win;
};

struct Layer
{
    char name[kNameLen];
    PadAction pads[kPadCount];
};

struct Profile
{
    char name[kNameLen];
    uint8_t layerCount;
    Layer layers[kMaxLayers];
};

// Parses profile text into `out` (cleared first). Returns the number of pad
// lines accepted; 0 means nothing usable was found.
int parseProfile(const char* text, size_t length, Profile& out);

// Parses one key spec like "CMD+SHIFT+K". Returns false on an unknown token.
bool parseKeySpec(const char* spec, size_t length, bool forMac, KeyCombo& out);

// Writes a short human-readable combo ("Cmd+Sh+K") for the pad grid.
void formatCombo(const KeyCombo& combo, bool forMac, char* buf, size_t bufLen);

extern const char kDefaultPremiereProfile[];
extern const size_t kDefaultPremiereProfileLength;

} // namespace i2ctl

#endif /* INTERFACE_I2CONTROLLER_I2CONTROLLERPROFILE_H_ */
