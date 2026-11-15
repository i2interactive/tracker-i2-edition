#include "i2DeckApp.h"

#include <math.h>
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
    { "@STUTTER", kActGlitch, kFxStutter }, { "@CRUSH", kActGlitch, kFxCrush },
    { "@STOP", kActGlitch, kFxStop },       { "@SCAN", kActGlitch, kFxScan },
    { "@FILTER", kActGlitch, kFxFilter },   { "@FREEZE", kActGlitch, kFxFreeze },
    { "@R2", kActRepeat, 0 },  { "@R4", kActRepeat, 1 },  { "@R8", kActRepeat, 2 },
    { "@R16", kActRepeat, 3 }, { "@R32", kActRepeat, 4 }, { "@R64", kActRepeat, 5 },
    { "@RSSI", kActRssi, 0 },  { "@MON", kActMon, 0 },    { "@KILL", kActKill, 0 },
    { "@AMOUNT", kActAmount, 0 }, { "@LENGTH", kActLength, 0 }, { "@VOLUME", kActVolume, 0 },
    // MIXER: wheel jobs
    { "@FADER", kActFader, 0 }, { "@MASTER", kActMaster, 0 }, { "@PAN", kActPan, 0 },
    { "@STRIP", kActStrip, 0 }, { "@JOG", kActJogMcu, 0 },
    // MIXER: Mackie Control buttons (strip buttons are @REC1-8, @SOLO1-8,
    // @MUTE1-8, @SEL1-8; any other MCU note is @N<number>)
    { "@BANK-", kActMcu, 0x2E },  { "@BANK+", kActMcu, 0x2F },  { "@CH-", kActMcu, 0x30 },
    { "@CH+", kActMcu, 0x31 },    { "@FLIP", kActMcu, 0x32 },   { "@READ", kActMcu, 0x4A },
    { "@WRITE", kActMcu, 0x4B },  { "@TOUCH", kActMcu, 0x4D },  { "@LATCH", kActMcu, 0x4E },
    { "@SAVE", kActMcu, 0x50 },   { "@UNDO", kActMcu, 0x51 },   { "@MARKER", kActMcu, 0x54 },
    { "@NUDGE", kActMcu, 0x55 },  { "@CYCLE", kActMcu, 0x56 },  { "@REW", kActMcu, 0x5B },
    { "@FF", kActMcu, 0x5C },     { "@STOP", kActMcu, 0x5D },   { "@PLAY", kActMcu, 0x5E },
    { "@RECORD", kActMcu, 0x5F },
};

struct StripName
{
    const char* name;
    uint8_t base;
};
const StripName kStripNames[] = { { "@REC", i2mcu::kRec }, { "@SOLO", i2mcu::kSolo }, { "@MUTE", i2mcu::kMute }, { "@SEL", i2mcu::kSelect } };

const char* const kFxNames[kFxCount] = { "STUTTER", "CRUSH", "TAPE STOP", "SCAN", "FILTER", "FREEZE" };
// stutter lengths as note values (1/4 = one beat at the project tempo)
const char* const kDivNames[kDivisions] = { "1/2", "1/4", "1/8", "1/16", "1/32", "1/64" };

// Unlock: hold Shift + Rec together for 2 seconds (in i2 DECK).
constexpr uint32_t kComboMs = 2000;

bool sameWord(const char* s, const char* e, const char* word)
{
    size_t n = strlen(word);
    if ((size_t)(e - s) != n) return false;
    for (size_t i = 0; i < n; i++)
    {
        if (up(s[i]) != word[i]) return false;
    }
    return true;
}

bool parseAct(const char* s, const char* e, uint8_t& act, uint8_t& arg)
{
    for (const ActName& a : kActs)
    {
        if (sameWord(s, e, a.name))
        {
            act = a.act;
            arg = a.arg;
            return true;
        }
    }
    // @REC1 .. @SEL8: a strip button
    for (const StripName& sn : kStripNames)
    {
        size_t n = strlen(sn.name);
        if ((size_t)(e - s) == n + 1 && sameWord(s, s + n, sn.name) && s[n] >= '1' && s[n] <= '8')
        {
            act = kActMcu;
            arg = (uint8_t)(sn.base + (s[n] - '1'));
            return true;
        }
    }
    // @N<0-127>: any Mackie Control note
    if (e - s >= 3 && e - s <= 5 && up(s[1]) == 'N')
    {
        int v = 0;
        for (const char* q = s + 2; q < e; q++)
        {
            if (*q < '0' || *q > '9') return false;
            v = v * 10 + (*q - '0');
        }
        if (v > 127) return false;
        act = kActMcu;
        arg = (uint8_t)v;
        return true;
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
            out.link = startsWord(v, e, "MIDI") ? kLinkMidi : startsWord(v, e, "RADIO") ? kLinkRadio : startsWord(v, e, "MCU") ? kLinkMcu : kLinkKeys;
        }
        else if (startsWord(s, e, "REV"))
        {
            const char* v = s + 3;
            trim(v, e);
            char num[8];
            copyText(num, sizeof(num), v, e);
            out.rev = (uint8_t)strtoul(num, nullptr, 10);
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
namespace
{
constexpr size_t kSettingsCap = 384;

// "key=value" lookup; the value runs to the end of the line.
bool setting(const char* buf, const char* key, const char*& value)
{
    size_t n = strlen(key);
    for (const char* p = buf; (p = strstr(p, key)) != nullptr; p += n)
    {
        if ((p == buf || p[-1] == '\n') && p[n] == '=')
        {
            value = p + n + 1;
            return true;
        }
    }
    return false;
}

bool settingIs(const char* buf, const char* key, const char* word)
{
    const char* v = nullptr;
    return setting(buf, key, v) && strncmp(v, word, strlen(word)) == 0;
}
} // namespace

void DeckApp::readBoot(Services& services, bool& music, bool& lock)
{
    char buf[kSettingsCap];
    music = false;
    lock = true;
    int32_t n = services.loadText(kSettingsPath, buf, sizeof(buf) - 1);
    if (n <= 0) return;
    buf[n] = 0;
    music = settingIs(buf, "boot", "music");
    lock = !settingIs(buf, "lock", "off");
}

void DeckApp::loadSettings()
{
    readBoot(*sv, bootMusic, lockSetting);
    char buf[kSettingsCap];
    int32_t n = sv->loadText(kSettingsPath, buf, sizeof(buf) - 1);
    if (n <= 0) return;
    buf[n] = 0;
    const char* v = nullptr;
    if (settingIs(buf, "os", "win")) mac = false;
    if (settingIs(buf, "os", "mac")) mac = true;
    gradeSide = settingIs(buf, "grade", "premiere") ? 1 : 0;
    appSide = settingIs(buf, "app", "resolve") ? 0 : 1;
    if (setting(buf, "freq", v))
    {
        long f = strtol(v, nullptr, 10);
        if (f >= 875 && f <= 1080) freq = f / 10.0f;
    }
    if (setting(buf, "vol", v))
    {
        long f = strtol(v, nullptr, 10);
        if (f >= 0 && f <= 100) volume = (uint8_t)f;
    }
    for (int i = 0; i < 8; i++)
    {
        char key[4] = { 'p', (char)('1' + i), 0, 0 };
        if (setting(buf, key, v))
        {
            long f = strtol(v, nullptr, 10);
            if (f >= 875 && f <= 1080) presets[i] = f / 10.0f;
        }
    }
}

void DeckApp::saveSettings()
{
    char buf[kSettingsCap];
    int n = snprintf(buf, sizeof(buf), "os=%s\nboot=%s\nlock=%s\ngrade=%s\napp=%s\nfreq=%d\nvol=%d\n", mac ? "mac" : "win",
                     bootMusic ? "music" : "deck", lockSetting ? "on" : "off", gradeSide ? "premiere" : "resolve",
                     appSide ? "premiere" : "resolve", (int)(freq * 10 + 0.5f), volume);
    for (int i = 0; i < 8 && n > 0 && n < (int)sizeof(buf) - 12; i++)
    {
        n += snprintf(buf + n, sizeof(buf) - (size_t)n, "p%d=%d\n", i + 1, (int)(presets[i] * 10 + 0.5f));
    }
    if (n > 0 && n < (int)sizeof(buf)) sv->saveText(kSettingsPath, buf, (size_t)n);
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
    homePage = 0;
    deckIndex = -1;
    held = 0;
    keyHeld = 0;
    touching = -1;
    shift = false;
    recHeld = false;
    last[0] = 0;
    flashText[0] = 0;
    memset(passKey, 0, sizeof(passKey));
    comboDown = false;
    comboFired = false;
    loadSettings();
    if (io) io->lockChanged(locked());
    dirtyMask = 0x3F;
    lightPads();
}

void DeckApp::radioOff()
{
    if (!radioOn || !io) return;
    memset(fxHeld, 0, sizeof(fxHeld));
    latched = 0;
    repeatHeld = 0;
    rssiMod = false;
    i2glitch::Params off;
    sent = off;
    io->glitch(off);
    io->radioAudio(false, 0);
    io->radioPower(false);
    radioOn = false;
}

void DeckApp::end()
{
    releaseAll();
    radioOff();
}

void DeckApp::releaseAll()
{
    if (io) io->keysOff();
    if (io && !home && deckPtr)
    {
        for (uint8_t p = 0; p < kPads; p++)
        {
            if (!(held & (1ULL << p))) continue;
            const PadDef& d = page().pads[p];
            if (d.act == kActMcu) io->midiShort(0x90, d.arg, 0);
            else if (d.act == kActNone && !(keyHeld & (1ULL << p)) && deck().link == kLinkMidi) io->midiNote(p, false, (uint8_t)(pageIndex + 1));
        }
    }
    mcuRelease();
    held = 0;
    keyHeld = 0;
    memset(passKey, 0, sizeof(passKey));
    // momentary glitches end with their pads
    bool hadFx = repeatHeld != 0;
    for (uint8_t i = 0; i < kFxCount; i++)
    {
        hadFx = hadFx || fxHeld[i] != 0;
        fxHeld[i] = 0;
    }
    repeatHeld = 0;
    if (hadFx && radioOn && io)
    {
        io->radioTune(freq); // in case SCAN was hopping
        pushGlitch();
    }
}

void DeckApp::goHome()
{
    releaseAll();
    radioOff();
    if (!home && deckIndex >= 0) homePage = deckIndex >= kHomeDecks ? 1 : 0;
    home = true;
    dirtyMask = 0x3F;
    lightPads();
}

const BuiltinDeck& DeckApp::source(uint8_t index) const
{
    const BuiltinDeck& b = kBuiltin[index];
    return (sideOf(index) && b.alt) ? *b.alt : b;
}

void DeckApp::openDeck(uint8_t index)
{
    if (index >= kBuiltinDecks) return;
    releaseAll();
    const BuiltinDeck& b = source(index);
    size_t size = 0;
    char* text = reinterpret_cast<char*>(sv->scratch(size) + kTextOffset);

    char path[40];
    snprintf(path, sizeof(path), "i2/Deck/%s", b.file);
    int32_t n = sv->loadText(path, text, kTextCap - 1);
    int keys = 0;
    if (n > 0)
    {
        keys = parseDeck(text, (size_t)n, deck());
        uint8_t fileRev = deck().rev;
        parseDeck(b.text, strlen(b.text), deck());
        if (fileRev < deck().rev)
        {
            // The firmware's deck is newer than the card's: keep the old
            // file as <name>-old.txt and put the new one in its place.
            char old[48];
            size_t baseLen = strlen(b.file) > 4 ? strlen(b.file) - 4 : strlen(b.file);
            snprintf(old, sizeof(old), "i2/Deck/%.*s-old.txt", (int)baseLen, b.file);
            sv->saveText(old, text, (size_t)n);
            sv->saveText(path, b.text, strlen(b.text));
            keys = 0;
            flash("NEW DECK: OLD ONE SAVED AS -OLD");
        }
        else if (keys > 0)
        {
            keys = parseDeck(text, (size_t)n, deck());
        }
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
    last[0] = 0;
    if (deck().link == kLinkRadio && io && !radioOn)
    {
        io->radioPower(true);
        io->radioTune(freq);
        radioOn = true;
        rds[0] = 0;
        inLevel = 0;
        memset(history, 0, sizeof(history));
        monitor();
        pushGlitch();
    }
    setPage(0);
}

void DeckApp::setPage(uint8_t p)
{
    releaseAll();
    pageIndex = p;
    dialSel = 0;
    memset(dialNet, 0, sizeof(dialNet));
    glitchPage = false;
    for (uint8_t i = 0; i < kPads; i++)
    {
        uint8_t a = page().pads[i].act;
        glitchPage = glitchPage || a == kActGlitch || a == kActRepeat;
    }
    // first wheel job: the first dial that exists
    for (uint8_t i = 0; i < kDials; i++)
    {
        if (page().dials[i].used())
        {
            dialSel = i;
            break;
        }
    }
    dirtyMask = 0x3F;
    lightPads();
}

bool DeckApp::padOn(uint8_t p) const
{
    if (held & (1ULL << p)) return true;
    const PadDef& d = page().pads[p];
    switch (d.act)
    {
        case kActGlitch: return d.arg == kFxFreeze && (latched & i2glitch::kFreeze);
        case kActRssi: return rssiMod;
        case kActMon: return monOn;
        case kActSend: return sending;
        case kActMcu: return mcuLed(d.arg);
        default: return false;
    }
}

void DeckApp::lightPads()
{
    if (!sv) return;
    for (uint8_t p = 0; p < kPads; p++)
    {
        uint8_t level = 0;
        if (!home && page().pads[p].used())
        {
            level = padOn(p) ? 2 : 1;
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

void DeckApp::flash(const char* text)
{
    strncpy(flashText, text, sizeof(flashText) - 1);
    flashText[sizeof(flashText) - 1] = 0;
    flashUntil = 0; // the 2.5 s start at the next tick
    dirtyMask |= 1u << kLayerLive;
}

void DeckApp::flashLocked()
{
    flash("LOCKED");
}

void DeckApp::tap(i2ctl::KeyCombo c)
{
    if (!io || c.isEmpty()) return;
    io->keyDown(c.key, c.mods);
    io->keyUp(c.key);
}

//==============================================================================================================
// Lock: hold Shift + Rec together for 2 seconds
//==============================================================================================================
void DeckApp::comboHeld(bool both)
{
    if (both && !comboDown)
    {
        comboDown = true;
        comboFired = false;
        comboStart = nowMs;
    }
    else if (!both)
    {
        comboDown = false;
        comboFired = false;
    }
}

void DeckApp::comboCheck()
{
    if (!comboDown || comboFired || nowMs - comboStart < kComboMs) return;
    comboFired = true; // once per hold
    if (!lockSetting)
    {
        flash("LOCK IS OFF IN SETTINGS");
        return;
    }
    unlocked = !unlocked;
    if (io) io->lockChanged(locked());
    flash(unlocked ? "UNLOCKED UNTIL RESTART" : "LOCKED");
    dirtyMask |= 1u << kLayerChrome;
}

//==============================================================================================================
// Glitch (STATIC)
//==============================================================================================================
uint32_t DeckApp::beatSamples() const
{
    float bpm = io ? io->tempo() : 120.0f;
    if (bpm < 40.0f || bpm > 400.0f) bpm = 120.0f;
    return (uint32_t)(60.0f / bpm * (float)i2glitch::kSampleRate);
}

void DeckApp::monitor()
{
    // MON off keeps the tuner on the line input, at volume 0, so the jack's
    // signal never leaks in while STATIC is open
    if (io && radioOn) io->radioAudio(true, monOn ? volume : 0);
}

// Turns the deck's state into the processor's parameters, and sends them
// when they change.
void DeckApp::pushGlitch()
{
    if (!io) return;
    i2glitch::Params p;
    p.on = radioOn ? 1 : 0;
    uint8_t fx = latched;
    for (uint8_t i = 0; i < kFxCount; i++)
    {
        if (fxHeld[i]) fx |= (uint8_t)(1u << i);
    }
    if (repeatHeld) fx |= i2glitch::kStutter;
    if (rssiMod) fx |= i2glitch::kFilter;
    p.fx = radioOn ? fx : 0;

    auto a = [&](uint8_t i) { return amount[i] * 0.01f; };
    uint8_t div = repeatHeld ? repeatDiv : division;
    p.stutterLen = (beatSamples() * 2) >> div;
    p.freezeLen = (uint32_t)((0.20f - 0.18f * a(kFxFreeze)) * i2glitch::kSampleRate);
    p.crushBits = (uint8_t)(16 - (int)(a(kFxCrush) * 13.0f));
    p.crushHold = (uint8_t)(1 + (int)(a(kFxCrush) * a(kFxCrush) * 31.0f));
    float stopSec = 1.6f - 1.4f * a(kFxStop);
    p.stopStep = (uint32_t)(16777216.0f / (stopSec * i2glitch::kSampleRate));
    p.noise = (uint16_t)((burst ? 26000.0f : 2500.0f) * a(kFxScan));

    // filter: the amount closes it from 7 kHz to about 200 Hz; with RSSI on,
    // a weak signal closes it further
    float fc = 7000.0f;
    if (fxHeld[kFxFilter]) fc = 7000.0f * expf(-3.5f * a(kFxFilter));
    if (rssiMod)
    {
        float sg = signal / 60.0f;
        if (sg < 0) sg = 0;
        if (sg > 1) sg = 1;
        float fr = 150.0f + sg * sg * 6000.0f;
        if (fr < fc) fc = fr;
    }
    float coef = 2.0f * sinf(3.14159265f * fc / (float)i2glitch::kSampleRate);
    if (coef > 0.99f) coef = 0.99f;
    p.cutoff = (uint16_t)(coef * 32767.0f);
    float q = fxHeld[kFxFilter] ? 1.4f - 1.1f * a(kFxFilter) : 1.0f;
    p.damp = (uint16_t)(q * 16384.0f);

    if (memcmp(&p, &sent, sizeof(p)) != 0)
    {
        sent = p;
        io->glitch(p);
    }
}

void DeckApp::glitchAction(uint8_t act, uint8_t arg, bool down)
{
    switch (act)
    {
        case kActGlitch:
            if (arg >= kFxCount) return;
            if (arg == kFxFreeze)
            {
                if (down) latched ^= i2glitch::kFreeze;
            }
            else if (down)
            {
                fxHeld[arg]++;
                if (arg == kFxScan)
                {
                    nextHop = nowMs;
                    nextBurst = nowMs;
                }
            }
            else if (fxHeld[arg])
            {
                fxHeld[arg]--;
                if (arg == kFxScan && !fxHeld[arg])
                {
                    io->radioTune(freq); // back to the station
                    settleUntil = nowMs + 600;
                    burst = false;
                }
            }
            if (down) focus = arg;
            break;
        case kActRepeat:
            if (arg >= kDivisions) return;
            if (down)
            {
                repeatHeld++;
                repeatDiv = arg;
                focus = kFxStutter;
            }
            else if (repeatHeld)
            {
                repeatHeld--;
            }
            break;
        case kActRssi:
            if (down) rssiMod = !rssiMod;
            break;
        case kActMon:
            if (down)
            {
                monOn = !monOn;
                monitor();
            }
            break;
        case kActKill:
            if (down)
            {
                latched = 0;
                rssiMod = false;
            }
            break;
        default: return;
    }
    pushGlitch();
    if (down && (act == kActRssi || act == kActMon || act == kActKill || (act == kActGlitch && arg == kFxFreeze)))
    {
        lightPads();
        dirtyMask |= 1u << kLayerBlockA | 1u << kLayerBlockB | 1u << kLayerBlockC;
    }
    dirtyMask |= 1u << kLayerLive | 1u << kLayerChrome;
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
            dirtyMask |= 1u << kLayerChrome;
            break;
        case kActRecorder:
            saveSettings();
            io->leave(1);
            break;
        default: glitchAction(act, arg, true); break;
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
        if (held & (1ULL << padIndex)) return; // already down
        held |= 1ULL << padIndex;
        i2ctl::KeyCombo c = mac ? d.mac : d.win;
        if (d.act == kActMcu)
        {
            mcuButton(d.arg, true);
        }
        else if (d.act != kActNone)
        {
            runAction(d.act, d.arg);
        }
        else if (!c.isEmpty())
        {
            // a keyboard shortcut (KEYS decks, and the key pads in MIXER)
            if (shift) c.mods |= i2ctl::kModShift;
            io->keyDown(c.key, c.mods);
            heldKey[padIndex] = c.key;
            keyHeld |= 1ULL << padIndex;
        }
        else if (deck().link == kLinkMidi)
        {
            io->midiNote(padIndex, true, (uint8_t)(pageIndex + 1));
        }
        note(d.label[0] ? d.label : d.mn);
        sv->padLight(padIndex, 2);
        dirtyMask |= 1u << blockLayer;
    }
    else if (held & (1ULL << padIndex))
    {
        held &= ~(1ULL << padIndex);
        if (d.act == kActGlitch || d.act == kActRepeat)
        {
            glitchAction(d.act, d.arg, false);
        }
        else if (d.act == kActMcu)
        {
            mcuButton(d.arg, false);
        }
        else if (keyHeld & (1ULL << padIndex))
        {
            keyHeld &= ~(1ULL << padIndex);
            io->keyUp(heldKey[padIndex]);
        }
        else if (d.act == kActNone && deck().link == kLinkMidi)
        {
            io->midiNote(padIndex, false, (uint8_t)(pageIndex + 1));
        }
        sv->padLight(padIndex, padOn(padIndex) ? 2 : 1);
        dirtyMask |= 1u << blockLayer;
    }
}

void DeckApp::button(Button b, bool down)
{
    if (b == Button::Shift)
    {
        shift = down;
        comboHeld(shift && recHeld);
        return;
    }
    if (b == Button::Rec)
    {
        recHeld = down;
        comboHeld(shift && recHeld);
        return;
    }

    int hotkey = (int)b - (int)Button::Deck0;
    if (hotkey >= 0 && hotkey < kBuiltinDecks)
    {
        // NOTE / INSTRUMENT / FX1 / FX2 / MASTER / FILE / CONFIG: straight to
        // CUT / GRADE / HUB / STATIC / MIXER / FILE / SETUP
        if (!down || (shift && recHeld)) return;
        if (!home && deckIndex == hotkey) return; // already there
        if (!home) goHome();
        openDeck((uint8_t)hotkey);
        return;
    }

    int screen = (int)b - (int)Button::Screen0;
    if (screen >= 0 && screen <= 7)
    {
        if (!down) return;
        if (shift && recHeld) return; // holding the unlock combo: screen buttons do nothing
        if (home)
        {
            // two pages of decks: 1-4 CUT GRADE HUB STATIC, then 1-3 MIXER FILE SETUP
            int index = homePage * kHomeDecks + screen;
            if (screen < kHomeDecks && index < kBuiltinDecks) openDeck((uint8_t)index);
            else if (screen == 4 && io) io->leave(0);
            else if (screen == 6)
            {
                mac = !mac;
                saveSettings();
                dirtyMask = 0x3F;
            }
            else if (screen == 7)
            {
                homePage = homePage ? 0 : 1; // button 8: the other home page
                dirtyMask = 0x3F;
            }
            return;
        }
        if (screen == 7)
        {
            goHome(); // button 8 is HOME in every deck
            return;
        }
        if (screen == 5 && hasAlt())
        {
            // button 6 switches the deck's app (Resolve / Premiere). GRADE has
            // its own choice; MIXER, FILE and SETUP share one.
            if (kBuiltin[deckIndex].pick) appSide = appSide ? 0 : 1;
            else gradeSide = gradeSide ? 0 : 1;
            saveSettings();
            openDeck((uint8_t)deckIndex);
            flash(source((uint8_t)deckIndex).app);
            return;
        }
        if (screen == 6 && deck().pageCount > 1)
        {
            setPage((uint8_t)((pageIndex + 1) % deck().pageCount)); // button 7 turns the page
            return;
        }
        if (page().dials[screen].used())
        {
            dialSel = (uint8_t)screen;
            dirtyMask |= 1u << kLayerChrome | 1u << kLayerLive;
        }
        return;
    }

    // The Tracker's own buttons keep their printed meaning in keyboard decks
    // (and in MIXER).
    if (home || !io || (deck().link != kLinkKeys && deck().link != kLinkMcu)) return;
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
        if (passKey[slot]) return;
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
    int n = steps > 0 ? steps : -steps;
    if (n > 8) n = 8;
    bool cw = steps > 0;

    // On the GLITCH page, the wheel sets the strength of a held effect.
    bool fxDown = repeatHeld != 0;
    for (uint8_t i = 0; i < kFxCount; i++) fxDown = fxDown || fxHeld[i] != 0;
    const DialDef& d = page().dials[dialSel];
    if (glitchPage && (fxDown || d.act == kActAmount))
    {
        if (focus == kFxStutter)
        {
            // for STUTTER the wheel picks the length; clockwise = shorter
            int v = division + (cw ? 1 : -1);
            division = (uint8_t)(v < 0 ? 0 : v >= kDivisions ? kDivisions - 1 : v);
            if (repeatHeld) repeatDiv = division;
            pushGlitch();
            dirtyMask |= 1u << kLayerChrome | 1u << kLayerLive;
            return;
        }
        int v = amount[focus] + (cw ? n : -n) * (shift ? 10 : 2);
        amount[focus] = (uint8_t)(v < 0 ? 0 : v > 100 ? 100 : v);
        pushGlitch();
        dirtyMask |= 1u << kLayerChrome | 1u << kLayerLive;
        return;
    }
    if (!d.used()) return;

    if (d.act == kActTune)
    {
        freq += (cw ? io->radioStep() : -io->radioStep()) * n * (shift ? 10 : 1);
        if (freq < 87.5f) freq = 87.5f;
        if (freq > 108.0f) freq = 108.0f;
        io->radioTune(freq);
        rds[0] = 0;
    }
    else if (d.act == kActLength)
    {
        int v = division + (cw ? 1 : -1);
        division = (uint8_t)(v < 0 ? 0 : v >= kDivisions ? kDivisions - 1 : v);
        pushGlitch();
    }
    else if (d.act == kActVolume)
    {
        int v = volume + (cw ? n : -n) * (shift ? 10 : 2);
        volume = (uint8_t)(v < 0 ? 0 : v > 100 ? 100 : v);
        monitor();
    }
    else if (d.act == kActFader || d.act == kActMaster)
    {
        mcuFader(d.act == kActMaster ? i2mcu::kMasterChannel : strip, cw ? n : -n);
    }
    else if (d.act == kActPan)
    {
        io->midiShort(0xB0, (uint8_t)(i2mcu::kVpot + strip), i2mcu::relative(cw ? n : -n));
    }
    else if (d.act == kActJogMcu)
    {
        io->midiShort(0xB0, i2mcu::kJog, i2mcu::relative(cw ? n : -n));
    }
    else if (d.act == kActStrip)
    {
        int v = strip + (cw ? 1 : -1);
        if (v >= 0 && v < i2mcu::kStrips)
        {
            mcuRelease();
            strip = (uint8_t)v;
        }
        dirtyMask |= 1u << kLayerChrome | 1u << kLayerLive;
        return;
    }
    else if (!(mac ? d.cwMac : d.cwWin).isEmpty() || !(mac ? d.ccwMac : d.ccwWin).isEmpty())
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
    comboCheck();
    if (flashText[0] && flashUntil == 0)
    {
        flashUntil = now + 2500;
        if (flashUntil == 0) flashUntil = 1;
    }
    else if (flashText[0] && (int32_t)(now - flashUntil) >= 0)
    {
        flashText[0] = 0;
        dirtyMask |= 1u << kLayerLive;
    }
    // MIXER: let go of the fader 400 ms after the wheel stops; meters fall
    // one step every 120 ms between the host's updates (as on an MCU)
    if (touching >= 0 && (int32_t)(now - touchUntil) >= 0) mcuRelease();
    if (now - meterAt >= 120)
    {
        meterAt = now;
        bool any = false;
        for (uint8_t& m : meter)
        {
            if (m)
            {
                m--;
                any = true;
            }
        }
        if (any && mcu()) dirtyMask |= 1u << kLayerLive;
    }
    if (!radioOn || !io) return;
    io->radioPoll();

    // SCAN: hop the tuner around the station and spit noise bursts
    if (fxHeld[kFxScan])
    {
        if ((int32_t)(now - nextBurst) >= 0 && sv)
        {
            burst = !burst;
            nextBurst = now + 20 + (uint32_t)irand(*sv, burst ? 60 : 140);
            pushGlitch();
        }
        if ((int32_t)(now - nextHop) >= 0 && sv)
        {
            // every 120-400 ms, up to +/- 4 MHz away (tuning takes ~60 ms)
            float span = 0.4f + 3.6f * amount[kFxScan] * 0.01f;
            hopFreq = freq + (frand(*sv) * 2.0f - 1.0f) * span;
            if (hopFreq < 87.5f) hopFreq = 87.5f;
            if (hopFreq > 108.0f) hopFreq = 108.0f;
            io->radioTune(hopFreq);
            nextHop = now + 400 - (uint32_t)(2.8f * amount[kFxScan]);
            settleUntil = now + 600;
        }
    }

    if (now - lastPoll < 250) return;
    lastPoll = now;

    float f = io->radioFreq();
    int s = io->radioSignal();
    const char* t = io->radioText();
    bool scanning = fxHeld[kFxScan] || (int32_t)(now - settleUntil) < 0;
    if (!scanning && f > 80.0f && (f > freq + 0.04f || f < freq - 0.04f))
    {
        freq = f; // a seek landed
    }
    signal = s;
    memmove(history, history + 1, kHistory - 1);
    history[kHistory - 1] = (uint8_t)(s < 0 ? 0 : s > 75 ? 75 : s);
    float lv = io->audioLevel();
    if (lv >= 0) inLevel = lv;
    if (!scanning && t && strncmp(t, rds, sizeof(rds) - 1) != 0)
    {
        strncpy(rds, t, sizeof(rds) - 1);
        rds[sizeof(rds) - 1] = 0;
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
    if (rssiMod) pushGlitch();
    dirtyMask |= 1u << kLayerLive;
}

//==============================================================================================================
// MIXER: Mackie Control over USB MIDI
//==============================================================================================================
void DeckApp::mcuButton(uint8_t note, bool down)
{
    if (!io) return;
    io->midiShort(0x90, note, down ? 0x7F : 0x00);
    if (down && note >= i2mcu::kRec && note < i2mcu::kSelect + i2mcu::kStrips)
    {
        // a strip button also picks that strip for the wheel
        uint8_t s = (uint8_t)(note & 7);
        if (s != strip) mcuRelease();
        strip = s;
        dirtyMask |= 1u << kLayerChrome;
    }
    if (down && (note == i2mcu::kBankLeft || note == i2mcu::kBankRight))
    {
        int v = bank + (note == i2mcu::kBankRight ? 1 : -1);
        bank = (int8_t)(v < 0 ? 0 : v > 15 ? 15 : v);
    }
    dirtyMask |= 1u << kLayerLive;
}

// Moves a fader like a hand on a motor fader: touch, move, let go 400 ms
// after the last step. 128 steps from bottom to top (Shift: 16).
void DeckApp::mcuFader(uint8_t s, int steps)
{
    if (!io || s > i2mcu::kMasterChannel) return;
    if (!(faderKnown & (1u << s)) && !warnedFader)
    {
        warnedFader = true;
        flash("NO FADER FROM HOST: FROM 0");
    }
    if (touching != (int8_t)s)
    {
        mcuRelease();
        touching = (int8_t)s;
        io->midiShort(0x90, (uint8_t)(s == i2mcu::kMasterChannel ? i2mcu::kMasterTouch : i2mcu::kTouch + s), 0x7F);
    }
    int v = fader[s] + steps * (shift ? 1024 : 128);
    fader[s] = (uint16_t)(v < 0 ? 0 : v > i2mcu::kFaderMax ? i2mcu::kFaderMax : v);
    faderKnown |= (uint16_t)(1u << s);
    uint8_t m[3];
    i2mcu::fader(fader[s], s, m);
    io->midiShort(m[0], m[1], m[2]);
    touchUntil = nowMs + 400;
}

void DeckApp::mcuRelease()
{
    if (touching < 0) return;
    if (io) io->midiShort(0x90, (uint8_t)(touching == i2mcu::kMasterChannel ? i2mcu::kMasterTouch : i2mcu::kTouch + touching), 0x00);
    touching = -1;
}

void DeckApp::midiIn(uint8_t status, uint8_t d1, uint8_t d2)
{
    uint8_t type = status & 0xF0, ch = status & 0x0F;
    bool seen = true;
    if ((type == 0x90 || type == 0x80) && ch == 0 && d1 < 128)
    {
        // button LEDs: 127 on, 1 blinking (shown on), 0 off
        bool on = type == 0x90 && d2 > 0;
        if (on) leds[d1 >> 3] |= (uint8_t)(1u << (d1 & 7));
        else leds[d1 >> 3] &= (uint8_t)~(1u << (d1 & 7));
        if (mcu())
        {
            lightPads();
            dirtyMask |= 1u << kLayerBlockA | 1u << kLayerBlockB | 1u << kLayerBlockC;
        }
    }
    else if (type == 0xE0 && ch <= i2mcu::kMasterChannel)
    {
        // the host moves a fader (ignored while the wheel holds it)
        if (touching != (int8_t)ch)
        {
            fader[ch] = (uint16_t)(d1 | (d2 << 7));
            faderKnown |= (uint16_t)(1u << ch);
        }
    }
    else if (type == 0xD0)
    {
        uint8_t s = d1 >> 4, lv = d1 & 0x0F;
        if (s < i2mcu::kStrips && lv <= i2mcu::kMeterMax) meter[s] = lv;
    }
    else if (type == 0xB0 && ch == 0 && d1 >= i2mcu::kVpotRing && d1 < i2mcu::kVpotRing + i2mcu::kStrips)
    {
        ring[d1 - i2mcu::kVpotRing] = (uint8_t)(d2 | 0x80); // 0x80 = known
    }
    else
    {
        seen = false;
    }
    if (seen)
    {
        if (!hostSeen) dirtyMask |= 1u << kLayerChrome;
        hostSeen = true;
        hostAt = nowMs;
        if (mcu()) dirtyMask |= 1u << kLayerLive;
    }
}

void DeckApp::sysexIn(const uint8_t* d, uint16_t n)
{
    if (!i2mcu::ours(d, n)) return;
    uint8_t out[20];
    uint16_t k = i2mcu::reply(d, n, out);
    if (k && io) io->midiSysex(out, k);
    if (d[5] == 0x12 && n >= 8)
    {
        // the display: offset, then characters, then F7
        uint16_t at = d[6];
        for (uint16_t i = 7; i + 1 < n && at < i2mcu::kLcdChars; i++, at++)
        {
            char c = (char)d[i];
            lcd[at] = (c >= 32 && c < 127) ? c : ' ';
        }
    }
    else if (d[5] == 0x62 || d[5] == 0x63)
    {
        memset(leds, 0, sizeof(leds)); // all LEDs off / reset
        if (mcu()) lightPads();
    }
    if (!hostSeen) dirtyMask |= 1u << kLayerChrome;
    hostSeen = true;
    hostAt = nowMs;
    if (mcu()) dirtyMask |= 1u << kLayerLive;
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

void DeckApp::dialValue(uint8_t i, char* out, size_t cap) const
{
    const DialDef& d = page().dials[i];
    if (d.act == kActTune)
    {
        int f10 = (int)(freq * 10 + 0.5f);
        snprintf(out, cap, "%d.%d", f10 / 10, f10 % 10);
    }
    else if (d.act == kActAmount)
    {
        if (focus == kFxStutter) snprintf(out, cap, "%s", kDivNames[repeatHeld ? repeatDiv : division]);
        else snprintf(out, cap, "%d", amount[focus]);
    }
    else if (d.act == kActLength)
    {
        snprintf(out, cap, "%s", kDivNames[division]);
    }
    else if (d.act == kActVolume)
    {
        snprintf(out, cap, "%d", volume);
    }
    else if (d.act == kActFader || d.act == kActMaster)
    {
        uint8_t s = d.act == kActMaster ? i2mcu::kMasterChannel : strip;
        if (faderKnown & (1u << s)) snprintf(out, cap, "%d%%", (fader[s] * 100 + i2mcu::kFaderMax / 2) / i2mcu::kFaderMax);
        else snprintf(out, cap, "--");
    }
    else if (d.act == kActStrip)
    {
        snprintf(out, cap, "%d", bank * i2mcu::kStrips + strip + 1);
    }
    else if (d.act == kActPan && (ring[strip] & 0x80))
    {
        // V-Pot ring position 1-11, 6 = centre
        int p = ring[strip] & 0x0F;
        if (p == 0) snprintf(out, cap, "--");
        else if (p == 6) snprintf(out, cap, "C");
        else snprintf(out, cap, "%c%d", p < 6 ? 'L' : 'R', p < 6 ? 6 - p : p - 6);
    }
    else
    {
        snprintf(out, cap, "%s%d", dialNet[i] > 0 ? "+" : "", dialNet[i]);
    }
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
        bool keys = io && io->keysConnected();
        link = deck().link == kLinkKeys ? (keys ? "USB KEYS" : "NO USB")
               : deck().link == kLinkMidi ? "MIDI OUT"
               : deck().link == kLinkMcu  ? (!keys ? "NO USB" : hostSeen ? "MCU LINKED" : "MCU OUT")
                                          : (sending ? "FM + CC" : "FM");
    }
    chip(g, x, link, true, accent, x);
    if (lockSetting && unlocked)
    {
        chip(g, x, "UNLOCKED", false, accent, x); // the Tracker's own screens are open
    }

    // dial strip
    for (uint8_t i = 0; i < 8; i++)
    {
        int16_t cx = (int16_t)(i * 100 + 50);
        char nameBuf[16] = "";
        char value[16] = "";
        bool on = false;
        if (home)
        {
            const char* v = i == 4 ? "MUSIC" : i == 6 ? (mac ? "MAC" : "WIN") : i == 7 ? (homePage ? "BACK" : "MORE") : "";
            int index = homePage * kHomeDecks + i;
            if (i < kHomeDecks && index < kBuiltinDecks) v = kBuiltin[index].key;
            if (v[0] == 0) continue;
            snprintf(nameBuf, sizeof(nameBuf), "BUTTON %d", i + 1);
            snprintf(value, sizeof(value), "%s", v);
        }
        else if (i == 7)
        {
            snprintf(nameBuf, sizeof(nameBuf), "BACK");
            snprintf(value, sizeof(value), "HOME");
        }
        else if (i == 5 && hasAlt())
        {
            // the app switch: two stacked choices, the live one filled
            const BuiltinDeck& b = kBuiltin[deckIndex];
            const char* sides[2] = { b.app, b.alt->app };
            const uint32_t colors[2] = { b.accent, b.alt->accent };
            for (uint8_t sIdx = 0; sIdx < 2; sIdx++)
            {
                int16_t y0 = (int16_t)(430 + sIdx * 24);
                bool live = sIdx == sideOf((uint8_t)deckIndex);
                g.color(live ? colors[sIdx] : 0x55555C);
                g.rect(506, y0, 594, (int16_t)(y0 + 20), 3);
                if (!live)
                {
                    g.color(kDark);
                    g.rect(507, (int16_t)(y0 + 1), 593, (int16_t)(y0 + 19), 2);
                }
                g.color(live ? kDark : kGrey);
                g.text(550, (int16_t)(y0 + 3), kFont12, kCenterX, sides[sIdx]);
            }
            continue;
        }
        else if (i == 6 && deck().pageCount > 1)
        {
            snprintf(nameBuf, sizeof(nameBuf), "PAGE");
            snprintf(value, sizeof(value), "%d/%d", pageIndex + 1, deck().pageCount);
        }
        else
        {
            const DialDef& d = page().dials[i];
            if (!d.used()) continue;
            on = i == dialSel;
            snprintf(nameBuf, sizeof(nameBuf), "%s", d.name);
            dialValue(i, value, sizeof(value));
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
    g.dot(788, 50, 0, kRight, homePage ? "2/2" : "1/2");
    g.color(flashText[0] ? kHomeAccent : kGrey);
    g.text(788, 66, kFont12, kRight, flashText[0] ? flashText : homePage ? "Press the button under a deck, or MASTER / FILE / CONFIG."
                                                                     : "Press the button under a deck, or NOTE / INSTR / FX1 / FX2.");

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
        int index = homePage * kHomeDecks + i;
        if (i < kHomeDecks && index < kBuiltinDecks)
        {
            const BuiltinDeck& b = kBuiltin[index];
            key = b.key;
            app = source((uint8_t)index).app;
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
        else if (i == 7)
        {
            key = homePage ? "BACK" : "MORE";
            app = homePage ? "PAGE 1" : "PAGE 2";
            b1 = homePage ? "Cut, grade," : "Mixer, file,";
            b2 = homePage ? "hub, static." : "setup.";
            st = "MORE DECKS";
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

    if (radio && glitchPage)
    {
        drawGlitch(g);
    }
    else if (radio)
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
    if (!(radio && glitchPage))
    {
        g.color(kGrey);
        g.text(12, 46, kFont12, kLeft, radio ? "TUNED" : "WHEEL");
        g.color(accent);
        g.dot(12, 70, 2, kLeft, big);
        g.color(flashText[0] ? accent : 0xC9C9C2);
        g.text(12, 124, kFont14, kLeft, flashText[0] ? flashText : sub);

        // bars (MIXER: one strip per track instead)
        if (mcu()) drawStrips(g);
        for (uint8_t i = 0; i < (mcu() ? 0 : 4); i++)
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

// MIXER: eight strips, as the host shows them. Each has its name (from the
// host's display, else the track number the deck has counted), its fader
// and its meter. The wheel's strip is lit.
void DeckApp::drawStrips(Gfx& g)
{
    const uint32_t accent = deck().accent;
    int16_t tracks[i2mcu::kStrips * 2 * 4];
    int16_t fills[2][i2mcu::kStrips * 4];
    int16_t lit[i2mcu::kStrips * 4];
    int nf[2] = { 0, 0 }, nl = 0;
    for (uint8_t i = 0; i < i2mcu::kStrips; i++)
    {
        int16_t x0 = (int16_t)(322 + i * 45);
        int16_t* t = &tracks[i * 8];
        t[0] = (int16_t)(x0 + 2);
        t[1] = 66;
        t[2] = (int16_t)(x0 + 18);
        t[3] = 158;
        t[4] = (int16_t)(x0 + 22);
        t[5] = 66;
        t[6] = (int16_t)(x0 + 28);
        t[7] = 158;
        bool sel = i == strip;
        if (faderKnown & (1u << i))
        {
            int16_t* q = &fills[sel][nf[sel]++ * 4];
            q[0] = (int16_t)(x0 + 2);
            q[1] = (int16_t)(157 - fader[i] * 91 / i2mcu::kFaderMax);
            q[2] = (int16_t)(x0 + 18);
            q[3] = 158;
        }
        if (meter[i])
        {
            int16_t* q = &lit[nl++ * 4];
            q[0] = (int16_t)(x0 + 22);
            q[1] = (int16_t)(158 - meter[i] * 92 / i2mcu::kMeterMax);
            q[2] = (int16_t)(x0 + 28);
            q[3] = 158;
        }
    }
    g.color(kTrack);
    g.rects(tracks, i2mcu::kStrips * 2, 1);
    const uint32_t fillColor[2] = { 0x8E8E88, accent };
    for (int k = 0; k < 2; k++)
    {
        if (!nf[k]) continue;
        g.color(fillColor[k]);
        g.rects(fills[k], nf[k], 1);
    }
    if (nl)
    {
        g.color(0x5BE3A5);
        g.rects(lit, nl, 1);
    }
    // names: the host's top display row, 7 characters per strip
    for (uint8_t i = 0; i < i2mcu::kStrips; i++)
    {
        char name[8];
        const char* src = &lcd[i * 7];
        int n = 0;
        for (int k = 0; k < 7 && n < 5; k++)
        {
            if (src[k] > ' ' || (n > 0 && src[k] == ' ')) name[n++] = src[k];
        }
        while (n > 0 && name[n - 1] == ' ') n--;
        name[n] = 0;
        if (n == 0) snprintf(name, sizeof(name), "%d", bank * i2mcu::kStrips + i + 1);
        g.color(i == strip ? accent : kGrey);
        g.text((int16_t)(322 + i * 45 + 2), 46, kFont12, kLeft, name);
    }
    g.color(kGrey);
    g.text(322, 162, kFont12, kLeft, hostSeen ? "" : "Nothing from the host yet. Set it up: see the deck file.");
}

// STATIC's GLITCH page: the station on the left, one row per effect in the
// middle (its strength; lit while engaged), what the radio is doing on the
// right.
void DeckApp::drawGlitch(Gfx& g)
{
    const uint32_t accent = deck().accent;
    uint8_t engaged = sent.fx;
    char big[24];
    char sub[64] = "";

    int f10 = (int)(freq * 10 + 0.5f);
    snprintf(big, sizeof(big), "%d.%d FM", f10 / 10, f10 % 10);
    if (flashText[0])
    {
        snprintf(sub, sizeof(sub), "%s", flashText);
    }
    else if (engaged == 0)
    {
        snprintf(sub, sizeof(sub), "%s", monOn ? "Hold a pad to glitch the radio" : "MON is off: radio muted");
    }
    else
    {
        size_t n = 0;
        for (uint8_t i = 0; i < kFxCount && n < sizeof(sub) - 12; i++)
        {
            if (!(engaged & (1u << i))) continue;
            if (i == kFxStutter)
                n += (size_t)snprintf(sub + n, sizeof(sub) - n, "%sSTUTTER %s", n ? " + " : "", kDivNames[repeatHeld ? repeatDiv : division]);
            else
                n += (size_t)snprintf(sub + n, sizeof(sub) - n, "%s%s", n ? " + " : "", kFxNames[i]);
        }
    }
    g.color(kGrey);
    g.text(12, 46, kFont12, kLeft, "GLITCHING");
    g.color(accent);
    g.dot(12, 70, 2, kLeft, big);
    g.color(flashText[0] || engaged ? accent : 0xC9C9C2);
    g.text(12, 124, kFont14, kLeft, sub);
    g.color(kGrey);
    g.text(12, 150, kFont12, kLeft, rds[0] ? rds : "");

    // one row per effect: strength bar; filled in the accent while engaged
    int16_t track[kFxCount * 4];
    int16_t fills[2][kFxCount * 4];
    int nf[2] = { 0, 0 };
    for (uint8_t i = 0; i < kFxCount; i++)
    {
        int16_t y = (int16_t)(48 + i * 21);
        int16_t* t = &track[i * 4];
        t[0] = 404;
        t[1] = y;
        t[2] = 620;
        t[3] = (int16_t)(y + 10);
        bool on = engaged & (1u << i);
        int16_t* q = &fills[on][nf[on]++ * 4];
        q[0] = 404;
        q[1] = y;
        int pct = i == kFxStutter ? (division + 1) * 100 / kDivisions : amount[i];
        q[2] = (int16_t)(405 + pct * 215 / 100);
        q[3] = (int16_t)(y + 10);
    }
    g.color(kTrack);
    g.rects(track, kFxCount, 2);
    g.color(0x6E6E68);
    g.rects(fills[0], nf[0], 2);
    if (nf[1])
    {
        g.color(accent);
        g.rects(fills[1], nf[1], 2);
    }
    for (uint8_t i = 0; i < kFxCount; i++)
    {
        int16_t y = (int16_t)(48 + i * 21);
        bool on = engaged & (1u << i);
        g.color(on ? accent : i == focus ? kInk : kGrey);
        g.text(322, (int16_t)(y - 2), kFont12, kLeft, kFxNames[i]);
        char v[8];
        if (i == kFxStutter) snprintf(v, sizeof(v), "%s", kDivNames[repeatHeld ? repeatDiv : division]);
        else if (i == kFxFilter && rssiMod && !fxHeld[kFxFilter]) snprintf(v, sizeof(v), "RSSI");
        else if (i == kFxFreeze && (latched & i2glitch::kFreeze)) snprintf(v, sizeof(v), "HELD");
        else snprintf(v, sizeof(v), "%d", amount[i]);
        g.text(676, (int16_t)(y - 2), kFont12, kRight, v);
    }

    // the radio: input level, signal strength, monitor
    int lv = (int)(inLevel * 1.4f * 88.0f);
    if (lv > 88) lv = 88;
    int sg = signal < 0 ? 0 : signal > 75 ? 75 : signal;
    int16_t meters[2 * 4] = { 700, 120, 788, 128, 700, 148, 788, 156 };
    g.color(kTrack);
    g.rects(meters, 2, 2);
    int16_t lit[2 * 4] = { 700, 120, (int16_t)(701 + lv), 128, 700, 148, (int16_t)(701 + sg * 88 / 75), 156 };
    g.color(accent);
    g.rects(lit, 2, 2);
    g.color(kGrey);
    g.text(700, 104, kFont12, kLeft, monOn ? "AUDIO" : "AUDIO (MUTED)");
    g.text(700, 132, kFont12, kLeft, "SIGNAL");
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
        int kind = !d.used() ? 0 : padOn(padIndex) ? 2 : 1;
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
            bool on = padOn(padIndex);
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
