#ifndef INTERFACE_I2CONTROLLER_I2CONTROLLER_H_
#define INTERFACE_I2CONTROLLER_I2CONTROLLER_H_

// i2 edition: Controller mode.
//
// Turns the Tracker into a shortcut controller for desktop apps (Premiere Pro
// first). The 48 pads send keyboard shortcuts over USB, the screen shows what
// each pad does, and the jog wheel scrubs. Opened from Config > i2 > Controller.

#include "core/modulesBase.h"
#include "i2ControllerProfile.h"
#include "i2PadGridControl.h"

class UsbKeyboard;

class cI2Controller : public ModuleBase
{
  public:
    enum JogMode : uint8_t
    {
        jogFrames,
        jogFiveFrames,
        jogEdits,
        jogZoom,
        jogModeCount
    };

    virtual void update() override;
    virtual void start(uint32_t options) override;
    virtual void stop() override;

    virtual void initDisplayControls() override;
    virtual void destroyDisplayControls() override {}

    cI2Controller() {}
    ~cI2Controller() {}

    // actions
    void padEvent(uint8_t pad, uint8_t state);
    void jog(int16_t value);
    void hardwareKey(uint8_t button, uint8_t state);
    void setShift(bool held);
    void nextLayer(int8_t direction);
    void nextJogMode();
    void toggleOs();
    void reloadProfile();

    // screen
    void showScreen();
    void refreshGrid(int8_t onlyHalf = -1); // -1 = both halves
    void refreshTitle();
    void refreshBottomLabels();
    void refreshPadLights();

  private:
    bool loadProfile();
    void loadSettings();
    void saveSettings();
    void tapCombo(i2ctl::KeyCombo combo);
    UsbKeyboard* keyboard();

    uint8_t layer = 0;
    bool mac = true;
    bool shiftHeld = false;
    uint8_t jogMode = jogFrames;
    bool profileLoaded = false;
    bool settingsLoaded = false;
    bool lastUsbState = false;
    uint8_t heldKey[i2ctl::kPadCount] = {};
    uint8_t heldValid[i2ctl::kPadCount] = {};

    I2PadGridData gridData;

    hControl grid[2] = { -1, -1 };
    hControl label[8] = { -1, -1, -1, -1, -1, -1, -1, -1 };
    hControl bgLabel = -1;

    char titleText[72];
    char rightText[24];
    char jogText[20];
    char osText[12];
};

extern cI2Controller i2Controller;

// Loaded profile and jog-mode names (defined in fI2Controller.cpp).
const i2ctl::Profile& i2ControllerProfile();
const char* i2ControllerJogLabel(uint8_t mode);

#endif /* INTERFACE_I2CONTROLLER_I2CONTROLLER_H_ */
