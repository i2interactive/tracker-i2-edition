// i2 DECK host harness: runs the firmware's DeckApp on a PC with scripted
// input. It records every layer's drawing commands (render.py turns them into
// PNGs of the 800x480 screen), logs everything the deck would send to the
// computer or the radio, and checks each layer against the display chip's
// limits (4 KB of commands per redraw, 8 KB display-list slot).
//
// Build + run: see run.sh.   Usage: host <sdroot> <outdir> <script>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <stdarg.h>
#include <algorithm>
#include <string>
#include <vector>

#include "../../../core/interface/i2Deck/i2DeckApp.h"
#include "../../../core/interface/i2Deck/i2DotFont.h"
#include "font_metrics.h"

using namespace i2arc;
using namespace i2deck;

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
    size_t cmd = 0;

    void reset()
    {
        out.clear();
        dl = 16; // save/restore context, alpha, line width
        cmd = 16;
    }
    void add(const char* s, size_t d, size_t c)
    {
        out += s;
        dl += d;
        cmd += c;
    }
    void color(uint32_t rgb) override
    {
        char b[32];
        snprintf(b, sizeof(b), "C %06x\n", rgb & 0xFFFFFF);
        add(b, 4, 4);
    }
    void alpha(uint8_t a) override
    {
        char b[16];
        snprintf(b, sizeof(b), "A %d\n", a);
        add(b, 4, 4);
    }
    void rect(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint8_t radius) override
    {
        char b[64];
        snprintf(b, sizeof(b), "R %d %d %d %d %d\n", x0, y0, x1, y1, radius);
        add(b, 20, 20);
    }
    void rects(const int16_t* xyxy, int n, uint8_t radius) override
    {
        if (n <= 0) return;
        char b[64];
        for (int i = 0; i < n; i++)
        {
            snprintf(b, sizeof(b), "R %d %d %d %d %d\n", xyxy[4 * i], xyxy[4 * i + 1], xyxy[4 * i + 2], xyxy[4 * i + 3], radius);
            out += b;
        }
        dl += 12 + 8 * n;
        cmd += 12 + 8 * n;
    }
    void circle(int16_t cx, int16_t cy, int16_t r) override
    {
        char b[48];
        snprintf(b, sizeof(b), "O %d %d %d\n", cx, cy, r);
        add(b, 16, 16);
    }
    void points(const int16_t*, int, int16_t) override {}
    void fillBelow(const int16_t*, int) override {}
    void line(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint8_t width) override
    {
        char b[64];
        snprintf(b, sizeof(b), "L %d %d %d %d %d\n", x0, y0, x1, y1, width);
        add(b, 20, 20);
    }
    void text(int16_t x, int16_t y, uint8_t font, uint8_t align, const char* s, int len) override
    {
        if (s == nullptr || *s == 0) return;
        int n = len > 0 ? len : (int)strlen(s);
        char b[64];
        snprintf(b, sizeof(b), "T %d %d %d %d %d|", x, y, font, align, n);
        out += b;
        out.append(s, (size_t)n);
        out += "\n";
        int glyphs = 0;
        for (int i = 0; i < n; i++) glyphs += s[i] != ' ';
        dl += 16 + 8 * glyphs;
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
    void dot(int16_t x, int16_t y, uint8_t size, uint8_t align, const char* s, int len) override
    {
        if (s == nullptr || *s == 0) return;
        int n = len > 0 ? len : (int)strlen(s);
        char b[64];
        snprintf(b, sizeof(b), "D %d %d %d %d %d|", x, y, size, align, n);
        out += b;
        out.append(s, (size_t)n);
        out += "\n";
        int glyphs = 0;
        for (int i = 0; i < n; i++) glyphs += s[i] != ' ';
        dl += 16 + 8 * glyphs;
        cmd += 12 + ((n + 4) & ~3);
    }
    void clip(int16_t x, int16_t y, int16_t w, int16_t h) override
    {
        char b[64];
        snprintf(b, sizeof(b), "S %d %d %d %d\n", x, y, w, h);
        add(b, 8, 8);
    }
    void unclip() override { add("U\n", 8, 8); }
};

class HostServices : public Services
{
  public:
    std::string root;
    uint32_t rng = 0x12345678;
    std::vector<uint8_t> scr = std::vector<uint8_t>(768 * 1024);
    uint8_t pads[48] = {};

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
        std::string dir = p.substr(0, p.rfind('/'));
        std::string mk = "mkdir -p '" + dir + "'";
        if (system(mk.c_str()) != 0) return false;
        FILE* f = fopen(p.c_str(), "wb");
        if (!f) return false;
        fwrite(data, 1, len, f);
        fclose(f);
        return true;
    }
    void sfx(Sfx) override {}
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

// Records what the deck sends, and checks that every key that goes down
// comes back up.
class HostIo : public DeckIo
{
  public:
    std::string log;
    int down[256] = {};
    int stuck = 0;
    bool connected = true;
    bool power = false;
    float freq = 0;
    int sig = 31;
    std::string rds = "";
    int left = -1;

    void say(const char* fmt, ...) __attribute__((format(printf, 2, 3)))
    {
        char b[96];
        va_list ap;
        va_start(ap, fmt);
        vsnprintf(b, sizeof(b), fmt, ap);
        va_end(ap);
        log += b;
        log += "\n";
    }
    bool keysConnected() override { return connected; }
    void keyDown(uint8_t key, uint8_t mods) override
    {
        down[key]++;
        say("key down %02x mods %x", key, mods);
    }
    void keyUp(uint8_t key) override
    {
        if (down[key] <= 0) stuck++;
        else down[key]--;
        say("key up   %02x", key);
    }
    void keysOff() override
    {
        memset(down, 0, sizeof(down));
        say("keys off");
    }
    void midiNote(uint8_t note, bool on, uint8_t channel) override { say("note %s %d ch %d", on ? "on " : "off", note, channel); }
    void midiCc(uint8_t cc, uint8_t value, uint8_t channel) override { say("cc %d = %d ch %d", cc, value, channel); }
    void radioPower(bool on) override
    {
        power = on;
        say("radio %s", on ? "on" : "off");
    }
    void radioTune(float mhz) override
    {
        freq = mhz;
        say("tune %.1f", mhz);
    }
    void radioSeek(bool up) override
    {
        freq += up ? 2.3f : -2.3f;
        say("seek %s -> %.1f", up ? "up" : "down", freq);
    }
    void radioPoll() override {}
    float radioFreq() override { return freq; }
    int radioSignal() override { return sig; }
    float radioStep() override { return 0.1f; }
    const char* radioText() override { return rds.c_str(); }
    void leave(uint8_t where) override
    {
        left = where;
        say("leave -> %s", where ? "recorder" : "tracker");
    }
    int heldKeys() const
    {
        int n = 0;
        for (int d : down) n += d;
        return n;
    }
};

static DeckApp app;

static bool nameToButton(const char* n, Button& b)
{
    struct
    {
        const char* name;
        Button b;
    } k[] = { { "play", Button::Play },     { "left", Button::Left },   { "right", Button::Right },
              { "up", Button::Up },         { "down", Button::Down },   { "delete", Button::Delete },
              { "copy", Button::Copy },     { "insert", Button::Insert } };
    for (auto& e : k)
    {
        if (!strcmp(n, e.name))
        {
            b = e.b;
            return true;
        }
    }
    return false;
}

int main(int argc, char** argv)
{
    if (argc < 4)
    {
        fprintf(stderr, "usage: host <sdroot> <outdir> <script>\n");
        return 2;
    }
    HostServices sv;
    HostIo io;
    sv.root = argv[1];
    std::string outDir = argv[2];
    FILE* sc = fopen(argv[3], "r");
    if (!sc)
    {
        fprintf(stderr, "no script\n");
        return 2;
    }

    // 1. Every built-in deck must parse completely: no unknown key names.
    int bad = 0;
    {
        static Deck d;
        for (uint8_t i = 0; i < kBuiltinDecks; i++)
        {
            int keys = parseDeck(kBuiltin[i].text, strlen(kBuiltin[i].text), d);
            int empty = 0, dials = 0;
            for (uint8_t p = 0; p < d.pageCount; p++)
            {
                for (uint8_t k = 0; k < kPads; k++)
                {
                    const PadDef& pd = d.pages[p].pads[k];
                    if (!pd.used()) continue;
                    if (d.link == kLinkKeys && (pd.mac.isEmpty() || pd.win.isEmpty()))
                    {
                        printf("  !! %s page %d pad %d (%s) has no shortcut\n", d.name, p, k, pd.mn);
                        empty++;
                    }
                    if (d.link == kLinkRadio && pd.act == kActNone)
                    {
                        printf("  !! %s pad %d (%s) has no action\n", d.name, k, pd.mn);
                        empty++;
                    }
                }
                for (uint8_t k = 0; k < kDials; k++)
                {
                    const DialDef& dd = d.pages[p].dials[k];
                    if (!dd.used()) continue;
                    dials++;
                    if (d.link == kLinkKeys && (dd.cwMac.isEmpty() || dd.ccwMac.isEmpty() || dd.cwWin.isEmpty() || dd.ccwWin.isEmpty()))
                    {
                        printf("  !! %s page %d dial %d (%s) has no shortcut\n", d.name, p, k + 1, dd.name);
                        empty++;
                    }
                }
            }
            printf("deck %-7s pages %d keys %3d dials %2d problems %d\n", d.name, d.pageCount, keys, dials, empty);
            bad |= empty > 0 || keys == 0;
        }
    }

    // 2. The dot-matrix fonts: build each size as the firmware does.
    {
        std::vector<uint8_t> buf(i2dot::kMaxFontBytes);
        for (uint8_t s = 0; s < i2dot::kSizes; s++)
        {
            uint32_t n = i2dot::buildFont(s, buf.data());
            uint32_t next = s + 1 < i2dot::kSizes ? i2dot::kAddress[s + 1] : 1024u * 1024u;
            bool fits = i2dot::kAddress[s] + n <= next && n <= i2dot::kMaxFontBytes;
            printf("dot font %d: %2dx%2d px, %5u bytes at %u %s\n", s, i2dot::cellW(s), i2dot::cellH(s), n, i2dot::kAddress[s], fits ? "" : "!! OVERLAPS");
            bad |= !fits;
        }
    }

    app.setIo(&io);
    app.begin(sv);
    uint32_t now = 1000;
    uint32_t frames = 0;
    size_t worst[kLayerCount] = {};
    size_t worstCmd[kLayerCount] = {};
    size_t current[kLayerCount] = {};
    size_t worstFrame = 0;
    RecGfx g;
    auto frameCheck = [&]() {
        size_t sum = 0;
        for (size_t c : current) sum += c;
        if (sum > worstFrame) worstFrame = sum;
    };

    auto advance = [&](int n) {
        for (int i = 0; i < n; i++)
        {
            now += 17;
            frames++;
            app.tick(now);
            for (uint8_t layer = 0; layer < kLayerCount; layer++)
            {
                if (!app.dirty(layer)) continue;
                g.reset();
                app.draw(g, layer);
                current[layer] = g.dl;
                if (g.dl > worst[layer]) worst[layer] = g.dl;
                if (g.cmd > worstCmd[layer]) worstCmd[layer] = g.cmd;
            }
            frameCheck();
        }
    };

    auto shot = [&](const char* name) {
        std::string path = outDir + "/" + name + ".txt";
        FILE* f = fopen(path.c_str(), "w");
        for (uint8_t layer = 0; layer < kLayerCount; layer++)
        {
            g.reset();
            app.draw(g, layer);
            if (g.dl > worst[layer]) worst[layer] = g.dl;
            if (g.cmd > worstCmd[layer]) worstCmd[layer] = g.cmd;
            current[layer] = g.dl;
            fprintf(f, "# layer %d dl=%zu cmd=%zu\n", layer, g.dl, g.cmd);
            fputs(g.out.c_str(), f);
        }
        fprintf(f, "P ");
        for (int p = 0; p < 48; p++) fprintf(f, "%d", sv.pads[p]);
        fprintf(f, "\n");
        fclose(f);
        frameCheck();
        size_t sum = 0;
        for (size_t c : current) sum += c;
        printf("shot %-18s frame %zu B\n", name, sum);
    };

    char line[256];
    while (fgets(line, sizeof(line), sc))
    {
        char cmd[32] = "";
        char name[64] = "";
        int a = 0, b = 0;
        Button btn;
        if (line[0] == '#' || sscanf(line, "%31s", cmd) != 1) continue;
        if (!strcmp(cmd, "wait") && sscanf(line, "%*s %d", &a) == 1) advance(a);
        else if (!strcmp(cmd, "pad") && sscanf(line, "%*s %d", &a) == 1)
        {
            app.pad((uint8_t)a, true);
            advance(3);
            app.pad((uint8_t)a, false);
            advance(1);
        }
        else if (!strcmp(cmd, "paddown") && sscanf(line, "%*s %d", &a) == 1)
        {
            app.pad((uint8_t)a, true);
            advance(1);
        }
        else if (!strcmp(cmd, "padup") && sscanf(line, "%*s %d", &a) == 1)
        {
            app.pad((uint8_t)a, false);
            advance(1);
        }
        else if (!strcmp(cmd, "screen") && sscanf(line, "%*s %d", &a) == 1)
        {
            // screen buttons are numbered 1-8, as printed on the dial strip
            Button sb = (Button)((int)Button::Screen0 + a - 1);
            app.button(sb, true);
            advance(2);
            app.button(sb, false);
            advance(1);
        }
        else if (!strcmp(cmd, "shift") && sscanf(line, "%*s %d", &a) == 1) app.button(Button::Shift, a != 0);
        else if (!strcmp(cmd, "btn") && sscanf(line, "%*s %63s", name) == 1 && nameToButton(name, btn))
        {
            app.button(btn, true);
            advance(2);
            app.button(btn, false);
            advance(1);
        }
        else if (!strcmp(cmd, "jog") && sscanf(line, "%*s %d", &a) == 1)
        {
            app.jog((int16_t)a);
            advance(1);
        }
        else if (!strcmp(cmd, "jogfor") && sscanf(line, "%*s %d %d", &a, &b) == 2)
        {
            for (int i = 0; i < b; i++)
            {
                app.jog((int16_t)a);
                advance(1);
            }
        }
        else if (!strcmp(cmd, "signal") && sscanf(line, "%*s %d", &a) == 1) io.sig = a;
        else if (!strcmp(cmd, "rds"))
        {
            const char* t = strchr(line, ' ');
            io.rds = t ? t + 1 : "";
            while (!io.rds.empty() && (io.rds.back() == '\n' || io.rds.back() == '\r')) io.rds.pop_back();
        }
        else if (!strcmp(cmd, "usb") && sscanf(line, "%*s %d", &a) == 1) io.connected = a != 0;
        else if (!strcmp(cmd, "allpads"))
        {
            for (int p = 0; p < 48; p++)
            {
                app.pad((uint8_t)p, true);
                advance(1);
                app.pad((uint8_t)p, false);
                advance(1);
            }
        }
        else if (!strcmp(cmd, "expectheld") && sscanf(line, "%*s %d", &a) == 1)
        {
            if (io.heldKeys() != a)
            {
                printf("!! expected %d keys held, have %d\n", a, io.heldKeys());
                bad = 1;
            }
        }
        else if (!strcmp(cmd, "expectleft") && sscanf(line, "%*s %d", &a) == 1)
        {
            if (io.left != a)
            {
                printf("!! expected leave(%d), have %d\n", a, io.left);
                bad = 1;
            }
        }
        else if (!strcmp(cmd, "shot") && sscanf(line, "%*s %63s", name) == 1) shot(name);
        else
        {
            fprintf(stderr, "unknown command: %s", line);
            bad = 1;
        }
    }
    fclose(sc);
    app.end();

    {
        std::string path = outDir + "/io.log";
        FILE* f = fopen(path.c_str(), "w");
        fputs(io.log.c_str(), f);
        fclose(f);
    }
    for (int l = 0; l < kLayerCount; l++)
    {
        printf("layer %d worst: display list %zu B (limit 8192), commands %zu B (limit 3800)\n", l, worst[l], worstCmd[l]);
        bad |= worst[l] > 8000 || worstCmd[l] > 3700;
    }
    // The Tracker builds each frame from every visible control and stops
    // adding controls at 8172 bytes. Stay well under, to leave room for the
    // Tracker's own controls and for error in these estimates.
    printf("whole frame worst: %zu B (budget 6000, hard limit 8172)\n", worstFrame);
    bad |= worstFrame > 6000;
    if (io.stuck)
    {
        printf("!! %d key-up without key-down\n", io.stuck);
        bad = 1;
    }
    if (io.heldKeys())
    {
        printf("!! %d keys still held at exit\n", io.heldKeys());
        bad = 1;
    }
    printf("frames %u, io events %zu, %s\n", frames, (size_t)std::count(io.log.begin(), io.log.end(), '\n'), bad ? "FAILED" : "ok");
    return bad;
}
