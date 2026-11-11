#include "i2Controller.h"

#include <stdio.h>
#include <string.h>

#include "Platform.h"

namespace
{
// i2 look for the pad grid: label, cell, pressed cell, combo text, empty cell
const uint32_t kGridColors[5] = {
    0xFFFFFF,
    0x262626,
    0xE8372C,
    0x9A9A9A,
    0x121212,
};

constexpr int16_t kGridTop = 30;
constexpr int16_t kGridHeight = 392;
} // namespace

void cI2Controller::initDisplayControls()
{
    strControlProperties prop;

    // Pad grid
    prop.colors = kGridColors;
    prop.text = nullptr;
    prop.data = &gridData;
    prop.style = controlStyleShow;
    prop.x = 0;
    prop.y = kGridTop;
    prop.w = 800;
    prop.h = kGridHeight;
    prop.value = 0;
    grid = display.createControl<cI2PadGrid>(&prop);

    // Bottom screen-button labels
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

void cI2Controller::showScreen()
{
    refreshTitle();
    refreshBottomLabels();
    display.refreshControl(bgLabel);
    refreshGrid();
    display.synchronizeRefresh();
}

void cI2Controller::refreshGrid()
{
    const i2ctl::Profile& p = i2ControllerProfile();
    gridData.layer = (p.layerCount > 0 && layer < p.layerCount) ? &p.layers[layer] : nullptr;
    gridData.mac = mac;
    display.setControlData(grid, &gridData);
    display.refreshControl(grid);
}

void cI2Controller::refreshTitle()
{
    const i2ctl::Profile& p = i2ControllerProfile();
    uint8_t count = p.layerCount;
    const i2ctl::Layer* l = (count > 0 && layer < count) ? &p.layers[layer] : nullptr;

    snprintf(titleText,
             sizeof(titleText),
             "%s - %s  %d/%d%s",
             count > 0 ? p.name : "-",
             l != nullptr ? l->name : "-",
             count > 0 ? layer + 1 : 0,
             count,
             shiftHeld ? "  +SHIFT" : "");

    UsbKeyboard* kb = platform != nullptr ? platform->getUsbKeyboard() : nullptr;
    const char* usb = (kb == nullptr) ? "No USB keys" : (kb->isConnected() ? "USB" : "USB off");
    snprintf(rightText, sizeof(rightText), "%s  %s", mac ? "MAC" : "WIN", usb);

    titleBar->show(true);
    titleBar->setTitle(titleText);
    titleBar->setRightLabel(rightText);
}

void cI2Controller::refreshBottomLabels()
{
    snprintf(jogText, sizeof(jogText), "%s", i2ControllerJogLabel(jogMode));
    snprintf(osText, sizeof(osText), "%s", mac ? "OS: Mac" : "OS: Win");

    const char* texts[8] = { "< Layer", "Layer >", jogText, osText, "Reload", "", "", "Close" };
    for (uint8_t i = 0; i < 8; i++)
    {
        display.setControlText(label[i], texts[i]);
        display.refreshControl(label[i]);
    }
}
