#ifndef INTERFACE_I2DECK_I2DECK_H_
#define INTERFACE_I2DECK_I2DECK_H_

// i2 edition: i2 DECK, the Tracker as a control surface.
//
// This module owns the whole screen (no Tracker title bar or button labels)
// and hands input to i2deck::DeckApp, which draws the interface and decides
// what to send: USB keyboard shortcuts, USB MIDI, or radio commands.
// Opened at start-up on the original Tracker, and from Config > i2 > Deck.

#include "core/modulesBase.h"
#include "i2Canvas.h"
#include "i2Gfx.h"

class Platform;

class cI2Deck : public ModuleBase
{
  public:
    virtual void update() override;
    virtual void start(uint32_t options) override;
    virtual void stop() override;
    virtual void initDisplayControls() override;
    virtual void destroyDisplayControls() override {}

    cI2Deck() {}
    ~cI2Deck() {}

    // input (called from the static handlers)
    void onPad(uint8_t pad, uint8_t state);
    void onButton(uint8_t button, uint8_t state);
    void onJog(int16_t value);

    // Asked for by the deck; carried out on the next update().
    void requestLeave(uint8_t where) { pendingLeave = (int8_t)where; }

    // True when the Tracker should start in i2 DECK (the default; put
    // "boot=music" in i2/Deck/settings.txt on the SD card to start in the
    // Tracker instead).
    static bool startsFirst(Platform* platform);

  private:
    bool running = false;
    int8_t pendingLeave = -1;
    i2arc::CanvasData layerData[i2arc::kLayerCount];
    hControl canvas[i2arc::kLayerCount] = {};
};

extern cI2Deck i2Deck;

#endif /* INTERFACE_I2DECK_I2DECK_H_ */
