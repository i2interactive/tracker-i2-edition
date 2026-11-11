#ifndef INTERFACE_I2CONTROLLER_I2PADGRIDCONTROL_H_
#define INTERFACE_I2CONTROLLER_I2PADGRIDCONTROL_H_

#include "displayControls.h"
#include "i2ControllerProfile.h"

// i2 edition: draws the 4x12 pad map (label + key combo per pad) as one display
// control, so the Controller screen uses only a handful of the display's 65
// control slots. Rows 0-1 draw in stage 1 and rows 2-3 in stage 2 to keep each
// display-list chunk small.
struct I2PadGridData
{
    const i2ctl::Layer* layer = nullptr;
    bool mac = true;
    uint64_t pressedMask = 0;
};

// colors[]: 0 label text, 1 cell, 2 pressed cell, 3 combo text, 4 empty cell
class cI2PadGrid : public cDisplayControl
{
  public:
    cI2PadGrid(strControlProperties* properties = nullptr);
    virtual ~cI2PadGrid() {}

    virtual void update() override;
    virtual void update2() override;

    virtual void setStyle(uint32_t style) override { this->style = style; }
    virtual void setText(const char* text) override { this->text = text; }
    virtual void setValue(int value) override { this->value = value; }
    virtual void setColors(const uint32_t* colors) override { this->colors = colors; }
    virtual void setData(void* data) override { this->data = (I2PadGridData*)data; }

  private:
    void drawRows(uint8_t firstRow, uint8_t rowCount);

    I2PadGridData* data = nullptr;
};

#endif /* INTERFACE_I2CONTROLLER_I2PADGRIDCONTROL_H_ */
