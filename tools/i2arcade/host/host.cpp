// i2 Arcade host harness: runs a game from core/interface/i2Arcade on a PC with
// scripted input, records every frame's drawing commands, and reports the
// display-list size per layer (each must stay under the Tracker's 8 KB
// per-control slot). render.py turns the recordings into PNGs.
//
// Build: see run.sh.   Usage: host <quiz|medal> <sdroot> <outdir> <script>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <string>
#include <vector>

#include "../../../core/interface/i2Arcade/i2ArcadeApi.h"
#include "../../../core/interface/i2Arcade/i2MedalGame.h"
#include "../../../core/interface/i2Arcade/i2QuizGame.h"
#include "font_metrics.h"

using namespace i2arc;

static int fontIndex(uint8_t font)
{
    for (size_t i = 0; i < sizeof(kHostFontIds); i++)
    {
        if (kHostFontIds[i] == font) return (int)i;
    }
    return 0;
}

class RecGfx : public Gfx
{
  public:
    std::string out;
    size_t dl = 0;
    size_t cmd = 0; // coprocessor command bytes (must fit the 4 KB FIFO)
    int maxClipNesting = 0;

    void reset()
    {
        out.clear();
        dl = 0;
        cmd = 0;
    }
    void color(uint32_t rgb) override
    {
        char b[32];
        snprintf(b, sizeof(b), "C %06x\n", rgb & 0xFFFFFF);
        out += b;
        dl += 4;
        cmd += 4;
    }
    void alpha(uint8_t a) override
    {
        char b[16];
        snprintf(b, sizeof(b), "A %d\n", a);
        out += b;
        dl += 4;
        cmd += 4;
    }
    void rect(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint8_t radius) override
    {
        char b[64];
        snprintf(b, sizeof(b), "R %d %d %d %d %d\n", x0, y0, x1, y1, radius);
        out += b;
        dl += 20;
        cmd += 20;
    }
    void circle(int16_t cx, int16_t cy, int16_t r) override
    {
        char b[48];
        snprintf(b, sizeof(b), "O %d %d %d\n", cx, cy, r);
        out += b;
        dl += 16;
        cmd += 16;
    }
    void points(const int16_t* xy, int n, int16_t r) override
    {
        if (n <= 0) return;
        char b[48];
        snprintf(b, sizeof(b), "Q %d %d", r, n);
        out += b;
        for (int i = 0; i < n; i++)
        {
            snprintf(b, sizeof(b), " %d %d", xy[2 * i], xy[2 * i + 1]);
            out += b;
        }
        out += "\n";
        dl += 12 + 4 * n;
        cmd += 12 + 4 * n;
    }
    void fillBelow(const int16_t* xy, int n) override
    {
        if (n <= 1) return;
        char b[48];
        snprintf(b, sizeof(b), "F %d", n);
        out += b;
        for (int i = 0; i < n; i++)
        {
            snprintf(b, sizeof(b), " %d %d", xy[2 * i], xy[2 * i + 1]);
            out += b;
        }
        out += "\n";
        dl += 8 + 4 * n;
        cmd += 12 + 4 * n;
    }
    void line(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint8_t width) override
    {
        char b[64];
        snprintf(b, sizeof(b), "L %d %d %d %d %d\n", x0, y0, x1, y1, width);
        out += b;
        dl += 20;
        cmd += 20;
    }
    void text(int16_t x, int16_t y, uint8_t font, uint8_t align, const char* s, int len) override
    {
        if (s == nullptr) return;
        int n = len > 0 ? len : (int)strlen(s);
        char b[64];
        snprintf(b, sizeof(b), "T %d %d %d %d %d|", x, y, font, align, n);
        out += b;
        out.append(s, (size_t)n);
        out += "\n";
        int glyphs = 0;
        for (int i = 0; i < n; i++) glyphs += s[i] != ' ';
        dl += 16 + 4 * glyphs;
        cmd += 12 + ((n + 4) & ~3);
    }
    int16_t textWidth(uint8_t font, const char* s, int len) override
    {
        if (s == nullptr) return 0;
        int n = len >= 0 ? len : (int)strlen(s);
        int fi = fontIndex(font);
        int w = 0;
        for (int i = 0; i < n && s[i]; i++)
        {
            unsigned c = (unsigned char)s[i];
            w += (c >= 32 && c < 127) ? kHostAdvance[fi][c - 32] : 6;
        }
        return (int16_t)w;
    }
    int16_t fontHeight(uint8_t font) override { return kHostFontHeight[fontIndex(font)]; }
    void clip(int16_t x, int16_t y, int16_t w, int16_t h) override
    {
        char b[64];
        snprintf(b, sizeof(b), "S %d %d %d %d\n", x, y, w, h);
        out += b;
        dl += 8;
        cmd += 8;
    }
    void unclip() override
    {
        out += "U\n";
        dl += 8;
        cmd += 8;
    }
};

class HostServices : public Services
{
  public:
    std::string root;
    uint32_t rng = 0x12345678;
    std::vector<uint8_t> scr = std::vector<uint8_t>(768 * 1024);
    uint8_t pads[48] = {};
    std::string sfxLog;

    uint32_t random32() override
    {
        rng ^= rng << 13;
        rng ^= rng >> 17;
        rng ^= rng << 5;
        return rng;
    }
    int32_t loadText(const char* path, char* buf, size_t cap) override
    {
        std::string p = root + "/" + path;
        FILE* f = fopen(p.c_str(), "rb");
        if (!f) return -1;
        size_t n = fread(buf, 1, cap, f);
        fclose(f);
        return (int32_t)n;
    }
    bool saveText(const char* path, const char* data, size_t len) override
    {
        std::string p = root + "/" + path;
        FILE* f = fopen(p.c_str(), "wb");
        if (!f) return false;
        fwrite(data, 1, len, f);
        fclose(f);
        return true;
    }
    void sfx(Sfx s) override
    {
        sfxLog += std::to_string((int)s) + " ";
    }
    void padLight(uint8_t pad, uint8_t level) override
    {
        if (pad < 48) pads[pad] = level;
    }
    void padsOff() override { memset(pads, 0, sizeof(pads)); }
    uint8_t* scratch(size_t& size) override
    {
        size = scr.size();
        return scr.data();
    }
};

static QuizGame quiz;
static MedalGame medal;

int main(int argc, char** argv)
{
    if (argc < 5)
    {
        fprintf(stderr, "usage: host <quiz|medal> <sdroot> <outdir> <script>\n");
        return 2;
    }
    Game* game = strcmp(argv[1], "quiz") == 0 ? (Game*)&quiz : (Game*)&medal;
    HostServices sv;
    sv.root = argv[2];
    std::string outDir = argv[3];
    FILE* sc = fopen(argv[4], "r");
    if (!sc)
    {
        fprintf(stderr, "no script\n");
        return 2;
    }

    game->begin(sv);
    uint32_t now = 1000;
    uint32_t frames = 0;
    size_t worst[kLayerCount] = {};
    size_t worstCmd[kLayerCount] = {};
    RecGfx g;

    auto advance = [&](int n) {
        for (int i = 0; i < n; i++)
        {
            now += (frames % 3 == 2) ? 16 : 17;
            frames++;
            game->tick(now);
            // draw every dirty layer, as the Tracker does, and measure it
            for (uint8_t layer = 0; layer < kLayerCount; layer++)
            {
                if (!game->dirty(layer)) continue;
                g.reset();
                game->draw(g, layer);
                if (g.dl > worst[layer]) worst[layer] = g.dl;
                if (g.cmd > worstCmd[layer]) worstCmd[layer] = g.cmd;
            }
        }
    };

    auto shot = [&](const char* name) {
        std::string path = outDir + "/" + name + ".txt";
        FILE* f = fopen(path.c_str(), "w");
        for (uint8_t layer = 0; layer < kLayerCount; layer++)
        {
            g.reset();
            game->draw(g, layer);
            if (g.dl > worst[layer]) worst[layer] = g.dl;
            if (g.cmd > worstCmd[layer]) worstCmd[layer] = g.cmd;
            fprintf(f, "# layer %d dl=%zu\n", layer, g.dl);
            fputs(g.out.c_str(), f);
        }
        for (uint8_t i = 0; i < 7; i++)
        {
            const char* l = game->buttonLabel(i);
            fprintf(f, "B %d %s\n", i, l ? l : "");
        }
        fprintf(f, "B 7 Exit\n");
        fprintf(f, "P ");
        for (int p = 0; p < 48; p++) fprintf(f, "%d", sv.pads[p]);
        fprintf(f, "\nN %s\n", game->name());
        fclose(f);
        printf("shot %-18s frame %5u  dl0=%zu dl1=%zu\n", name, frames, (size_t)0, g.dl);
    };

    char line[256];
    while (fgets(line, sizeof(line), sc))
    {
        char cmd[32] = "";
        int a = 0, b = 0;
        char name[64] = "";
        if (line[0] == '#' || sscanf(line, "%31s", cmd) != 1) continue;
        if (!strcmp(cmd, "wait") && sscanf(line, "%*s %d", &a) == 1) advance(a);
        else if (!strcmp(cmd, "seed") && sscanf(line, "%*s %d", &a) == 1) sv.rng = (uint32_t)a | 1u;
        else if (!strcmp(cmd, "pad") && sscanf(line, "%*s %d", &a) == 1)
        {
            game->pad((uint8_t)a, true);
            advance(3);
            game->pad((uint8_t)a, false);
            advance(1);
        }
        else if (!strcmp(cmd, "paddown") && sscanf(line, "%*s %d", &a) == 1) game->pad((uint8_t)a, true);
        else if (!strcmp(cmd, "padup") && sscanf(line, "%*s %d", &a) == 1) game->pad((uint8_t)a, false);
        else if (!strcmp(cmd, "row") && sscanf(line, "%*s %d", &a) == 1)
        {
            game->pad((uint8_t)(a * 12 + 5), true);
            advance(3);
            game->pad((uint8_t)(a * 12 + 5), false);
            advance(1);
        }
        else if (!strcmp(cmd, "play"))
        {
            game->button(Button::Play, true);
            advance(2);
            game->button(Button::Play, false);
            advance(1);
        }
        else if (!strcmp(cmd, "screen") && sscanf(line, "%*s %d", &a) == 1)
        {
            game->button((Button)((int)Button::Screen0 + a), true);
            advance(2);
            game->button((Button)((int)Button::Screen0 + a), false);
            advance(1);
        }
        else if (!strcmp(cmd, "jog") && sscanf(line, "%*s %d", &a) == 1)
        {
            game->jog((int16_t)a);
            advance(1);
        }
        else if (!strcmp(cmd, "jogfor") && sscanf(line, "%*s %d %d", &a, &b) == 2)
        {
            for (int i = 0; i < b; i++)
            {
                game->jog((int16_t)a);
                advance(1);
            }
        }
        else if (!strcmp(cmd, "shot") && sscanf(line, "%*s %63s", name) == 1) shot(name);
        else if (!strcmp(cmd, "quizround") && sscanf(line, "%*s %d", &a) == 1) quiz.debugSkipToRound(a);
        else if (!strcmp(cmd, "quizuntil") && sscanf(line, "%*s %d %d", &a, &b) == 2)
        {
            for (int i = 0; i < b && quiz.debugPhase() != a; i++) advance(1);
        }
        else if (!strcmp(cmd, "medalstate")) medal.debugPrint();
        else if (!strcmp(cmd, "medalnobonus")) medal.debugNoBonus = true;
        else if (!strcmp(cmd, "medalforce") && sscanf(line, "%*s %d", &a) == 1)
        {
            medal.debugForceOut = (int8_t)a;
            medal.debugQueueSpin();
        }
        else fprintf(stderr, "unknown command: %s", line);
    }
    fclose(sc);
    int bad = 0;
    for (int l = 0; l < kLayerCount; l++)
    {
        printf("layer %d worst: display list %zu B (limit 8192), commands %zu B (limit 4096)\n", l, worst[l], worstCmd[l]);
        bad |= worst[l] > 8000 || worstCmd[l] > 3900;
    }
    printf("frames %u  sfx: %.120s\n", frames, sv.sfxLog.c_str());
    return bad;
}
