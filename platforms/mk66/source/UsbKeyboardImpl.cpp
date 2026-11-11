#include "UsbKeyboardImpl.h"

#include "usb_dev.h"
#include "usb_keyboard.h"

bool UsbKeyboardImpl::isConnected()
{
#ifdef KEYBOARD_INTERFACE
    return usb_configuration != 0;
#else
    return false;
#endif
}

void UsbKeyboardImpl::press(uint8_t keycode, uint8_t modifiers)
{
    int freeSlot = -1;
    for (int i = 0; i < kMaxKeys; i++)
    {
        if (keycode != 0 && keys[i] == keycode)
        {
            mods[i] = modifiers;
            sendReport();
            return;
        }
        if (freeSlot < 0 && keys[i] == 0 && mods[i] == 0)
        {
            freeSlot = i;
        }
    }

    if (freeSlot < 0)
    {
        return; // 6-key rollover limit
    }

    keys[freeSlot] = keycode; // keycode 0 = modifier-only "key"
    mods[freeSlot] = modifiers;
    sendReport();
}

void UsbKeyboardImpl::release(uint8_t keycode)
{
    for (int i = 0; i < kMaxKeys; i++)
    {
        if (keys[i] == keycode)
        {
            keys[i] = 0;
            mods[i] = 0;
        }
    }
    sendReport();
}

void UsbKeyboardImpl::releaseAll()
{
    for (int i = 0; i < kMaxKeys; i++)
    {
        keys[i] = 0;
        mods[i] = 0;
    }
    sendReport();
}

void UsbKeyboardImpl::sendReport()
{
#ifdef KEYBOARD_INTERFACE
    uint8_t modifierBits = 0;
    for (int i = 0; i < kMaxKeys; i++)
    {
        modifierBits |= mods[i];
        keyboard_keys[i] = keys[i];
    }
    keyboard_modifier_keys = modifierBits;

    if (usb_configuration)
    {
        usb_keyboard_send();
    }
#endif
}
