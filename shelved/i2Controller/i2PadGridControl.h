#ifndef INTERFACE_I2CONTROLLER_I2PADGRIDCONTROL_H_
#define INTERFACE_I2CONTROLLER_I2PADGRIDCONTROL_H_

#include "displayControls.h"
#include "i2ControllerProfile.h"

// i2 edition: draws two rows of the 4x12 pad map (label + key combo per pad).
// The Controller screen uses two of these (rows 1-2 and rows 3-4) so each
// control's display list stays well under its 8 KB slot in FT812 RAM_G, while
// using only 2 of the display's 65 control slots. `value` = first row (0 or 2).
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

    virtual void setStyle(uint32_t style) override { this->style = style; }
    virtual void setText(const char* text) override { this->text = text; }
    virtual void setValue(int value) override { this->value = value; }
    virtual void setColors(const uint32_t* colors) override { this->colors = colors; }
    virtual void setData(void* data) override { this->data = (I2PadGridData*)data; }

  private:
    void drawRows(uint8_t firstRow);

    I2PadGridData* data = nullptr;
};

#endif /* INTERFACE_I2CONTROLLER_I2PADGRIDCONTROL_H_ */
