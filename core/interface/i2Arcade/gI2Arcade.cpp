#include "i2Arcade.h"

using namespace i2arc;

void cI2Arcade::initDisplayControls()
{
    // One canvas control per game layer, drawn in order (background first)
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
        canvas[l] = display.createControl<cI2ArcadeCanvas>(&prop);
    }

    for (uint8_t i = 0; i < 8; i++)
    {
        strControlProperties lp;
        lp.colors = display.getTheme().activeLabelsColors;
        lp.text = (char*)"";
        lp.style = (controlStyleShow | controlStyleCenterX | controlStyleCenterY | controlStyleFont3);
        lp.x = (800 / 8) * i + (800 / 16);
        lp.y = 424;
        lp.w = 800 / 8 - 6;
        lp.h = 55;
        lp.value = 0;
        label[i] = display.createControl<cLabel>(&lp);
    }

    strControlProperties bg;
    bg.text = nullptr;
    bg.colors = nullptr;
    bg.value = 255;
    bg.style = controlStyleNoTransparency | controlStyleShow;
    bg.x = 0;
    bg.w = 800;
    bg.y = 425;
    bg.h = 54;
    bgLabel = display.createControl<cBgLabel>(&bg);
}

void cI2Arcade::refreshLabels()
{
    if (active == nullptr)
    {
        return;
    }
    for (uint8_t i = 0; i < 8; i++)
    {
        const char* t = i == 7 ? (activeIndex == 0 ? "Exit" : "Menu") : active->buttonLabel(i);
        if (t == nullptr) t = "";
        if (t != lastLabel[i])
        {
            lastLabel[i] = t;
            display.setControlText(label[i], t);
            display.refreshControl(label[i]);
        }
    }
}

void cI2Arcade::showAll()
{
    titleBar->show(false);
    titleBar->setTitle(active ? active->name() : "i2 ARCADE");
    display.refreshControl(bgLabel);
    for (uint8_t l = 0; l < kLayerCount; l++)
    {
        display.refreshControl(canvas[l]);
    }
    refreshLabels();
    display.synchronizeRefresh();
}
