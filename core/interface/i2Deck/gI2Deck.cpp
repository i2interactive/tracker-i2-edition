#include "i2Deck.h"

using namespace i2arc;

void cI2Deck::initDisplayControls()
{
    // One full-screen canvas per layer, drawn in order (background first).
    for (uint8_t l = 0; l < kLayerCount; l++)
    {
        strControlProperties prop;
        prop.colors = nullptr;
        prop.text = nullptr;
        prop.data = &layerData[l];
        prop.style = controlStyleShow;
        prop.x = kCanvasX;
        prop.y = kCanvasY;
        prop.w = kCanvasW;
        prop.h = kCanvasH;
        prop.value = l;
        canvas[l] = display.createControl<cI2Canvas>(&prop);
    }
}
