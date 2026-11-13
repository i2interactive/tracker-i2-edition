#ifndef INTERFACE_I2DECK_I2CANVAS_H_
#define INTERFACE_I2DECK_I2CANVAS_H_

// i2 edition: the display control that hosts one layer of an i2 screen, and
// the FT812 implementation of the Gfx interface.

#include "displayControls.h"
#include "i2Gfx.h"

namespace i2arc
{

// Draws through the FT812 coprocessor. Each layer's drawing is one burst into
// the chip's 4 KB command buffer, so the renderer keeps a budget and skips
// anything that would not fit (a busy frame loses a few details rather than
// corrupting the screen).
class Ft812Gfx final : public Gfx
{
  public:
    void beginLayer();
    void endLayer();

    virtual void color(uint32_t rgb) override;
    virtual void alpha(uint8_t a) override;
    virtual void rect(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint8_t radius = 0) override;
    virtual void circle(int16_t cx, int16_t cy, int16_t r) override;
    virtual void points(const int16_t* xy, int n, int16_t r) override;
    virtual void fillBelow(const int16_t* xy, int n) override;
    virtual void line(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint8_t width) override;
    virtual void text(int16_t x, int16_t y, uint8_t font, uint8_t align, const char* s, int len = 0) override;
    virtual int16_t textWidth(uint8_t font, const char* s, int len = -1) override;
    virtual int16_t fontHeight(uint8_t font) override;
    virtual void dot(int16_t x, int16_t y, uint8_t size, uint8_t align, const char* s, int len = 0) override;
    virtual void clip(int16_t x, int16_t y, int16_t w, int16_t h) override;
    virtual void unclip() override;

    uint16_t skipped() const { return skippedCount; }

  private:
    bool spend(uint16_t bytes);

    static constexpr uint16_t kBudget = 3800; // bytes of coprocessor commands per layer
    uint16_t used = 0;
    uint16_t skippedCount = 0;
};

struct CanvasData
{
    Game* game = nullptr;
    uint8_t layer = 0;
};

class cI2Canvas : public cDisplayControl
{
  public:
    cI2Canvas(strControlProperties* properties = nullptr);
    virtual ~cI2Canvas() {}

    virtual void update() override;

    virtual void setStyle(uint32_t style) override { this->style = style; }
    virtual void setText(const char* text) override { this->text = text; }
    virtual void setValue(int value) override { this->value = value; }
    virtual void setColors(const uint32_t* colors) override { this->colors = colors; }
    virtual void setData(void* data) override { this->data = (CanvasData*)data; }

  private:
    CanvasData* data = nullptr;
};

} // namespace i2arc

#endif /* INTERFACE_I2DECK_I2CANVAS_H_ */
