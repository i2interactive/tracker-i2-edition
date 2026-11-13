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
#include "i2Keys.h"

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
};

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
    const char* app;
    const char* blurb1;
    const char* blurb2;
    const char* status;
    uint32_t accent;
    const char* text;
};
constexpr uint8_t kBuiltinDecks = 4;
extern const BuiltinDeck kBuiltin[kBuiltinDecks];

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
    virtual void radioPower(bool on) = 0;
    virtual void radioTune(float mhz) = 0;
    virtual void radioSeek(bool up) = 0;
    virtual void radioPoll() = 0; // call often: seek + station text housekeeping
    virtual float radioFreq() = 0;
    virtual int radioSignal() = 0; // 0-75
    virtual float radioStep() = 0; // channel spacing in MHz (0.1 or 0.2)
    virtual const char* radioText() = 0;
    // 0 = the Tracker (music), 1 = the Tracker's sample recorder
    virtual void leave(uint8_t where) = 0;
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

    // True when settings ask to start in the Tracker instead of i2 DECK.
    static bool bootToMusic(Services& sv);

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
    void goHome();
    void setPage(uint8_t p);
    void releaseAll();
    void runAction(uint8_t act, uint8_t arg);
    void loadSettings();
    void saveSettings();
    void lightPads();
    void note(const char* text);
    const Page& page() const { return deck().pages[pageIndex]; }
    Deck& deck() const { return *deckPtr; }
    void tap(i2ctl::KeyCombo c);

    void drawBg(Gfx& g);
    void drawChrome(Gfx& g);
    void drawHome(Gfx& g);
    void drawLive(Gfx& g);
    void drawBlock(Gfx& g, uint8_t block);
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
    uint8_t heldKey[kPads] = {};
    uint8_t passKey[8] = {};
    char last[24] = "";
    uint32_t nowMs = 0;

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
};

} // namespace i2deck

#endif /* INTERFACE_I2DECK_I2DECKAPP_H_ */
