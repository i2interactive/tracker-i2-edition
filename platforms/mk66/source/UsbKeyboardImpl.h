#ifndef SOURCE_USBKEYBOARDIMPL_H_
#define SOURCE_USBKEYBOARDIMPL_H_

#include "Platform.h"

// i2 edition: HID keyboard on the original Tracker (Teensy USB core, MK66).
// Tracks up to 6 held keys, each with its own modifier bits, and sends the
// combined boot-keyboard report whenever the set changes.
class UsbKeyboardImpl final : public UsbKeyboard
{
  public:
    virtual bool isConnected() override;
    virtual void press(uint8_t keycode, uint8_t modifiers) override;
    virtual void release(uint8_t keycode) override;
    virtual void releaseAll() override;

  private:
    static constexpr int kMaxKeys = 6;
    uint8_t keys[kMaxKeys] = {};
    uint8_t mods[kMaxKeys] = {};

    void sendReport();
};

#endif /* SOURCE_USBKEYBOARDIMPL_H_ */
