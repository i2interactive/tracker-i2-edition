#include "i2DeckApp.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

namespace i2deck
{
using namespace i2arc;

namespace
{
constexpr uint32_t kInk = 0xEDEDE6, kGrey = 0xA3A39C, kTile = 0x17171A, kTileOff = 0x101012;
constexpr uint32_t kDark = 0x0B0B0C, kLine = 0x2E2E33, kTrack = 0x1E1E22, kHomeAccent = 0xFF5A3C;

constexpr size_t kDeckOffset = 0;
constexpr size_t kTextOffset = 16 * 1024;
constexpr size_t kTextCap = 16 * 1024;

const char* const kSettingsPath = "i2/Deck/settings.txt";

// HID usages for the Tracker's own buttons
constexpr uint8_t kKeySpace = 0x2C, kKeyBksp = 0x2A, kKeyRight = 0x4F, kKeyLeft = 0x50, kKeyDown = 0x51, kKeyUp = 0x52;
constexpr uint8_t kKeyC = 0x06, kKeyV = 0x19;

bool isSpace(char c)
{
    return c == ' ' || c == '\t' || c == '\r';
}

void trim(const char*& s, const char*& e)
{
    while (s < e && isSpace(*s)) s++;
    while (e > s && isSpace(*(e - 1))) e--;
}

char up(char c)
{
    return (c >= 'a' && c <= 'z') ? (char)(c - 'a' + 'A') : c;
}

void copyUpper(char* dst, size_t cap, const char* s, const char* e)
{
    size_t n = (size_t)(e - s);
    if (n >= cap) n = cap - 1;
    for (size_t i = 0; i < n; i++) dst[i] = up(s[i]);
    dst[n] = 0;
}

void copyText(char* dst, size_t cap, const char* s, const char* e)
{
    size_t n = (size_t)(e - s);
    if (n >= cap) n = cap - 1;
    memcpy(dst, s, n);
    dst[n] = 0;
}

bool startsWord(const char* s, const char* e, const char* word)
{
    size_t n = strlen(word);
    if ((size_t)(e - s) < n) return false;
    for (size_t i = 0; i < n; i++)
    {
        if (up(s[i]) != word[i]) return false;
    }
    return (size_t)(e - s) == n || isSpace(s[n]);
}

const char* findBar(const char* s, const char* e)
{
    for (; s < e; s++)
    {
        if (*s == '|') return s;
    }
    return nullptr;
}

struct ActName
{
    const char* name;
    uint8_t act, arg;
};
const ActName kActs[] = {
    { "@SEEK+", kActSeekUp, 0 },   { "@SEEK-", kActSeekDown, 0 }, { "@STEP+", kActStepUp, 0 },
    { "@STEP-", kActStepDown, 0 }, { "@P1", kActPreset, 0 },      { "@P2", kActPreset, 1 },
    { "@P3", kActPreset, 2 },      { "@P4", kActPreset, 3 },      { "@P5", kActPreset, 4 },
    { "@P6", kActPreset, 5 },      { "@P7", kActPreset, 6 },      { "@P8", kActPreset, 7 },
    { "@SEND", kActSend, 0 },      { "@RECORDER", kActRecorder, 0 }, { "@TUNE", kActTune, 0 },
};

bool parseAct(const char* s, const char* e, uint8_t& act, uint8_t& arg)
{
    for (const ActName& a : kActs)
    {
        size_t n = strlen(a.name);
        if ((size_t)(e - s) != n) continue;
        bool same = true;
        for (size_t i = 0; i < n; i++) same = same && up(s[i]) == a.name[i];
        if (same)
        {
            act = a.act;
            arg = a.arg;
            return true;
        }
    }
    return false;
}

// Splits "a | b | c" into up to `max` trimmed fields.
int splitBars(const char* s, const char* e, const char** fs, const char** fe, int max)
{
    int n = 0;
    while (n < max)
    {
        const char* bar = findBar(s, e);
        fs[n] = s;
        fe[n] = bar ? bar : e;
        trim(fs[n], fe[n]);
        n++;
        if (!bar) break;
        s = bar + 1;
    }
    return n;
}
} // namespace

//==============================================================================================================
// Deck files
//==============================================================================================================
int parseDeck(const char* text, size_t length, Deck& out)
{
    memset(&out, 0, sizeof(out));
    strcpy(out.name, "DECK");
    out.accent = kInk;

    int accepted = 0;
    int pg = -1, block = 0, slot = 0;
    const char* p = text;
    const char* end = text + length;

    auto needPage = [&]() -> Page* {
        if (pg < 0)
        {
            pg = 0;
            out.pageCount = 1;
            strcpy(out.pages[0].name, "MAIN");
        }
        return pg < kMaxPages ? &out.pages[pg] : nullptr;
    };

    while (p < end)
    {
        const char* le = p;
        while (le < end && *le != '\n') le++;
        const char* s = p;
        const char* e = le;
        trim(s, e);
        p = le < end ? le + 1 : end;
        if (s >= e || *s == '#') continue;

        if (startsWord(s, e, "DECK"))
        {
            const char* v = s + 4;
            trim(v, e);
            copyUpper(out.name, sizeof(out.name), v, e);
        }
        else if (startsWord(s, e, "APP"))
        {
            const char* v = s + 3;
            trim(v, e);
            copyText(out.app, sizeof(out.app), v, e);
        }
        else if (startsWord(s, e, "LINK"))
        {
            const char* v = s + 4;
            trim(v, e);
            out.link = startsWord(v, e, "MIDI") ? kLinkMidi : startsWord(v, e, "RADIO") ? kLinkRadio : kLinkKeys;
        }
        else if (startsWord(s, e, "COLOR"))
        {
            char hex[8];
            const char* v = s + 5;
            trim(v, e);
            copyText(hex, sizeof(hex), v, e);
            out.accent = (uint32_t)strtoul(hex, nullptr, 16) & 0xFFFFFF;
        }
        else if (startsWord(s, e, "PAGE"))
        {
            if (out.pageCount >= kMaxPages)
            {
                pg = kMaxPages; // ignore extra pages
                continue;
            }
            pg = out.pageCount++;
            const char* v = s + 4;
            trim(v, e);
            copyUpper(out.pages[pg].name, sizeof(out.pages[pg].name), v, e);
            block = 0;
            slot = 0;
        }
        else if (startsWord(s, e, "BLOCK"))
        {
            Page* page = needPage();
            const char* v = s + 5;
            trim(v, e);
            if (!page || v >= e || *v < '1' || *v > '3') continue;
            block = *v - '1';
            slot = 0;
            v++;
            trim(v, e);
            copyUpper(page->block[block], sizeof(page->block[block]), v, e);
        }
        else if (startsWord(s, e, "K"))
        {
            Page* page = needPage();
            if (!page || slot >= 16) continue;
            int padIndex = (slot / 4) * 12 + block * 4 + (slot % 4);
            slot++;
            const char* v = s + 1;
            trim(v, e);
            if (v >= e || (*v == '-' && (v + 1 == e || isSpace(v[1])))) continue; // blank key

            const char* fs[3];
            const char* fe[3];
            int nf = splitBars(v, e, fs, fe, 3);

            // first field: mnemonic then label
            const char* m = fs[0];
            const char* me = m;
            while (me < fe[0] && !isSpace(*me)) me++;
            const char* l = me;
            const char* lEnd = fe[0];
            trim(l, lEnd);

            PadDef& d = page->pads[padIndex];
            memset(&d, 0, sizeof(d));
            copyUpper(d.mn, sizeof(d.mn), m, me);
            copyUpper(d.label, sizeof(d.label), l, lEnd);
            if (nf >= 2 && fs[1] < fe[1])
            {
                if (*fs[1] == '@')
                {
                    parseAct(fs[1], fe[1], d.act, d.arg);
                }
                else
                {
                    i2ctl::parseKeySpec(fs[1], (size_t)(fe[1] - fs[1]), true, d.mac);
                    const char* ws = nf >= 3 ? fs[2] : fs[1];
                    const char* we = nf >= 3 ? fe[2] : fe[1];
                    i2ctl::parseKeySpec(ws, (size_t)(we - ws), false, d.win);
                }
            }
            accepted++;
        }
        else if (startsWord(s, e, "DIAL"))
        {
            Page* page = needPage();
            const char* v = s + 4;
            trim(v, e);
            if (!page || v >= e || *v < '1' || *v > '0' + kDials) continue;
            DialDef& d = page->dials[*v - '1'];
            memset(&d, 0, sizeof(d));
            v++;
            const char* fs[5];
            const char* fe[5];
            int nf = splitBars(v, e, fs, fe, 5);
            copyUpper(d.name, sizeof(d.name), fs[0], fe[0]);
            if (nf >= 2 && fs[1] < fe[1] && *fs[1] == '@')
            {
                uint8_t arg = 0;
                parseAct(fs[1], fe[1], d.act, arg);
            }
            else if (nf >= 3)
            {
                i2ctl::parseKeySpec(fs[1], (size_t)(fe[1] - fs[1]), true, d.cwMac);
                i2ctl::parseKeySpec(fs[2], (size_t)(fe[2] - fs[2]), true, d.ccwMac);
                int w = nf >= 5 ? 3 : 1;
                i2ctl::parseKeySpec(fs[w], (size_t)(fe[w] - fs[w]), false, d.cwWin);
                i2ctl::parseKeySpec(fs[w + 1], (size_t)(fe[w + 1] - fs[w + 1]), false, d.ccwWin);
            }
        }
    }
    return accepted;
}

//==============================================================================================================
// Settings
//==============================================================================================================
bool DeckApp::bootToMusic(Services& services)
{
    char buf[256];
    int32_t n = services.loadText(kSettingsPath, buf, sizeof(buf) - 1);
    if (n <= 0) return false;
    buf[n] = 0;
    return strstr(buf, "boot=music") != nullptr;
}

void DeckApp::loadSettings()
{
    char buf[256];
    int32_t n = sv->loadText(kSettingsPath, buf, sizeof(buf) - 1);
    if (n <= 0) return;
    buf[n] = 0;
    if (strstr(buf, "os=win")) mac = false;
    if (strstr(buf, "os=mac")) mac = true;
    const char* f = strstr(buf, "freq=");
    if (f)
    {
        long v = strtol(f + 5, nullptr, 10);
        if (v >= 875 && v <= 1080) freq = v / 10.0f;
    }
    for (int i = 0; i < 8; i++)
    {
        char key[16];
        snprintf(key, sizeof(key), "p%d=", i + 1);
        const char* q = strstr(buf, key);
        if (q)
        {
            long v = strtol(q + 3, nullptr, 10);
            if (v >= 875 && v <= 1080) presets[i] = v / 10.0f;
        }
    }
}

void DeckApp::saveSettings()
{
    char buf[256];
    int n = snprintf(buf, sizeof(buf), "os=%s\nboot=deck\nfreq=%d\n", mac ? "mac" : "win", (int)(freq * 10 + 0.5f));
    for (int i = 0; i < 8 && n > 0 && n < (int)sizeof(buf) - 12; i++)
    {
        n += snprintf(buf + n, sizeof(buf) - (size_t)n, "p%d=%d\n", i + 1, (int)(presets[i] * 10 + 0.5f));
    }
    if (n > 0) sv->saveText(kSettingsPath, buf, (size_t)n);
}

//==============================================================================================================
// Lifecycle and navigation
//==============================================================================================================
void DeckApp::begin(Services& services)
{
    sv = &services;
    size_t size = 0;
    uint8_t* mem = sv->scratch(size);
    deckPtr = reinterpret_cast<Deck*>(mem + kDeckOffset);
    home = true;
    deckIndex = -1;
    held = 0;
    shift = false;
    last[0] = 0;
    memset(passKey, 0, sizeof(passKey));
    loadSettings();
    dirtyMask = 0x3F;
    lightPads();
}

void DeckApp::end()
{
    releaseAll();
    if (radioOn && io)
    {
        io->radioPower(false);
        radioOn = false;
    }
}

void DeckApp::releaseAll()
{
    if (io) io->keysOff();
    if (io && !home && deckPtr && deck().link == kLinkMidi)
    {
        for (uint8_t p = 0; p < kPads; p++)
        {
            if (held & (1ULL << p)) io->midiNote(p, false, (uint8_t)(pageIndex + 1));
        }
    }
    held = 0;
    memset(passKey, 0, sizeof(passKey));
}

void DeckApp::goHome()
{
    releaseAll();
    if (radioOn && io)
    {
        io->radioPower(false);
        radioOn = false;
    }
    home = true;
    dirtyMask = 0x3F;
    lightPads();
}

void DeckApp::openDeck(uint8_t index)
{
    if (index >= kBuiltinDecks) return;
    releaseAll();
    const BuiltinDeck& b = kBuiltin[index];
    size_t size = 0;
    char* text = reinterpret_cast<char*>(sv->scratch(size) + kTextOffset);

    char path[40];
    snprintf(path, sizeof(path), "i2/Deck/%s", b.file);
    int32_t n = sv->loadText(path, text, kTextCap - 1);
    int keys = 0;
    if (n > 0)
    {
        keys = parseDeck(text, (size_t)n, deck());
    }
    else
    {
        // first run: put the built-in deck on the card so it can be edited
        sv->saveText(path, b.text, strlen(b.text));
    }
    if (keys == 0)
    {
        parseDeck(b.text, strlen(b.text), deck());
    }
    if (deck().pageCount == 0) deck().pageCount = 1;

    home = false;
    deckIndex = (int8_t)index;
    pageIndex = 0;
    dialSel = 0;
    memset(dialNet, 0, sizeof(dialNet));
    last[0] = 0;
    if (deck().link == kLinkRadio && io)
    {
        io->radioPower(true);
        io->radioTune(freq);
        radioOn = true;
        rds[0] = 0;
        memset(history, 0, sizeof(history));
    }
    dirtyMask = 0x3F;
    lightPads();
}

void DeckApp::setPage(uint8_t p)
{
    releaseAll();
    pageIndex = p;
    dialSel = 0;
    memset(dialNet, 0, sizeof(dialNet));
    dirtyMask = 0x3F;
    lightPads();
}

void DeckApp::lightPads()
{
    if (!sv) return;
    for (uint8_t p = 0; p < kPads; p++)
    {
        uint8_t level = 0;
        if (!home && page().pads[p].used())
        {
            level = (held & (1ULL << p)) ? 2 : 1;
        }
        sv->padLight(p, level);
    }
}

void DeckApp::note(const char* text)
{
    strncpy(last, text, sizeof(last) - 1);
    last[sizeof(last) - 1] = 0;
    dirtyMask |= 1u << kLayerLive;
}

void DeckApp::tap(i2ctl::KeyCombo c)
{
    if (!io || c.isEmpty()) return;
    io->keyDown(c.key, c.mods);
    io->keyUp(c.key);
}

//==============================================================================================================
// Input
//==============================================================================================================
void DeckApp::runAction(uint8_t act, uint8_t arg)
{
    if (!io) return;
    switch (act)
    {
        case kActSeekUp:
        case kActSeekDown:
            io->radioSeek(act == kActSeekUp);
            rds[0] = 0;
            break;
        case kActStepUp:
        case kActStepDown:
            freq += act == kActStepUp ? io->radioStep() : -io->radioStep();
            if (freq < 87.5f) freq = 108.0f;
            if (freq > 108.0f) freq = 87.5f;
            io->radioTune(freq);
            rds[0] = 0;
            break;
        case kActPreset:
            if (arg >= 8) break;
            if (shift)
            {
                presets[arg] = freq;
                saveSettings();
                dirtyMask |= 1u << kLayerBlockA | 1u << kLayerBlockB | 1u << kLayerBlockC;
            }
            else
            {
                freq = presets[arg];
                io->radioTune(freq);
                rds[0] = 0;
            }
            break;
        case kActSend:
            sending = !sending;
            lastCc = 255;
            break;
        case kActRecorder:
            saveSettings();
            io->leave(1);
            break;
        default: break;
    }
    dirtyMask |= 1u << kLayerLive;
}

void DeckApp::pad(uint8_t padIndex, bool down)
{
    if (home || padIndex >= kPads || !io) return;
    const PadDef& d = page().pads[padIndex];
    if (!d.used()) return;
    uint8_t blockLayer = (uint8_t)(kLayerBlockA + (padIndex % 12) / 4);

    if (down)
    {
        if (d.act != kActNone)
        {
            runAction(d.act, d.arg);
        }
        else if (deck().link == kLinkKeys)
        {
            i2ctl::KeyCombo c = mac ? d.mac : d.win;
            if (!c.isEmpty())
            {
                if (shift) c.mods |= i2ctl::kModShift;
                io->keyDown(c.key, c.mods);
                heldKey[padIndex] = c.key;
            }
        }
        else if (deck().link == kLinkMidi)
        {
            io->midiNote(padIndex, true, (uint8_t)(pageIndex + 1));
        }
        held |= 1ULL << padIndex;
        note(d.label[0] ? d.label : d.mn);
        sv->padLight(padIndex, 2);
        dirtyMask |= 1u << blockLayer;
    }
    else if (held & (1ULL << padIndex))
    {
        if (d.act == kActNone && deck().link == kLinkKeys)
        {
            io->keyUp(heldKey[padIndex]);
        }
        else if (d.act == kActNone && deck().link == kLinkMidi)
        {
            io->midiNote(padIndex, false, (uint8_t)(pageIndex + 1));
        }
        held &= ~(1ULL << padIndex);
        sv->padLight(padIndex, 1);
        dirtyMask |= 1u << blockLayer;
    }
}

void DeckApp::button(Button b, bool down)
{
    if (b == Button::Shift)
    {
        shift = down;
        return;
    }

    int screen = (int)b - (int)Button::Screen0;
    if (screen >= 0 && screen <= 7)
    {
        if (!down) return;
        if (home)
        {
            if (screen < kBuiltinDecks) openDeck((uint8_t)screen);
            else if (screen == 4 && io) io->leave(0);
            else if (screen == 6)
            {
                mac = !mac;
                saveSettings();
                dirtyMask = 0x3F;
            }
            return;
        }
        if (screen == 7)
        {
            if (shift || deck().pageCount <= 1) goHome();
            else setPage((uint8_t)((pageIndex + 1) % deck().pageCount));
            return;
        }
        if (page().dials[screen].used())
        {
            dialSel = (uint8_t)screen;
            dirtyMask |= 1u << kLayerChrome | 1u << kLayerLive;
        }
        return;
    }

    // The Tracker's own buttons keep their printed meaning in keyboard decks.
    if (home || !io || deck().link != kLinkKeys) return;
    i2ctl::KeyCombo c{};
    uint8_t slot = 0;
    const uint8_t cmd = mac ? i2ctl::kModGui : i2ctl::kModCtrl;
    switch (b)
    {
        case Button::Play: c.key = kKeySpace; slot = 0; break;
        case Button::Left: c.key = kKeyLeft; slot = 1; break;
        case Button::Right: c.key = kKeyRight; slot = 2; break;
        case Button::Up: c.key = kKeyUp; slot = 3; break;
        case Button::Down: c.key = kKeyDown; slot = 4; break;
        case Button::Delete: c.key = kKeyBksp; slot = 5; break;
        case Button::Copy: c.key = kKeyC; c.mods = cmd; slot = 6; break;
        case Button::Insert: c.key = kKeyV; c.mods = cmd; slot = 7; break;
        default: return;
    }
    if (down)
    {
        if (shift) c.mods |= i2ctl::kModShift;
        io->keyDown(c.key, c.mods);
        passKey[slot] = c.key;
    }
    else if (passKey[slot])
    {
        io->keyUp(passKey[slot]);
        passKey[slot] = 0;
    }
}

void DeckApp::jog(int16_t steps)
{
    if (home || !io || steps == 0) return;
    const DialDef& d = page().dials[dialSel];
    if (!d.used()) return;
    int n = steps > 0 ? steps : -steps;
    if (n > 8) n = 8;
    bool cw = steps > 0;

    if (d.act == kActTune)
    {
        freq += (cw ? io->radioStep() : -io->radioStep()) * n * (shift ? 10 : 1);
        if (freq < 87.5f) freq = 87.5f;
        if (freq > 108.0f) freq = 108.0f;
        io->radioTune(freq);
        rds[0] = 0;
    }
    else if (deck().link == kLinkKeys)
    {
        i2ctl::KeyCombo c = mac ? (cw ? d.cwMac : d.ccwMac) : (cw ? d.cwWin : d.ccwWin);
        if (shift) c.mods |= i2ctl::kModShift;
        for (int i = 0; i < n; i++) tap(c);
    }
    else if (deck().link == kLinkMidi)
    {
        // relative control change: 65 = up one, 63 = down one
        for (int i = 0; i < n; i++) io->midiCc((uint8_t)(dialSel + 1), cw ? 65 : 63, (uint8_t)(pageIndex + 1));
    }
    dialNet[dialSel] = (int16_t)(dialNet[dialSel] + (cw ? n : -n));
    dirtyMask |= 1u << kLayerChrome | 1u << kLayerLive;
}

void DeckApp::tick(uint32_t now)
{
    nowMs = now;
    if (!radioOn || !io) return;
    io->radioPoll();
    if (now - lastPoll < 250) return;
    lastPoll = now;

    float f = io->radioFreq();
    int s = io->radioSignal();
    const char* t = io->radioText();
    bool changed = false;
    if (f > 80.0f && (f > freq + 0.04f || f < freq - 0.04f))
    {
        freq = f; // a seek landed
        changed = true;
    }
    signal = s;
    memmove(history, history + 1, kHistory - 1);
    history[kHistory - 1] = (uint8_t)(s < 0 ? 0 : s > 75 ? 75 : s);
    changed = true;
    if (t && strncmp(t, rds, sizeof(rds) - 1) != 0)
    {
        strncpy(rds, t, sizeof(rds) - 1);
        rds[sizeof(rds) - 1] = 0;
        changed = true;
    }
    if (sending)
    {
        int v = signal * 127 / 75;
        if (v > 127) v = 127;
        if (v < 0) v = 0;
        if ((uint8_t)v != lastCc)
        {
            lastCc = (uint8_t)v;
            io->midiCc(20, lastCc, 16);
        }
    }
    if (changed) dirtyMask |= 1u << kLayerLive;
}

//==============================================================================================================
// Drawing
//==============================================================================================================
void DeckApp::draw(Gfx& g, uint8_t layer)
{
    dirtyMask &= (uint8_t)~(1u << layer);
    switch (layer)
    {
        case kLayerBg: drawBg(g); break;
        case kLayerChrome: drawChrome(g); break;
        case kLayerLive:
            if (home) drawHome(g);
            else drawLive(g);
            break;
        default:
            if (!home) drawBlock(g, (uint8_t)(layer - kLayerBlockA));
            break;
    }
}

void DeckApp::drawBg(Gfx& g)
{
    g.color(kDark);
    g.rect(0, 0, 800, 480);
    g.color(kLine);
    g.rect(0, 35, 800, 36);
    g.rect(0, 424, 800, 425);
}

void DeckApp::chip(Gfx& g, int16_t rightX, const char* text, bool solid, uint32_t accent, int16_t& outLeft)
{
    int16_t w = (int16_t)(g.textWidth(kFont12, text) + 14);
    int16_t x0 = (int16_t)(rightX - w);
    g.color(solid ? accent : 0x55555C);
    g.rect(x0, 9, rightX, 27, 3);
    if (!solid)
    {
        g.color(kDark);
        g.rect((int16_t)(x0 + 1), 10, (int16_t)(rightX - 1), 26, 2);
    }
    g.color(solid ? kDark : kInk);
    g.text((int16_t)(x0 + w / 2), 11, kFont12, kCenterX, text);
    outLeft = (int16_t)(x0 - 6);
}

void DeckApp::drawChrome(Gfx& g)
{
    const uint32_t accent = home ? kHomeAccent : deck().accent;
    const char* title = home ? "DECK" : deck().name;
    const char* app = home ? "Tracker i2 edition" : deck().app;

    g.color(accent);
    g.dot(12, 8, 1, kLeft, "i2");
    g.color(kInk);
    g.dot(58, 8, 1, kLeft, title);
    g.color(kGrey);
    g.text((int16_t)(58 + (int16_t)strlen(title) * 18 + 8), 11, kFont14, kLeft, app);

    int16_t x = 788;
    chip(g, x, mac ? "MAC" : "WIN", false, accent, x);
    const char* link = "USB";
    if (!home)
    {
        link = deck().link == kLinkKeys ? ((io && io->keysConnected()) ? "USB KEYS" : "NO USB") : deck().link == kLinkMidi ? "MIDI OUT" : (sending ? "FM + CC" : "FM");
    }
    chip(g, x, link, true, accent, x);

    // dial strip
    for (uint8_t i = 0; i < 8; i++)
    {
        int16_t cx = (int16_t)(i * 100 + 50);
        char nameBuf[16] = "";
        char value[16] = "";
        bool on = false;
        if (home)
        {
            const char* names[8] = { kBuiltin[0].key, kBuiltin[1].key, kBuiltin[2].key, kBuiltin[3].key, "MUSIC", "", mac ? "MAC" : "WIN", "" };
            if (names[i][0] == 0) continue;
            snprintf(nameBuf, sizeof(nameBuf), "BUTTON %d", i + 1);
            snprintf(value, sizeof(value), "%s", names[i]);
        }
        else if (i == 7)
        {
            if (deck().pageCount > 1)
            {
                snprintf(nameBuf, sizeof(nameBuf), "PAGE");
                snprintf(value, sizeof(value), "%d/%d", pageIndex + 1, deck().pageCount);
            }
            else
            {
                snprintf(nameBuf, sizeof(nameBuf), "BACK");
                snprintf(value, sizeof(value), "HOME");
            }
        }
        else
        {
            const DialDef& d = page().dials[i];
            if (!d.used()) continue;
            on = i == dialSel;
            snprintf(nameBuf, sizeof(nameBuf), "%s", d.name);
            if (d.act == kActTune)
            {
                int f10 = (int)(freq * 10 + 0.5f);
                snprintf(value, sizeof(value), "%d.%d", f10 / 10, f10 % 10);
            }
            else
            {
                snprintf(value, sizeof(value), "%s%d", dialNet[i] > 0 ? "+" : "", dialNet[i]);
            }
        }
        if (on)
        {
            g.color(accent);
            g.rect((int16_t)(i * 100 + 4), 425, (int16_t)(i * 100 + 96), 428);
        }
        g.color(kGrey);
        g.text(cx, 433, kFont12, kCenterX, nameBuf);
        g.color(on ? accent : kInk);
        g.dot(cx, 453, 0, kCenterX, value);
    }
}

void DeckApp::drawHome(Gfx& g)
{
    g.color(kInk);
    g.dot(12, 50, 2, kLeft, "PICK A DECK");
    g.color(kGrey);
    g.text(788, 66, kFont12, kRight, "Press the button under a deck.");

    int16_t boxes[8 * 4];
    for (uint8_t i = 0; i < 8; i++)
    {
        boxes[i * 4] = (int16_t)(i * 100 + 4);
        boxes[i * 4 + 1] = 108;
        boxes[i * 4 + 2] = (int16_t)(i * 100 + 96);
        boxes[i * 4 + 3] = 416;
    }
    g.color(kTile);
    g.rects(boxes, 8, 6);

    for (uint8_t i = 0; i < 8; i++)
    {
        const char* key = "";
        const char* app = "";
        const char* b1 = "";
        const char* b2 = "";
        const char* st = "";
        uint32_t c = 0x8E8E88;
        if (i < kBuiltinDecks)
        {
            const BuiltinDeck& b = kBuiltin[i];
            key = b.key;
            app = b.app;
            b1 = b.blurb1;
            b2 = b.blurb2;
            st = b.status;
            c = b.accent;
        }
        else if (i == 4)
        {
            key = "MUSIC";
            app = "TRACKER";
            b1 = "The Tracker,";
            b2 = "as it was.";
            st = "POLYEND";
            c = 0xD6D6CE;
        }
        else if (i == 6)
        {
            key = mac ? "MAC" : "WIN";
            app = "COMPUTER";
            b1 = "Press to";
            b2 = "switch.";
            st = "CMD / CTRL";
        }
        int16_t x0 = (int16_t)(i * 100 + 4), x1 = (int16_t)(x0 + 92);
        if (!key[0]) continue;
        g.color(c);
        g.rect(x0, 108, x1, 114, 2);
        g.dot((int16_t)(x0 + 8), 128, 0, kLeft, key);
        g.color(kInk);
        g.text((int16_t)(x0 + 8), 152, kFont12, kLeft, app);
        g.color(0xB8B8B1);
        g.text((int16_t)(x0 + 8), 184, kFont12, kLeft, b1);
        g.text((int16_t)(x0 + 8), 200, kFont12, kLeft, b2);
        g.color(kGrey);
        g.text((int16_t)(x0 + 8), 394, kFont12, kLeft, st);
    }
}

void DeckApp::drawLive(Gfx& g)
{
    const uint32_t accent = deck().accent;
    const bool radio = deck().link == kLinkRadio;
    const DialDef& sel = page().dials[dialSel];
    char big[24];
    char sub[64];

    if (radio)
    {
        int f10 = (int)(freq * 10 + 0.5f);
        snprintf(big, sizeof(big), "%d.%d FM", f10 / 10, f10 % 10);
        snprintf(sub, sizeof(sub), "%s", rds[0] ? rds : "Shift + station pad = store");
    }
    else
    {
        snprintf(big, sizeof(big), "%s", sel.used() ? sel.name : "--");
        snprintf(sub, sizeof(sub), "%s%s", last[0] ? "Key: " : "Press a pad to see its name", last);
    }
    g.color(kGrey);
    g.text(12, 46, kFont12, kLeft, radio ? "TUNED" : "WHEEL");
    g.color(accent);
    g.dot(12, 70, 2, kLeft, big);
    g.color(0xC9C9C2);
    g.text(12, 124, kFont14, kLeft, sub);

    // bars
    for (uint8_t i = 0; i < 4; i++)
    {
        const char* label = "";
        char value[8] = "";
        int16_t fill0 = 392, fill1 = 392;
        if (radio)
        {
            if (i == 0)
            {
                label = "SIGNAL";
                int s = signal < 0 ? 0 : signal > 75 ? 75 : signal;
                fill1 = (int16_t)(392 + s * 228 / 75);
                snprintf(value, sizeof(value), "%d", signal);
            }
            else if (i == 1)
            {
                label = "CC OUT";
                int v = sending && lastCc != 255 ? lastCc : 0;
                fill1 = (int16_t)(392 + v * 228 / 127);
                snprintf(value, sizeof(value), "%s", sending ? "ON" : "OFF");
            }
            else continue;
        }
        else
        {
            const DialDef& d = page().dials[i];
            if (!d.used()) continue;
            label = d.name;
            int n = dialNet[i] < -20 ? -20 : dialNet[i] > 20 ? 20 : dialNet[i];
            int16_t mid = 506;
            fill0 = n < 0 ? (int16_t)(mid + n * 57 / 10) : mid;
            fill1 = n < 0 ? mid : (int16_t)(mid + n * 57 / 10);
            if (n == 0)
            {
                fill0 = (int16_t)(mid - 1);
                fill1 = (int16_t)(mid + 1);
            }
            snprintf(value, sizeof(value), "%s%d", dialNet[i] > 0 ? "+" : "", dialNet[i]);
        }
        int16_t y = (int16_t)(50 + i * 26);
        g.color(kGrey);
        g.text(322, (int16_t)(y - 1), kFont12, kLeft, label);
        g.color(kTrack);
        g.rect(392, y, 620, (int16_t)(y + 12), 2);
        g.color(i == dialSel || radio ? accent : 0x8E8E88);
        g.rect(fill0, y, fill1 > fill0 ? fill1 : (int16_t)(fill0 + 1), (int16_t)(y + 12), 1);
        g.color(kInk);
        g.dot(676, (int16_t)(y - 1), 0, kRight, value);
    }

    if (radio)
    {
        // signal scope: the last 14 seconds, newest on the right
        g.color(kGrey);
        g.text(322, 104, kFont12, kLeft, "SCOPE");
        g.color(kTrack);
        g.rect(392, 176, 676, 177);
        g.color(accent);
        int16_t bars[kHistory * 4];
        int n = 0;
        for (uint8_t i = 0; i < kHistory; i++)
        {
            int16_t h = (int16_t)(history[i] * 70 / 75);
            if (h < 1) continue;
            int16_t* q = &bars[n++ * 4];
            q[0] = (int16_t)(392 + i * 5);
            q[1] = (int16_t)(176 - h);
            q[2] = (int16_t)(q[0] + 4);
            q[3] = 176;
        }
        g.rects(bars, n);
    }

    // pages
    for (uint8_t i = 0; i < deck().pageCount; i++)
    {
        int16_t y = (int16_t)(50 + i * 26);
        bool on = i == pageIndex;
        if (on)
        {
            g.color(accent);
            g.rect(692, (int16_t)(y - 3), 788, (int16_t)(y + 17), 3);
        }
        g.color(on ? kDark : kGrey);
        g.text(700, (int16_t)(y - 1), kFont12, kLeft, deck().pages[i].name);
    }
}

void DeckApp::drawBlock(Gfx& g, uint8_t block)
{
    const uint32_t accent = deck().accent;
    const int16_t bx = (int16_t)(12 + block * 263);
    g.color(block == 0 ? accent : kGrey);
    g.text(bx, 189, kFont12, kLeft, page().block[block]);

    // Tiles are drawn in three batches (empty, key, held) to keep the frame
    // small; see the display-list note in i2Gfx.h.
    int16_t boxes[3][16 * 4];
    int count[3] = { 0, 0, 0 };
    for (uint8_t i = 0; i < 16; i++)
    {
        uint8_t padIndex = (uint8_t)((i / 4) * 12 + block * 4 + (i % 4));
        const PadDef& d = page().pads[padIndex];
        int kind = !d.used() ? 0 : ((held >> padIndex) & 1ULL) ? 2 : 1;
        int16_t* q = &boxes[kind][count[kind]++ * 4];
        q[0] = (int16_t)(bx + (i % 4) * 63);
        q[1] = (int16_t)(206 + (i / 4) * 54);
        q[2] = (int16_t)(q[0] + 59);
        q[3] = (int16_t)(q[1] + 50);
    }
    const uint32_t fill[3] = { kTileOff, kTile, accent };
    for (int kind = 0; kind < 3; kind++)
    {
        if (count[kind] == 0) continue;
        g.color(fill[kind]);
        g.rects(boxes[kind], count[kind], 5);
    }

    for (int pass = 0; pass < 2; pass++)
    {
        bool any = false;
        for (uint8_t i = 0; i < 16; i++)
        {
            uint8_t padIndex = (uint8_t)((i / 4) * 12 + block * 4 + (i % 4));
            const PadDef& d = page().pads[padIndex];
            bool on = (held >> padIndex) & 1ULL;
            if (!d.used() || (int)on != pass) continue;
            if (!any)
            {
                g.color(pass ? kDark : kInk);
                any = true;
            }
            char code[16];
            if (d.act == kActPreset && d.arg < 8)
            {
                int f10 = (int)(presets[d.arg] * 10 + 0.5f);
                snprintf(code, sizeof(code), "%d.%d", f10 / 10, f10 % 10);
            }
            else
            {
                snprintf(code, sizeof(code), "%s", d.mn);
            }
            int16_t cx = (int16_t)(bx + (i % 4) * 63 + 30);
            g.dot(cx, (int16_t)(206 + (i / 4) * 54 + 18), 0, kCenterX, code);
        }
    }
}

} // namespace i2deck
