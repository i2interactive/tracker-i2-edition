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

#include <math.h>
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
    bool audio = false;
    int volume = -1;
    i2glitch::Params fx;
    int lockedNow = -1;

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
    void midiShort(uint8_t status, uint8_t d1, uint8_t d2) override { say("midi %02x %02x %02x", status, d1, d2); }
    void midiSysex(const uint8_t* data, uint16_t length) override
    {
        std::string t = "sysex";
        char b[8];
        for (uint16_t i = 0; i < length; i++)
        {
            snprintf(b, sizeof(b), " %02x", data[i]);
            t += b;
        }
        log += t + "\n";
    }
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
    void radioAudio(bool on, uint8_t vol) override
    {
        audio = on && vol > 0; // audible
        volume = on ? vol : -1;
        say("radio audio %s vol %d", on ? "on" : "off", vol);
    }
    void glitch(const i2glitch::Params& p) override
    {
        fx = p;
        say("glitch on %d fx %02x stut %u frz %u bits %d hold %d stop %u noise %d cut %d damp %d", p.on, p.fx, (unsigned)p.stutterLen,
            (unsigned)p.freezeLen, p.crushBits, p.crushHold, (unsigned)p.stopStep, p.noise, p.cutoff, p.damp);
    }
    float audioLevel() override { return 0.35f; }
    float tempo() override { return 120.0f; }
    void leave(uint8_t where) override
    {
        left = where;
        say("leave -> %s", where ? "recorder" : "tracker");
    }
    void lockChanged(bool on) override
    {
        lockedNow = on;
        say("lock %s", on ? "on" : "off");
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
              { "copy", Button::Copy },     { "insert", Button::Insert }, { "rec", Button::Rec },
              { "note", Button::Deck0 },    { "instr", Button::Deck1 },  { "fx1", Button::Deck2 },
              { "fx2", Button::Deck3 },     { "master", Button::Deck4 }, { "file", Button::Deck5 },
              { "config", Button::Deck6 } };
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
        for (uint8_t i = 0; i < kAllDecks; i++)
        {
            int keys = parseDeck(kAll[i]->text, strlen(kAll[i]->text), d);
            int empty = 0, dials = 0;
            for (uint8_t p = 0; p < d.pageCount; p++)
            {
                for (uint8_t k = 0; k < kPads; k++)
                {
                    const PadDef& pd = d.pages[p].pads[k];
                    if (!pd.used()) continue;
                    bool keyless = pd.mac.isEmpty() || pd.win.isEmpty();
                    if ((d.link == kLinkKeys && keyless) || (d.link == kLinkMcu && pd.act == kActNone && keyless))
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
                    bool keyless = dd.cwMac.isEmpty() || dd.ccwMac.isEmpty() || dd.cwWin.isEmpty() || dd.ccwWin.isEmpty();
                    if ((d.link == kLinkKeys && keyless) || (d.link == kLinkMcu && dd.act == kActNone && keyless))
                    {
                        printf("  !! %s page %d dial %d (%s) has no shortcut\n", d.name, p, k + 1, dd.name);
                        empty++;
                    }
                }
            }
            for (uint8_t p = 0; p < d.pageCount && kAll[i]->alt; p++)
            {
                if (d.pages[p].dials[5].used())
                {
                    printf("  !! %s page %d: dial 6 is the app switch in this deck\n", d.name, p);
                    empty++;
                }
            }
            printf("deck %-7s %-9s rev %d pages %d keys %3d dials %2d problems %d\n", d.name, kAll[i]->app, d.rev, d.pageCount, keys, dials,
                   empty);
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

    // 3. The glitch processor: every effect on a test tone stays in range,
    //    and STUTTER really repeats.
    {
        static int16_t hist[131072];
        i2glitch::Core core;
        core.init(hist, 131072);
        int16_t l[128], r[128], out[128];
        uint32_t t = 0;
        auto tone = [&]() {
            for (int i = 0; i < 128; i++, t++)
            {
                l[i] = (int16_t)(12000 * ((t / 50) % 2 ? 1 : -1) + (int)(t % 997) * 8);
                r[i] = l[i];
            }
        };
        i2glitch::Params p;
        p.on = 1;
        const uint8_t sets[] = { 0, i2glitch::kStutter, i2glitch::kCrush, i2glitch::kStop, i2glitch::kNoise, i2glitch::kFilter, i2glitch::kFreeze,
                                 (uint8_t)(i2glitch::kStutter | i2glitch::kCrush | i2glitch::kFilter | i2glitch::kNoise), 0 };
        int problems = 0;
        for (uint8_t fxs : sets)
        {
            p.fx = fxs;
            p.stutterLen = 5512;
            p.crushBits = 4;
            p.crushHold = 8;
            p.stopStep = 300;
            p.noise = 20000;
            p.cutoff = 3000;
            p.damp = 6000;
            core.set(p);
            int64_t energy = 0;
            std::vector<int16_t> rec;
            for (int b = 0; b < 400; b++)
            {
                tone();
                if (core.idle())
                {
                    core.feed(l, r, 128);
                    memcpy(out, l, sizeof(out));
                }
                else core.process(l, r, out, 128);
                for (int i = 0; i < 128; i++)
                {
                    energy += (int64_t)out[i] * out[i];
                    rec.push_back(out[i]);
                }
            }
            bool repeats = true;
            if (fxs == i2glitch::kStutter)
            {
                // well after the start, the output is periodic in stutterLen
                for (size_t i = 20000; i + 5512 < rec.size() && repeats; i += 101)
                {
                    if ((i % 5512) < 70 || (i % 5512) > 5512 - 70) continue;
                    repeats = rec[i] == rec[i + 5512];
                }
            }
            double rms = sqrt((double)energy / (double)rec.size());
            bool ok = rms > 50.0 && rms < 33000.0 && repeats;
            if (fxs == i2glitch::kStop) ok = repeats; // ends in silence
            printf("glitch fx %02x rms %7.1f %s\n", fxs, rms, ok ? "" : "!! PROBLEM");
            problems += !ok;
        }
        bad |= problems > 0;
    }

    // 4. Mackie Control encoding
    {
        int problems = 0;
        auto check = [&](bool ok, const char* what) {
            if (!ok)
            {
                printf("  !! mcu: %s\n", what);
                problems++;
            }
        };
        check(i2mcu::relative(1) == 0x01 && i2mcu::relative(5) == 0x05 && i2mcu::relative(63) == 0x3F && i2mcu::relative(99) == 0x3F, "clockwise ticks");
        check(i2mcu::relative(-1) == 0x41 && i2mcu::relative(-5) == 0x45 && i2mcu::relative(-99) == 0x7F, "anticlockwise ticks");
        uint8_t m[3];
        i2mcu::fader(16383, 0, m);
        check(m[0] == 0xE0 && m[1] == 0x7F && m[2] == 0x7F, "fader top, strip 1");
        i2mcu::fader(8192, 2, m);
        check(m[0] == 0xE2 && m[1] == 0x00 && m[2] == 0x40, "fader middle, strip 3");
        i2mcu::fader(0, 8, m);
        check(m[0] == 0xE8 && m[1] == 0 && m[2] == 0, "master fader bottom");
        const uint8_t query[] = { 0xF0, 0x00, 0x00, 0x66, 0x14, 0x00, 0xF7 };
        uint8_t out[20];
        uint16_t n = i2mcu::reply(query, sizeof(query), out);
        check(n == 18 && out[5] == 0x01 && out[4] == 0x14 && out[17] == 0xF7, "device query reply");
        const uint8_t other[] = { 0xF0, 0x00, 0x00, 0x66, 0x05, 0x00, 0xF7 };
        check(i2mcu::reply(other, sizeof(other), out) == 0, "ignores other models");
        const uint8_t conn[] = { 0xF0, 0x00, 0x00, 0x66, 0x14, 0x02, 'I', '2', 'D', 'E', 'C', 'K', '1', 1, 2, 3, 4, 0xF7 };
        n = i2mcu::reply(conn, sizeof(conn), out);
        check(n == 14 && out[5] == 0x03, "connection confirmation");
        // deck file names: @MUTE3 = note 18, @SEL8 = 31, @N100 = 100
        static Deck d;
        const char t[] = "DECK T\nLINK MCU\nK A A | @MUTE3\nK B B | @SEL8\nK C C | @N100\nK D D | @REC9\n";
        parseDeck(t, strlen(t), d);
        check(d.link == kLinkMcu, "LINK MCU");
        check(d.pages[0].pads[0].act == kActMcu && d.pages[0].pads[0].arg == 18, "@MUTE3");
        check(d.pages[0].pads[1].arg == 31 && d.pages[0].pads[2].arg == 100, "@SEL8, @N100");
        check(d.pages[0].pads[3].act == kActNone, "@REC9 is not a strip");
        printf("mcu encoding: %d problems\n", problems);
        bad |= problems > 0;
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
    size_t mark = 0;
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
        else if (!strcmp(cmd, "rec") && sscanf(line, "%*s %d", &a) == 1) app.button(Button::Rec, a != 0);
        else if (!strcmp(cmd, "expectdeck") && sscanf(line, "%*s %d", &a) == 1)
        {
            if (app.openDeckIndex() != a)
            {
                printf("!! expected deck %d, have %d\n", a, app.openDeckIndex());
                bad = 1;
            }
        }
        else if (!strcmp(cmd, "expectlocked") && sscanf(line, "%*s %d", &a) == 1)
        {
            if (io.lockedNow != a || (int)app.locked() != a)
            {
                printf("!! expected locked=%d, have %d / %d\n", a, io.lockedNow, (int)app.locked());
                bad = 1;
            }
        }
        else if (!strcmp(cmd, "expectfx") && sscanf(line, "%*s %x", &a) == 1)
        {
            if (io.fx.fx != a)
            {
                printf("!! expected glitch fx %02x, have %02x\n", a, io.fx.fx);
                bad = 1;
            }
        }
        else if (!strcmp(cmd, "expectaudio") && sscanf(line, "%*s %d", &a) == 1)
        {
            if ((int)io.audio != a)
            {
                printf("!! expected radio audio %d, have %d\n", a, (int)io.audio);
                bad = 1;
            }
        }
        else if (!strcmp(cmd, "settings"))
        {
            // write a settings file, e.g.: settings lock=off
            const char* t = strchr(line, ' ');
            std::string v = t ? t + 1 : "";
            for (auto& c : v) if (c == ' ') c = '\n';
            sv.saveText("i2/Deck/settings.txt", v.c_str(), v.size());
        }
        else if (!strcmp(cmd, "restart"))
        {
            app.end();
            app.begin(sv);
            advance(2);
        }
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
        else if (!strcmp(cmd, "midi"))
        {
            // MIDI from the host: midi 90 10 7f (hex)
            unsigned st = 0, d1 = 0, d2 = 0;
            if (sscanf(line, "%*s %x %x %x", &st, &d1, &d2) >= 2) app.midiIn((uint8_t)st, (uint8_t)d1, (uint8_t)d2);
            advance(1);
        }
        else if (!strcmp(cmd, "sysex"))
        {
            // sysex f0 00 00 66 14 12 00 41 42 f7 (hex), or with "text" after the offset
            std::vector<uint8_t> bytes;
            const char* q = line + 5;
            while (*q)
            {
                while (*q == ' ') q++;
                if (*q == '"')
                {
                    q++;
                    while (*q && *q != '"') bytes.push_back((uint8_t)*q++);
                    if (*q) q++;
                    continue;
                }
                char* e2 = nullptr;
                long v = strtol(q, &e2, 16);
                if (e2 == q) break;
                bytes.push_back((uint8_t)v);
                q = e2;
            }
            app.sysexIn(bytes.data(), (uint16_t)bytes.size());
            advance(1);
        }
        else if (!strcmp(cmd, "mark")) mark = io.log.size();
        else if (!strcmp(cmd, "expectsent"))
        {
            // the log since the last "mark" contains this text
            std::string t = strchr(line, ' ') ? strchr(line, ' ') + 1 : "";
            while (!t.empty() && (t.back() == '\n' || t.back() == '\r')) t.pop_back();
            if (io.log.find(t, mark) == std::string::npos)
            {
                printf("!! expected to send \"%s\"\n", t.c_str());
                bad = 1;
            }
        }
        else if (!strcmp(cmd, "expectnotsent"))
        {
            std::string t = strchr(line, ' ') ? strchr(line, ' ') + 1 : "";
            while (!t.empty() && (t.back() == '\n' || t.back() == '\r')) t.pop_back();
            if (io.log.find(t, mark) != std::string::npos)
            {
                printf("!! did not expect \"%s\"\n", t.c_str());
                bad = 1;
            }
        }
        else if (!strcmp(cmd, "expectfader") && sscanf(line, "%*s %d %d", &a, &b) == 2)
        {
            if (app.faderPos((uint8_t)a) != b)
            {
                printf("!! expected fader %d at %d, have %d\n", a, b, app.faderPos((uint8_t)a));
                bad = 1;
            }
        }
        else if (!strcmp(cmd, "expectled") && sscanf(line, "%*s %d %d", &a, &b) == 2)
        {
            if ((int)app.mcuLed((uint8_t)a) != b)
            {
                printf("!! expected led %d = %d\n", a, b);
                bad = 1;
            }
        }
        else if (!strcmp(cmd, "expecthome") && sscanf(line, "%*s %d", &a) == 1)
        {
            if (app.openDeckIndex() != -1 || app.homePageIndex() != a)
            {
                printf("!! expected home page %d, have deck %d page %d\n", a, app.openDeckIndex(), app.homePageIndex());
                bad = 1;
            }
        }
        else if (!strcmp(cmd, "expectfile"))
        {
            char fn[64] = "";
            if (sscanf(line, "%*s %63s", fn) == 1)
            {
                std::string pth = sv.root + "/i2/Deck/" + fn;
                FILE* f = fopen(pth.c_str(), "rb");
                if (!f)
                {
                    printf("!! expected %s on the card\n", fn);
                    bad = 1;
                }
                else fclose(f);
            }
        }
        else if (!strcmp(cmd, "expectcard"))
        {
            // expectcard settings.txt app=resolve: the file on the card has this text
            char fn[64] = "", want[96] = "";
            if (sscanf(line, "%*s %63s %95s", fn, want) == 2)
            {
                char buf[4096];
                std::string pth = std::string("i2/Deck/") + fn;
                int32_t n = sv.loadText(pth.c_str(), buf, sizeof(buf) - 1);
                buf[n > 0 ? n : 0] = 0;
                if (n <= 0 || !strstr(buf, want))
                {
                    printf("!! expected \"%s\" in %s\n", want, fn);
                    bad = 1;
                }
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
