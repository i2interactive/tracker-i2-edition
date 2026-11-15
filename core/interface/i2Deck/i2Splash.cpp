#include "i2Splash.h"

#include <math.h>
#include <string.h>

#include "i2DotFont.h"

namespace i2splash
{
using namespace i2arc;

namespace
{
constexpr uint32_t kInk = 0xF2F2EC, kGrey = 0xA3A39C, kSoft = 0xC9C9C2, kDim = 0x55555C;
constexpr uint32_t kGhost = 0x151518, kBarOff = 0x242428;
constexpr uint32_t kAccent[4] = { 0x19D36B, 0x3B62FF, 0xFF3D3D, 0xFFD60A };

// The mark: a big dotted "i2" on a 9 x 7 grid (the small i, one blank
// column, the 2), with "interactive" in small dots under it.
constexpr int16_t kCols = 9, kRows = 7;
constexpr int16_t kPitch = 28, kLit = 23, kRadius = 3;
constexpr int16_t kMarkW = (kCols - 1) * kPitch + kLit; // 247
constexpr int16_t kMarkX = 400 - kMarkW / 2;
constexpr int16_t kMarkY = 40;
constexpr int16_t kMarkBottom = kMarkY + (kRows - 1) * kPitch + kLit;

constexpr int16_t kSmallPitch = 4, kSmallLit = 3;
constexpr int16_t kWordY = kMarkBottom + 20;
constexpr int16_t kBandY = kWordY + 6 * kSmallPitch + kSmallLit + 19;
constexpr int16_t kBandH = 3;

constexpr int16_t kTitleY = 336;
constexpr int16_t kBarDots = 31, kBarPitch = 8, kBarLit = 5;
constexpr int16_t kBarX = 400 - ((kBarDots - 1) * kBarPitch + kBarLit) / 2;
constexpr int16_t kBarY = 374;
constexpr int16_t kLineY = 392;
constexpr int16_t kFootY = 456;

// "interactive" in lower case: the dot face has only upper case (and the
// small i of "i2"), so these seven letters live here. Columns, bit 0 = top.
struct Lower
{
    char c;
    uint8_t col[5];
};
const Lower kLower[] = {
    { 'n', { 0x7C, 0x08, 0x04, 0x04, 0x78 } }, { 't', { 0x04, 0x3F, 0x44, 0x40, 0x20 } },
    { 'e', { 0x38, 0x54, 0x54, 0x54, 0x18 } }, { 'r', { 0x7C, 0x08, 0x04, 0x04, 0x08 } },
    { 'a', { 0x20, 0x54, 0x54, 0x54, 0x78 } }, { 'c', { 0x38, 0x44, 0x44, 0x44, 0x20 } },
    { 'v', { 0x1C, 0x20, 0x40, 0x20, 0x1C } },
};
const char kWord[] = "interactive";
constexpr int kWordLen = sizeof(kWord) - 1;

const uint8_t* lowerColumns(char c)
{
    for (const Lower& l : kLower)
    {
        if (l.c == c) return l.col;
    }
    return i2dot::columns(c); // the small i
}

// first and last used column of a glyph
void span(const uint8_t* col, int& first, int& last)
{
    first = 0;
    last = 4;
    while (first < 4 && col[first] == 0) first++;
    while (last > first && col[last] == 0) last--;
}

struct Timing
{
    uint16_t drop0, dropStep, fall; // the mark's dots
    uint16_t band0, band;           // the colour band (and the pad sweep)
    uint16_t type0, typeStep, typeFade;
    uint16_t ui0, uiFade;           // title, bar, status
    uint16_t padSpread;             // how long each pad column glows, either side
};
const Timing kTiming[2] = {
    { 120, 22, 320, 640, 400, 900, 42, 50, 1150, 260, 140 }, // full
    { 0, 7, 200, 160, 240, 260, 0, 120, 300, 120, 100 },     // short
};

// The mark's dots in drop order: column by column, left to right, each
// column from the bottom up, so they stack as they land.
struct MarkDot
{
    int8_t col, row;
};
constexpr int kMaxMarkDots = 35;
MarkDot s_dots[kMaxMarkDots];
int s_dotCount = -1;

void buildMark()
{
    if (s_dotCount >= 0) return;
    s_dotCount = 0;
    const uint8_t* gi = i2dot::columns('i');
    const uint8_t* g2 = i2dot::columns('2');
    int f = 0, l = 0;
    span(gi, f, l);
    uint8_t cols[kCols] = {};
    int c = 0;
    for (int x = f; x <= l && c < kCols; x++) cols[c++] = gi[x];
    c++; // the blank column between i and 2
    for (int x = 0; x < 5 && c < kCols; x++) cols[c++] = g2[x];
    for (int x = 0; x < kCols; x++)
    {
        for (int y = kRows - 1; y >= 0; y--)
        {
            if ((cols[x] >> y) & 1)
            {
                if (s_dotCount < kMaxMarkDots) s_dots[s_dotCount++] = { (int8_t)x, (int8_t)y };
            }
        }
    }
}

float clamp01(float v)
{
    return v < 0 ? 0 : v > 1 ? 1 : v;
}

float easeOut3(float p)
{
    float q = 1 - p;
    return 1 - q * q * q;
}

float easeInOut(float p)
{
    return p < 0.5f ? 2 * p * p : 1 - 2 * (1 - p) * (1 - p);
}

// 0-1 along a phase that starts at t0 and lasts len ms (len 0 = a step)
float phase(const Frame& f, uint32_t t0, uint32_t len)
{
    if (f.style == kStill) return 1;
    if (f.ms < t0) return 0;
    if (len == 0) return 1;
    return clamp01((float)(f.ms - t0) / (float)len);
}

const Timing& timing(const Frame& f)
{
    return kTiming[f.style == kShort ? 1 : 0];
}

uint8_t a255(float v)
{
    return (uint8_t)(clamp01(v) * 255.0f + 0.5f);
}

void drawGhost(Gfx& g)
{
    int16_t row[kCols * 4];
    g.color(kGhost);
    for (int16_t y = 0; y < kRows; y++)
    {
        for (int16_t x = 0; x < kCols; x++)
        {
            int16_t x0 = (int16_t)(kMarkX + x * kPitch), y0 = (int16_t)(kMarkY + y * kPitch);
            row[x * 4] = x0;
            row[x * 4 + 1] = y0;
            row[x * 4 + 2] = (int16_t)(x0 + kLit);
            row[x * 4 + 3] = (int16_t)(y0 + kLit);
        }
        g.rects(row, kCols, kRadius);
    }
}

void drawBackground(Gfx& g)
{
    g.color(0x000000);
    g.rect(0, 0, 800, 480);
    drawGhost(g);
}

void drawMark(Gfx& g, const Frame& f)
{
    buildMark();
    const Timing& t = timing(f);
    int16_t landed[kMaxMarkDots * 4];
    int nLanded = 0;
    bool inked = false, faded = false; // colour / alpha already set

    for (int i = 0; i < s_dotCount; i++)
    {
        int16_t x0 = (int16_t)(kMarkX + s_dots[i].col * kPitch);
        int16_t y0 = (int16_t)(kMarkY + s_dots[i].row * kPitch);
        uint32_t start = t.drop0 + (uint32_t)i * t.dropStep;
        float p = phase(f, start, t.fall);
        if (p <= 0) continue;
        if (p >= 1)
        {
            landed[nLanded * 4] = x0;
            landed[nLanded * 4 + 1] = y0;
            landed[nLanded * 4 + 2] = (int16_t)(x0 + kLit);
            landed[nLanded * 4 + 3] = (int16_t)(y0 + kLit);
            nLanded++;
            // a soft glow as it lands, gone in 180 ms
            uint32_t landAt = start + t.fall;
            if (f.style != kStill && f.ms < landAt + 180)
            {
                float k = 1 - (float)(f.ms - landAt) / 180.0f;
                if (!inked) g.color(kInk);
                inked = true;
                faded = true;
                g.alpha(a255(0.22f * k));
                g.rect((int16_t)(x0 - 4), (int16_t)(y0 - 4), (int16_t)(x0 + kLit + 4), (int16_t)(y0 + kLit + 4), 6);
            }
            continue;
        }
        // still falling: eases in from above, fading up
        int16_t dy = (int16_t)(110.0f * (1.0f - easeOut3(p)));
        if (!inked) g.color(kInk);
        inked = true;
        faded = true;
        g.alpha(a255(p / 0.35f));
        g.rect(x0, (int16_t)(y0 - dy), (int16_t)(x0 + kLit), (int16_t)(y0 - dy + kLit), kRadius);
    }
    if (faded) g.alpha(255);
    if (nLanded > 0)
    {
        if (!inked) g.color(kInk);
        g.rects(landed, nLanded, kRadius);
    }
}

void drawWord(Gfx& g, const Frame& f)
{
    const Timing& t = timing(f);
    // total width, to centre it
    int cols = 0;
    for (int k = 0; k < kWordLen; k++)
    {
        int a = 0, b = 0;
        span(lowerColumns(kWord[k]), a, b);
        cols += (b - a + 1) + (k + 1 < kWordLen ? 1 : 0);
    }
    int16_t x = (int16_t)(400 - (cols * kSmallPitch - (kSmallPitch - kSmallLit)) / 2);
    int typed = 0;
    for (int k = 0; k < kWordLen; k++)
    {
        const uint8_t* col = lowerColumns(kWord[k]);
        int a = 0, b = 0;
        span(col, a, b);
        float p = phase(f, t.type0 + (uint32_t)k * t.typeStep, t.typeFade);
        if (p > 0)
        {
            typed = k + 1;
            int16_t r[35 * 4];
            int n = 0;
            for (int cx = a; cx <= b; cx++)
            {
                for (int cy = 0; cy < 7; cy++)
                {
                    if (!((col[cx] >> cy) & 1)) continue;
                    int16_t px = (int16_t)(x + (cx - a) * kSmallPitch), py = (int16_t)(kWordY + cy * kSmallPitch);
                    r[n * 4] = px;
                    r[n * 4 + 1] = py;
                    r[n * 4 + 2] = (int16_t)(px + kSmallLit);
                    r[n * 4 + 3] = (int16_t)(py + kSmallLit);
                    n++;
                }
            }
            g.color(kInk);
            if (p < 1) g.alpha(a255(p));
            g.rects(r, n, 1);
            if (p < 1) g.alpha(255);
        }
        x = (int16_t)(x + (b - a + 2) * kSmallPitch);
        // the cursor: a dotted underscore where the next letter goes
        if (f.style == kFull && typed == k + 1 && k + 1 < kWordLen && f.ms < t.type0 + (uint32_t)(k + 1) * t.typeStep)
        {
            int16_t r[5 * 4];
            for (int i = 0; i < 5; i++)
            {
                int16_t px = (int16_t)(x + i * kSmallPitch), py = (int16_t)(kWordY + 6 * kSmallPitch);
                r[i * 4] = px;
                r[i * 4 + 1] = py;
                r[i * 4 + 2] = (int16_t)(px + kSmallLit);
                r[i * 4 + 3] = (int16_t)(py + kSmallLit);
            }
            g.color(kAccent[0]);
            g.alpha(200);
            g.rects(r, 5, 1);
            g.alpha(255);
        }
    }
}

void drawBand(Gfx& g, const Frame& f)
{
    const Timing& t = timing(f);
    float p = phase(f, t.band0, t.band);
    if (p <= 0) return;
    int16_t w = (int16_t)(kMarkW * easeInOut(p) + 0.5f);
    int16_t seg = (int16_t)((kMarkW + 3) / 4);
    for (int i = 0; i < 4; i++)
    {
        int16_t x0 = (int16_t)(kMarkX + i * seg);
        int16_t x1 = (int16_t)(i == 3 ? kMarkX + kMarkW : x0 + seg);
        if (x1 > kMarkX + w) x1 = (int16_t)(kMarkX + w);
        if (x1 <= x0) break;
        g.color(kAccent[i]);
        g.rect(x0, kBandY, x1, (int16_t)(kBandY + kBandH));
    }
}

// One line of small dot items with a dot between them: measure (g null) or draw.
struct FootItem
{
    const char* text;
    uint32_t color;
};

int16_t footLine(Gfx* g, int16_t x, const FootItem* items, int n)
{
    int16_t x0 = x;
    bool first = true;
    for (int i = 0; i < n; i++)
    {
        if (items[i].text == nullptr || items[i].text[0] == 0) continue;
        if (!first)
        {
            if (g)
            {
                g->color(kDim);
                g->rect((int16_t)(x + 9), (int16_t)(kFootY + 6), (int16_t)(x + 11), (int16_t)(kFootY + 8));
            }
            x = (int16_t)(x + 20);
        }
        first = false;
        if (g)
        {
            g->color(items[i].color);
            g->dot(x, kFootY, 0, kLeft, items[i].text);
        }
        x = (int16_t)(x + (int16_t)strlen(items[i].text) * 12 - 2);
    }
    return (int16_t)(x - x0);
}

void drawLower(Gfx& g, const Frame& f)
{
    const Timing& t = timing(f);
    float p = phase(f, t.ui0, t.uiFade);
    if (p <= 0) return;
    const uint8_t a = a255(p);

    g.alpha(a);
    g.color(kInk);
    g.dot(400, kTitleY, 1, kCenterX, "i2 DECK");

    // the progress bar: real load progress, one dot per ~3%
    int prog = f.progress > 100 ? 100 : f.progress;
    int lit = (prog * kBarDots + 50) / 100;
    int16_t bar[kBarDots * 4];
    for (int i = 0; i < kBarDots; i++)
    {
        int16_t x0 = (int16_t)(kBarX + i * kBarPitch);
        bar[i * 4] = x0;
        bar[i * 4 + 1] = kBarY;
        bar[i * 4 + 2] = (int16_t)(x0 + kBarLit);
        bar[i * 4 + 3] = (int16_t)(kBarY + kBarLit);
    }
    if (lit < kBarDots)
    {
        g.color(kBarOff);
        g.rects(&bar[lit * 4], kBarDots - lit, 1);
    }
    // lit dots in white; while loading, the leading one in the band colour
    // of its quarter
    int white = lit < kBarDots ? lit - 1 : lit;
    if (white > 0)
    {
        g.color(kInk);
        g.rects(bar, white, 1);
    }
    if (lit > 0 && lit < kBarDots)
    {
        g.color(kAccent[((lit - 1) * 4) / kBarDots]);
        g.rects(&bar[(lit - 1) * 4], 1, 1);
    }

    if (f.line && f.line[0])
    {
        g.color(kGrey);
        g.text(400, kLineY, kFont12, kCenterX, f.line);
    }

    // bottom left: versions
    char v[12] = "V";
    strncat(v, kDeckVersion, sizeof(v) - 2);
    const FootItem left[3] = { { v, kGrey }, { f.version, kGrey }, { f.build, kGrey } };
    footLine(&g, 12, left, 3);

    // bottom right: only what is really there
    const bool sd = f.status & kStatusSd, usb = f.status & kStatusUsb, radio = f.status & kStatusRadio;
    const FootItem right[3] = { { sd ? "SD OK" : "NO SD", sd ? kSoft : kDim },
                                { usb ? "USB" : "NO USB", usb ? kSoft : kDim },
                                { radio ? "RADIO" : "NO RADIO", radio ? kSoft : kDim } };
    footLine(&g, (int16_t)(788 - footLine(nullptr, 0, right, 3)), right, 3);
    g.alpha(255);
}
} // namespace

void drawGrid(Gfx& g)
{
    drawBackground(g);
}

void draw(Gfx& g, const Frame& f)
{
    drawBackground(g);
    drawMark(g, f);
    drawWord(g, f);
    drawBand(g, f);
    drawLower(g, f);
}

bool animating(const Frame& f)
{
    if (f.style == kStill) return false;
    const Timing& t = timing(f);
    buildMark();
    uint32_t end = t.drop0 + (uint32_t)(s_dotCount > 0 ? s_dotCount - 1 : 0) * t.dropStep + t.fall + 180;
    uint32_t e2 = t.type0 + (uint32_t)(kWordLen - 1) * t.typeStep + t.typeFade;
    uint32_t e3 = t.ui0 + t.uiFade;
    uint32_t e4 = t.band0 + t.band;
    if (e2 > end) end = e2;
    if (e3 > end) end = e3;
    if (e4 > end) end = e4;
    return f.ms <= end;
}

uint8_t padLevel(const Frame& f, uint8_t pad)
{
    if (f.style == kStill || pad >= 48) return 0;
    const Timing& t = timing(f);
    float column = (float)(pad % 12) + 0.5f;
    // when the band's leading edge passes this column (easeInOut inverted)
    float y = column / 12.0f;
    float q = y < 0.5f ? sqrtf(y * 0.5f) : 1.0f - sqrtf((1.0f - y) * 0.5f);
    float centre = (float)t.band0 + (float)t.band * q;
    float d = (float)f.ms - centre;
    if (d < 0) d = -d;
    float k = 1.0f - d / (float)t.padSpread;
    if (k <= 0) return 0;
    return (uint8_t)(k * k * 255.0f);
}

uint32_t padsDoneAt(uint8_t style)
{
    if (style == kStill) return 0;
    const Timing& t = kTiming[style == kShort ? 1 : 0];
    return (uint32_t)t.band0 + t.band + t.padSpread + 40;
}

} // namespace i2splash
