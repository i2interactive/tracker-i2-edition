#ifndef INTERFACE_I2DECK_I2DECKAPP_H_
#define INTERFACE_I2DECK_I2DECKAPP_H_

// i2 edition: i2 DECK, the control-surface interface.
//
// The whole 800x480 screen is drawn here: a launcher (HOME) and "decks" that
// share one layout: a live band, a key map mirroring the 48 pads as three
// 4x4 blocks, and a dial strip that gives the jog wheel eight jobs.
//
// This file has no Tracker dependencies. The firmware module (fI2Deck.cpp)
// and the PC test harness (tools/i2deck/host) both drive it through the
// Game, Services and DeckIo interfaces.

#include "i2Gfx.h"
#include "i2Glitch.h"
#include "i2Keys.h"
#include "i2Mcu.h"

namespace i2deck
{
using i2arc::Button;
using i2arc::Gfx;
using i2arc::Services;

// What a deck talks to
enum Link : uint8_t
{
    kLinkKeys,  // USB keyboard shortcuts
    kLinkMidi,  // USB MIDI notes and control changes
    kLinkRadio, // the built-in FM tuner
    kLinkMcu,   // a Mackie Control surface over USB MIDI (keys too)
};

enum Action : uint8_t
{
    kActNone,
    kActSeekUp,
    kActSeekDown,
    kActStepUp,
    kActStepDown,
    kActPreset, // arg = 0-7
    kActSend,   // toggle signal -> MIDI CC
    kActRecorder,
    kActTune, // dial
    // STATIC's GLITCH page
    kActGlitch, // arg = effect (kFx*), held = on (FREEZE latches)
    kActRepeat, // arg = beat division index, held = stutter at that length
    kActRssi,   // latch: signal strength drives the filter
    kActMon,    // latch: hear the radio
    kActKill,   // everything off
    kActAmount, // dial: the strength of the last effect touched
    kActLength, // dial: stutter length (beat division)
    kActVolume, // dial: how loud the radio is
    // MIXER (Mackie Control)
    kActMcu,    // arg = MCU note: press / release
    kActFader,  // dial: the picked strip's fader
    kActMaster, // dial: the master fader
    kActPan,    // dial: the picked strip's V-Pot (pan)
    kActStrip,  // dial: pick the strip (1-8) the wheel works on
    kActJogMcu, // dial: the MCU jog wheel
};

// Glitch effects, in pad order
enum GlitchFx : uint8_t
{
    kFxStutter,
    kFxCrush,
    kFxStop,
    kFxScan,
    kFxFilter,
    kFxFreeze,
    kFxCount,
};
constexpr uint8_t kDivisions = 6; // note values 1/2 to 1/64

struct PadDef
{
    char mn[5];
    char label[11];
    i2ctl::KeyCombo mac, win;
    uint8_t act, arg;
    bool used() const { return mn[0] != 0; }
};

struct DialDef
{
    char name[9];
    i2ctl::KeyCombo cwMac, ccwMac, cwWin, ccwWin;
    uint8_t act;
    bool used() const { return name[0] != 0; }
};

constexpr uint8_t kPads = 48;
constexpr uint8_t kDials = 7; // button 8 is always HOME; button 7 is PAGE in multi-page decks
constexpr uint8_t kMaxPages = 4;
constexpr uint8_t kHistory = 56; // signal samples kept for the STATIC scope

struct Page
{
    char name[9];
    char block[3][20];
    PadDef pads[kPads]; // physical order: left to right, top row first
    DialDef dials[kDials];
};

struct Deck
{
    char name[8];
    char app[24];
    uint8_t link;
    uint8_t rev; // REV line: a newer built-in replaces an older file on the card
    uint32_t accent;
    uint8_t pageCount;
    Page pages[kMaxPages];
};

// Parses a deck file into `out`. Returns the number of keys accepted.
int parseDeck(const char* text, size_t length, Deck& out);

struct BuiltinDeck
{
    const char* file; // on the SD card, under i2/Deck/
    const char* key;  // launcher name
    const char* app;  // also the name on the app switch
    const char* blurb1;
    const char* blurb2;
    const char* status;
    uint32_t accent;
    const char* text;
    // A second app for the same deck (GRADE: Resolve / Premiere). Screen
    // button 6 switches between them; the choice is kept in settings.txt.
    const BuiltinDeck* alt;
    // Which setting picks the side: 0 = grade= (GRADE), 1 = app= (MIXER,
    // FILE and SETUP share it).
    uint8_t pick;
};
// CUT, GRADE, HUB, STATIC, MIXER, FILE, SETUP
constexpr uint8_t kBuiltinDecks = 7;
constexpr uint8_t kHomeDecks = 4; // decks per home page
extern const BuiltinDeck kBuiltin[kBuiltinDecks];
// Every deck text the firmware carries, for the harness to check.
constexpr uint8_t kAllDecks = 11;
extern const BuiltinDeck* const kAll[kAllDecks];

// Hardware the decks drive. Implemented by the firmware and by the host.
class DeckIo
{
  public:
    virtual ~DeckIo() {}
    virtual bool keysConnected() = 0;
    virtual void keyDown(uint8_t key, uint8_t mods) = 0;
    virtual void keyUp(uint8_t key) = 0;
    virtual void keysOff() = 0;
    virtual void midiNote(uint8_t note, bool on, uint8_t channel) = 0;
    virtual void midiCc(uint8_t cc, uint8_t value, uint8_t channel) = 0;
    // Any channel message (status byte with channel), and a whole sysex
    // message including F0 and F7. Used by MIXER's Mackie Control.
    virtual void midiShort(uint8_t status, uint8_t d1, uint8_t d2) = 0;
    virtual void midiSysex(const uint8_t* data, uint16_t length) = 0;
    virtual void radioPower(bool on) = 0;
    virtual void radioTune(float mhz) = 0;
    virtual void radioSeek(bool up) = 0;
    virtual void radioPoll() = 0; // call often: seek + station text housekeeping
    virtual float radioFreq() = 0;
    virtual int radioSignal() = 0; // 0-75
    virtual float radioStep() = 0; // channel spacing in MHz (0.1 or 0.2)
    virtual const char* radioText() = 0;
    // on: route the tuner to the line input and play it (volume 0-100);
    // off: put the line input back the way the Tracker leaves it.
    virtual void radioAudio(bool on, uint8_t volume) = 0;
    virtual void glitch(const i2glitch::Params& p) = 0;
    virtual float audioLevel() = 0; // line-in RMS 0-1, or < 0 if none yet
    virtual float tempo() = 0;      // the project's BPM
    // 0 = the Tracker (music), 1 = the Tracker's sample recorder
    virtual void leave(uint8_t where) = 0;
    // The Tracker's File and Config screens follow this.
    virtual void lockChanged(bool locked) = 0;
};

class DeckApp final : public i2arc::Game
{
  public:
    void setIo(DeckIo* deckIo) { io = deckIo; }

    virtual const char* name() const override { return "i2 DECK"; }
    virtual void begin(Services& sv) override;
    virtual void end() override;
    virtual void tick(uint32_t nowMs) override;
    virtual void draw(Gfx& g, uint8_t layer) override;
    virtual bool dirty(uint8_t layer) const override { return dirtyMask & (1u << layer); }
    virtual void pad(uint8_t pad, bool down) override;
    virtual void button(Button b, bool down) override;
    virtual void jog(int16_t steps) override;
    virtual void midiIn(uint8_t status, uint8_t d1, uint8_t d2) override;
    virtual void sysexIn(const uint8_t* data, uint16_t length) override;

    // Start-up settings: boot=music asks to start in the Tracker; lock=off
    // turns the lock off. Defaults: start in i2 DECK, locked.
    static void readBoot(Services& sv, bool& music, bool& lock);
    // The boot splash setting: splash=full (default), short or off, as an
    // i2splash::Style (off = kStill: the finished splash, no motion).
    static uint8_t readSplash(Services& sv);

    bool locked() const { return lockSetting && !unlocked; }
    int8_t openDeckIndex() const { return home ? -1 : deckIndex; }
    uint8_t homePageIndex() const { return homePage; }
    // MIXER state, for the harness
    uint16_t faderPos(uint8_t strip) const { return strip <= i2mcu::kMasterChannel ? fader[strip] : 0; }
    bool mcuLed(uint8_t note) const { return note < 128 && (leds[note >> 3] & (1u << (note & 7))); }
    // A locked Tracker screen was asked for: say so.
    void flashLocked();

  private:
    enum : uint8_t
    {
        kLayerBg,
        kLayerChrome,
        kLayerLive,
        kLayerBlockA,
        kLayerBlockB,
        kLayerBlockC,
    };

    void openDeck(uint8_t index);
    const BuiltinDeck& source(uint8_t index) const;
    bool hasAlt() const { return !home && deckIndex >= 0 && kBuiltin[deckIndex].alt != nullptr; }
    uint8_t sideOf(uint8_t index) const { return kBuiltin[index].pick ? appSide : gradeSide; }
    // MIXER
    bool mcu() const { return !home && deck().link == kLinkMcu; }
    void mcuButton(uint8_t note, bool down);
    void mcuFader(uint8_t strip, int steps);
    void mcuRelease();
    void drawStrips(Gfx& g);
    void flash(const char* text);
    void comboHeld(bool both);
    void comboCheck();
    // glitch
    void pushGlitch();
    void glitchAction(uint8_t act, uint8_t arg, bool down);
    void monitor();
    uint32_t beatSamples() const;
    void goHome();
    void radioOff();
    void setPage(uint8_t p);
    void releaseAll();
    void runAction(uint8_t act, uint8_t arg);
    void loadSettings();
    void saveSettings();
    void lightPads();
    bool padOn(uint8_t p) const;
    void note(const char* text);
    const Page& page() const { return deck().pages[pageIndex]; }
    Deck& deck() const { return *deckPtr; }
    void tap(i2ctl::KeyCombo c);

    void drawBg(Gfx& g);
    void drawChrome(Gfx& g);
    void drawHome(Gfx& g);
    void drawLive(Gfx& g);
    void drawBlock(Gfx& g, uint8_t block);
    void drawGlitch(Gfx& g);
    void dialValue(uint8_t i, char* out, size_t cap) const;
    void chip(Gfx& g, int16_t rightX, const char* text, bool solid, uint32_t accent, int16_t& outLeft);

    Services* sv = nullptr;
    DeckIo* io = nullptr;
    Deck* deckPtr = nullptr;
    uint8_t dirtyMask = 0x3F;

    bool home = true;
    int8_t deckIndex = -1;
    uint8_t pageIndex = 0;
    uint8_t dialSel = 0;
    int16_t dialNet[kDials] = {};
    bool mac = true;
    bool shift = false;
    uint64_t held = 0;
    uint64_t keyHeld = 0; // pads holding a key down
    uint8_t heldKey[kPads] = {};
    uint8_t passKey[8] = {};
    char last[24] = "";
    uint32_t nowMs = 0;
    char flashText[32] = "";
    uint32_t flashUntil = 0;

    // settings
    bool bootMusic = false;
    bool lockSetting = true;
    uint8_t splashSetting = 0; // i2splash::Style, kept in settings.txt
    bool unlocked = false; // the secret combo, until restart
    uint8_t gradeSide = 0; // 0 = first app, 1 = alt
    uint8_t appSide = 1;   // MIXER / FILE / SETUP: 0 = Resolve, 1 = Premiere
    uint8_t homePage = 0;  // home screen: 0 = CUT..STATIC, 1 = MIXER..SETUP
    bool recHeld = false;
    bool comboDown = false;  // Shift + Rec both held
    bool comboFired = false; // already toggled during this hold
    uint32_t comboStart = 0;

    // radio
    bool radioOn = false;
    bool sending = false;
    float freq = 98.1f;
    float presets[8] = { 88.5f, 91.1f, 94.1f, 98.1f, 100.7f, 102.9f, 105.3f, 107.9f };
    int signal = 0;
    char rds[24] = "";
    uint8_t history[kHistory] = {};
    uint32_t lastPoll = 0;
    uint8_t lastCc = 255;

    // glitch
    bool glitchPage = false;
    bool monOn = true;
    uint8_t volume = 80;
    uint8_t amount[kFxCount] = { 50, 60, 50, 60, 60, 40 };
    uint8_t fxHeld[kFxCount] = {};
    uint8_t latched = 0; // FREEZE
    bool rssiMod = false;
    uint8_t focus = kFxStutter;
    uint8_t division = 3; // 1/16 note
    uint8_t repeatHeld = 0;
    uint8_t repeatDiv = 0;
    float inLevel = 0;
    float hopFreq = 0;
    uint32_t nextHop = 0, nextBurst = 0, settleUntil = 0;
    bool burst = false;
    i2glitch::Params sent;

    // MIXER (Mackie Control): what the host told us, and what we hold
    uint8_t leds[16] = {};                         // LED per MCU note
    uint16_t fader[i2mcu::kStrips + 1] = {};       // strips 1-8, master
    uint16_t faderKnown = 0;                       // host sent a position
    uint8_t meter[i2mcu::kStrips] = {};            // 0-12
    uint8_t ring[i2mcu::kStrips] = {};             // V-Pot ring, 0 = unknown
    uint32_t meterAt = 0;
    char lcd[i2mcu::kLcdChars] = {};
    uint32_t hostAt = 0;                           // last MIDI from the host
    bool hostSeen = false;
    uint8_t strip = 0;                             // the wheel's strip, 0-7
    int8_t bank = 0;                               // banks moved (estimate)
    int8_t touching = -1;                          // fader held by the wheel
    uint32_t touchUntil = 0;
    bool warnedFader = false;
};

} // namespace i2deck

#endif /* INTERFACE_I2DECK_I2DECKAPP_H_ */
