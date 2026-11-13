#include "i2Canvas.h"

#include <string.h>

#include "FT812.h"
#include "display.h"
#include "i2DotFont.h"

namespace i2arc
{
namespace
{
// Tracker fonts 0-4 are display fonts[]; 26-31 are FT812 ROM fonts.
bool isRomFont(uint8_t font)
{
    return font >= 16 && font <= 31;
}

const strFont* trackerFont(uint8_t font)
{
    return &fonts[font < 5 ? font : 0];
}

int16_t romHeight(uint8_t font)
{
    switch (font)
    {
        case 26: return 16;
        case 27: return 20;
        case 28: return 25;
        case 29: return 28;
        case 30: return 36;
        case 31: return 49;
        default: return 16;
    }
}
} // namespace

//==============================================================================================================
// Ft812Gfx
//==============================================================================================================
bool Ft812Gfx::spend(uint16_t bytes)
{
    if (used + bytes > kBudget)
    {
        skippedCount++;
        return false;
    }
    used = (uint16_t)(used + bytes);
    return true;
}

void Ft812Gfx::beginLayer()
{
    used = 0;
    skippedCount = 0;
    spend(16);
    API_SAVE_CONTEXT();
    API_COLOR_A(255);
    API_LINE_WIDTH(16);
}

void Ft812Gfx::endLayer()
{
    // always room: reserved by kBudget's margin
    API_RESTORE_CONTEXT();
}

void Ft812Gfx::color(uint32_t rgb)
{
    if (spend(4)) API_COLOR(rgb & 0xFFFFFF);
}

void Ft812Gfx::alpha(uint8_t a)
{
    if (spend(4)) API_COLOR_A(a);
}

void Ft812Gfx::rect(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint8_t radius)
{
    if (!spend(20)) return;
    // EVE idiom: the line width is the corner radius and the vertices are the
    // centres of the corner arcs, so inset them by the radius.
    int16_t r = radius ? radius : 1;
    API_LINE_WIDTH((uint16_t)(r * 16));
    API_BEGIN(RECTS);
    API_VERTEX2F((int16_t)(x0 + r), (int16_t)(y0 + r));
    API_VERTEX2F((int16_t)(x1 - 1 - r), (int16_t)(y1 - 1 - r));
    API_END();
}

void Ft812Gfx::circle(int16_t cx, int16_t cy, int16_t r)
{
    if (!spend(16)) return;
    API_POINT_SIZE((uint16_t)(r * 16));
    API_BEGIN(FTPOINTS);
    API_VERTEX2F(cx, cy);
    API_END();
}

void Ft812Gfx::points(const int16_t* xy, int n, int16_t r)
{
    if (n <= 0) return;
    if (!spend(12)) return;
    API_POINT_SIZE((uint16_t)(r * 16));
    API_BEGIN(FTPOINTS);
    for (int i = 0; i < n; i++)
    {
        if (!spend(4)) break;
        API_VERTEX2F(xy[2 * i], xy[2 * i + 1]);
    }
    API_END();
}

void Ft812Gfx::fillBelow(const int16_t* xy, int n)
{
    if (n < 2) return;
    if (!spend((uint16_t)(12 + 4 * n))) return;
    API_BEGIN(EDGE_STRIP_B);
    for (int i = 0; i < n; i++)
    {
        API_VERTEX2F(xy[2 * i], xy[2 * i + 1]);
    }
    API_END();
}

void Ft812Gfx::line(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint8_t width)
{
    if (!spend(20)) return;
    // LINE_WIDTH is the half-width in 1/16 px
    API_LINE_WIDTH((uint16_t)(width ? width * 8 : 8));
    API_BEGIN(LINES);
    API_VERTEX2F(x0, y0);
    API_VERTEX2F(x1, y1);
    API_END();
}

void Ft812Gfx::text(int16_t x, int16_t y, uint8_t font, uint8_t align, const char* s, int len)
{
    if (s == nullptr || *s == 0) return;
    int n = len > 0 ? len : (int)strlen(s);
    if (!spend((uint16_t)(12 + ((n + 4) & ~3)))) return;
    uint16_t opt = 0;
    if (align & kCenterX) opt |= OPT_CENTERX;
    else if (align & kRight) opt |= OPT_RIGHTX;
    if (align & kCenterY) opt |= OPT_CENTERY;
    int16_t handle = isRomFont(font) ? font : trackerFont(font)->handle;
    if (len > 0)
    {
        API_CMD_TEXT(x, y, handle, opt, s, (uint16_t)n);
    }
    else
    {
        API_CMD_TEXT(x, y, handle, opt, s);
    }
}

int16_t Ft812Gfx::textWidth(uint8_t font, const char* s, int len)
{
    if (s == nullptr) return 0;
    int n = len >= 0 ? len : (int)strlen(s);
    if (isRomFont(font))
    {
        return (int16_t)(n * romHeight(font) * 11 / 20); // estimate
    }
    return (int16_t)getTextWidth(trackerFont(font), s, (uint16_t)n);
}

int16_t Ft812Gfx::fontHeight(uint8_t font)
{
    return isRomFont(font) ? romHeight(font) : trackerFont(font)->height;
}

void Ft812Gfx::dot(int16_t x, int16_t y, uint8_t size, uint8_t align, const char* s, int len)
{
    if (s == nullptr || *s == 0 || size >= i2dot::kSizes) return;
    // The dot fonts hold ASCII 32-95 only: fold lower case, blank the rest.
    char buf[32];
    int n = len > 0 ? len : (int)strlen(s);
    if (n > (int)sizeof(buf) - 1) n = (int)sizeof(buf) - 1;
    for (int i = 0; i < n; i++)
    {
        char c = s[i];
        if (c >= 'a' && c <= 'z') c = (char)(c - 'a' + 'A');
        buf[i] = (c >= 32 && c <= 95) ? c : ' ';
    }
    buf[n] = 0;
    if (!spend((uint16_t)(12 + ((n + 4) & ~3)))) return;
    uint16_t opt = 0;
    if (align & kCenterX) opt |= OPT_CENTERX;
    else if (align & kRight) opt |= OPT_RIGHTX;
    API_CMD_TEXT(x, y, i2dot::kHandle[size], opt, buf);
}

void Ft812Gfx::clip(int16_t x, int16_t y, int16_t w, int16_t h)
{
    if (!spend(8)) return;
    API_SCISSOR_XY((uint16_t)(x < 0 ? 0 : x), (uint16_t)(y < 0 ? 0 : y));
    API_SCISSOR_SIZE((uint16_t)(w < 0 ? 0 : w), (uint16_t)(h < 0 ? 0 : h));
}

void Ft812Gfx::unclip()
{
    // keep 8 bytes back so the clip can always be undone
    used = (uint16_t)(used + 8);
    API_SCISSOR_XY(0, 0);
    API_SCISSOR_SIZE(800, 480);
}

//==============================================================================================================
// cI2Canvas
//==============================================================================================================
static Ft812Gfx s_gfx;

cI2Canvas::cI2Canvas(strControlProperties* properties)
  : cDisplayControl(controlIdTemplate, 1)
{
    colorsCount = 0;
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
    data = (CanvasData*)properties->data;
    style = properties->style;
}

void cI2Canvas::update()
{
    if (data == nullptr || data->game == nullptr)
    {
        return;
    }
    s_gfx.beginLayer();
    data->game->draw(s_gfx, data->layer);
    s_gfx.endLayer();
}

} // namespace i2arc
