#include "i2PadGridControl.h"

#include "FT812.h"
#include "display.h"

#include <string.h>

namespace
{
constexpr uint8_t kCols = 12;
constexpr uint8_t kRowsTotal = 4;
constexpr uint8_t kRowsPerControl = 2;
constexpr int16_t kGap = 3;

constexpr int16_t kLabelFontIdx = 2; // 14 bold
constexpr int16_t kComboFontIdx = 4; // 12 regular

// Splits a label into at most two lines that fit `maxWidth`.
// Returns the length of line 1 and sets `secondStart` (or -1 for one line).
int splitLabel(const strFont* font, const char* label, int16_t maxWidth, int& secondStart)
{
    int len = (int)strlen(label);
    secondStart = -1;

    if (getTextWidth(font, label, len) <= maxWidth)
    {
        return len;
    }

    // Break at the space closest to the middle.
    int best = -1;
    for (int i = 1; i < len - 1; i++)
    {
        if (label[i] == ' ' || label[i] == '/')
        {
            if (best < 0 || (i - len / 2) * (i - len / 2) < (best - len / 2) * (best - len / 2))
            {
                best = i;
            }
        }
    }

    if (best < 0)
    {
        return getMaxTextLengthThatFitSpecificWidth(font, label, maxWidth);
    }

    if (label[best] == '/')
    {
        secondStart = best + 1;
        return best + 1; // keep the slash on line 1
    }
    secondStart = best + 1;
    return best;
}
} // namespace

cI2PadGrid::cI2PadGrid(strControlProperties* properties)
  : cDisplayControl(controlIdTemplate, 1)
{
    colorsCount = 5;

    if (properties == nullptr)
    {
        return;
    }

    colors = properties->colors;
    posX = properties->x;
    posY = properties->y;
    width = properties->w;
    height = properties->h;
    value = properties->value;
    text = properties->text;
    data = (I2PadGridData*)properties->data;
    style = properties->style;
}

void cI2PadGrid::update()
{
    drawRows((uint8_t)value);
}

void cI2PadGrid::drawRows(uint8_t firstRow)
{
    if (data == nullptr || data->layer == nullptr || colors == nullptr)
    {
        return;
    }

    const int16_t cellW = width / kCols;
    const int16_t cellH = height / kRowsPerControl;
    const uint8_t lastRow = firstRow + kRowsPerControl;
    const int16_t originX = posX + (width - cellW * kCols) / 2;

    const strFont* labelFont = &fonts[kLabelFontIdx];
    const strFont* comboFont = &fonts[kComboFontIdx];

    // 1. Cell backgrounds
    API_BEGIN(RECTS);
    for (uint8_t r = firstRow; r < lastRow && r < kRowsTotal; r++)
    {
        for (uint8_t c = 0; c < kCols; c++)
        {
            uint8_t pad = r * kCols + c;
            const i2ctl::PadAction& a = data->layer->pads[pad];
            bool empty = (a.label[0] == 0);
            bool pressed = (data->pressedMask >> pad) & 1ULL;

            API_COLOR(pressed ? colors[2] : (empty ? colors[4] : colors[1]));

            int16_t x0 = originX + c * cellW;
            int16_t y0 = posY + (r - firstRow) * cellH;
            API_VERTEX2F(x0 + kGap / 2, y0 + kGap / 2);
            API_VERTEX2F(x0 + cellW - kGap, y0 + cellH - kGap);
        }
    }
    API_END();

    // 2. Labels and combos
    for (uint8_t r = firstRow; r < lastRow && r < kRowsTotal; r++)
    {
        for (uint8_t c = 0; c < kCols; c++)
        {
            uint8_t pad = r * kCols + c;
            const i2ctl::PadAction& a = data->layer->pads[pad];
            if (a.label[0] == 0)
            {
                continue;
            }

            int16_t x0 = originX + c * cellW;
            int16_t y0 = posY + (r - firstRow) * cellH;
            int16_t cx = x0 + cellW / 2;

            int second = -1;
            int firstLen = splitLabel(labelFont, a.label, cellW - 6, second);
            int firstFit = getMaxTextLengthThatFitSpecificWidth(labelFont, a.label, cellW - 6);
            if (firstFit > 0 && firstLen > firstFit)
            {
                firstLen = firstFit;
            }

            API_COLOR(colors[0]);
            if (second < 0)
            {
                API_CMD_TEXT(cx, y0 + cellH / 2 - labelFont->height + 4, labelFont->handle,
                             OPT_CENTERX, a.label, firstLen);
            }
            else
            {
                const char* line2 = a.label + second;
                int line2Len = getMaxTextLengthThatFitSpecificWidth(labelFont, line2, cellW - 6);
                API_CMD_TEXT(cx, y0 + 10, labelFont->handle, OPT_CENTERX, a.label, firstLen);
                API_CMD_TEXT(cx, y0 + 10 + labelFont->height + 2, labelFont->handle, OPT_CENTERX,
                             line2, line2Len);
            }

            char combo[24];
            i2ctl::formatCombo(data->mac ? a.mac : a.win, data->mac, combo, sizeof(combo));
            if (combo[0] != 0)
            {
                int comboLen = getMaxTextLengthThatFitSpecificWidth(comboFont, combo, cellW - 4);
                API_COLOR(colors[3]);
                API_CMD_TEXT(cx, y0 + cellH - comboFont->height - 10, comboFont->handle,
                             OPT_CENTERX, combo, comboLen);
            }
        }
    }
}
