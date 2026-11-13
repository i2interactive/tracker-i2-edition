#include "i2Deck.h"

using namespace i2arc;

void cI2Deck::initDisplayControls()
{
    // One full-screen canvas per layer. The display draws controls in the
    // reverse of the order they were created (newest first, oldest on top),
    // so the top layer is created first and the background last.
    for (int8_t l = kLayerCount - 1; l >= 0; l--)
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
