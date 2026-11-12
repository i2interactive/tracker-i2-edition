#ifndef INTERFACE_I2ARCADE_I2ARCADE_H_
#define INTERFACE_I2ARCADE_I2ARCADE_H_

// i2 edition: i2 Arcade, the module that runs i2 Arcade games on the Tracker.
// Opened from Config > i2 > Arcade. Holds a small menu and the KAGEMORI
// ports (QUIZ KAGEMORI, MEDAL MOUNTAIN).

#include "core/modulesBase.h"
#include "i2ArcadeApi.h"
#include "i2ArcadeCanvas.h"

class cI2Arcade : public ModuleBase
{
  public:
    virtual void update() override;
    virtual void start(uint32_t options) override;
    virtual void stop() override;
    virtual void initDisplayControls() override;
    virtual void destroyDisplayControls() override {}

    cI2Arcade() {}
    ~cI2Arcade() {}

    // input (called from the static handlers)
    void onPad(uint8_t pad, uint8_t state);
    void onButton(uint8_t button, uint8_t state);
    void onJog(int16_t value);

    void launch(uint8_t index); // 0 = menu, 1 = quiz, 2 = medal
    void refreshLabels();

  private:
    void showAll();

    i2arc::Game* active = nullptr;
    uint8_t activeIndex = 0;
    i2arc::CanvasData layerData[i2arc::kLayerCount];
    hControl canvas[i2arc::kLayerCount] = {};
    hControl label[8] = {};
    hControl bgLabel = -1;
    const char* lastLabel[8] = {};
};

extern cI2Arcade i2Arcade;

#endif /* INTERFACE_I2ARCADE_I2ARCADE_H_ */
