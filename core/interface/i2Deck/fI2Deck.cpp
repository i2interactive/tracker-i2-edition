#include "i2Deck.h"

#include <string.h>

#include "Platform.h"
#include "mtPadBoard.h"
#include "mtPadsBacklight.h"
#include "mtStructs.h"

#include "core/songTimer.h"

#include "i2DeckApp.h"

extern uint8_t externalRamScratchBuff[];

cI2Deck i2Deck;
static cI2Deck* DK = &i2Deck;

namespace
{
using namespace i2arc;

//==============================================================================================================
// Services: SD card files, pad lights, scratch memory
//==============================================================================================================
constexpr size_t kIoChunk = 256; // small chunks go through FatFs' own sector buffer

class TrackerServices final : public Services
{
  public:
    Platform* platform = nullptr;
    uint32_t rng = 0x2545F491;

    virtual uint32_t random32() override
    {
        rng ^= rng << 13;
        rng ^= rng >> 17;
        rng ^= rng << 5;
        return rng;
    }

    virtual int32_t loadText(const char* path, char* buf, size_t cap) override
    {
        FileSystem* fs = platform ? platform->getFileSystem() : nullptr;
        if (fs == nullptr || !fs->exists(path))
        {
            return -1;
        }
        auto [ok, handle] = fs->openFile(path, FileSystem::kRead);
        if (!ok || handle == nullptr)
        {
            return -1;
        }
        size_t total = 0;
        while (total < cap)
        {
            size_t want = cap - total;
            if (want > kIoChunk) want = kIoChunk;
            int32_t got = fs->readFile(ioBuf, want, handle);
            if (got <= 0) break;
            memcpy(buf + total, ioBuf, (size_t)got);
            total += (size_t)got;
            if ((size_t)got < want) break;
        }
        fs->closeFile(handle);
        return (int32_t)total;
    }

    virtual bool saveText(const char* path, const char* data, size_t len) override
    {
        FileSystem* fs = platform ? platform->getFileSystem() : nullptr;
        if (fs == nullptr)
        {
            return false;
        }
        if (!fs->exists("i2/Deck"))
        {
            fs->mkdir(0, "i2/Deck", true);
        }
        if (fs->exists(path))
        {
            fs->remove(path);
        }
        auto [ok, handle] = fs->openFile(path, FileSystem::kWriteCreateNew);
        if (!ok || handle == nullptr)
        {
            return false;
        }
        bool good = true;
        size_t done = 0;
        while (done < len)
        {
            size_t n = len - done;
            if (n > kIoChunk) n = kIoChunk;
            memcpy(ioBuf, data + done, n);
            if (fs->writeFile(ioBuf, n, handle) != (int32_t)n)
            {
                good = false;
                break;
            }
            done += n;
        }
        fs->closeFile(handle);
        return good;
    }

    virtual void sfx(Sfx s) override { (void)s; }

    virtual void padLight(uint8_t pad, uint8_t level) override
    {
        if (pad >= 48) return;
        if (level == lastLevel[pad]) return;
        lastLevel[pad] = level;
        if (level == 0)
        {
            padsBacklight.setBackLayer(0, 0, pad);
        }
        else
        {
            padsBacklight.setBackLayer(1, level >= 2 ? mtConfig.values.padsLightFront : mtConfig.values.padsLightBack, pad);
        }
    }

    virtual void padsOff() override
    {
        padsBacklight.clearGlobalPads(1, 1, 1);
        memset(lastLevel, 0, sizeof(lastLevel));
    }

    virtual uint8_t* scratch(size_t& size) override
    {
        size = SHARED_SCRATCH_MEMORY_SIZE;
        return externalRamScratchBuff;
    }

    void resetLights() { memset(lastLevel, 0xFF, sizeof(lastLevel)); }

  private:
    __attribute__((aligned(4))) uint8_t ioBuf[kIoChunk];
    uint8_t lastLevel[48] = {};
};

//==============================================================================================================
// What the decks drive: USB keyboard, USB MIDI, the FM tuner
//==============================================================================================================
class TrackerIo final : public i2deck::DeckIo
{
  public:
    Platform* platform = nullptr;

    virtual bool keysConnected() override
    {
        UsbKeyboard* kb = keyboard();
        return kb != nullptr && kb->isConnected();
    }
    virtual void keyDown(uint8_t key, uint8_t mods) override
    {
        if (UsbKeyboard* kb = keyboard()) kb->press(key, mods);
    }
    virtual void keyUp(uint8_t key) override
    {
        if (UsbKeyboard* kb = keyboard()) kb->release(key);
    }
    virtual void keysOff() override
    {
        if (UsbKeyboard* kb = keyboard()) kb->releaseAll();
    }

    virtual void midiNote(uint8_t note, bool on, uint8_t channel) override
    {
        UsbMidi* midi = platform ? platform->getUsbMidi() : nullptr;
        if (midi == nullptr) return;
        if (on) midi->sendNoteOn(note, 100, channel);
        else midi->sendNoteOff(note, 0, channel);
    }
    virtual void midiCc(uint8_t cc, uint8_t value, uint8_t channel) override
    {
        UsbMidi* midi = platform ? platform->getUsbMidi() : nullptr;
        if (midi != nullptr) midi->sendControlChange(cc, value, channel);
    }

    virtual void radioPower(bool on) override
    {
        Radio* r = radio();
        if (r == nullptr) return;
        if (on)
        {
            r->enable(true);
            r->setRegion((Radio::Region)mtConfig.general.radioRegion);
            r->setSeekCallback(nullptr);
        }
        else
        {
            r->clearRDS();
            r->setSeekCallback(nullptr);
            r->enable(false);
        }
        radioIsOn = on;
    }
    virtual void radioTune(float mhz) override
    {
        Radio* r = radio();
        if (r == nullptr || !radioIsOn) return;
        float base = r->getBaseFrequency();
        if (mhz < base) mhz = base;
        if (mhz > Radio::kMaxAudioFrequencyMHz) mhz = Radio::kMaxAudioFrequencyMHz;
        r->clearRDS();
        r->setFrequency(mhz);
    }
    virtual void radioSeek(bool up) override
    {
        Radio* r = radio();
        if (r == nullptr || !radioIsOn || !r->isinitialized()) return;
        r->clearRDS();
        r->seek(up ? Radio::SeekDirection::kUp : Radio::SeekDirection::kDown);
    }
    virtual void radioPoll() override
    {
        Radio* r = radio();
        if (r == nullptr || !radioIsOn) return;
        r->updateRDS();
        r->updateSeek();
    }
    virtual float radioFreq() override
    {
        Radio* r = radio();
        return (r != nullptr && radioIsOn) ? r->getFrequency() : 0.0f;
    }
    virtual int radioSignal() override
    {
        Radio* r = radio();
        return (r != nullptr && radioIsOn) ? r->getSignalStrength() : 0;
    }
    virtual float radioStep() override
    {
        Radio* r = radio();
        float step = r != nullptr ? r->getSpacing() : 0.1f;
        return step > 0.04f ? step : 0.1f;
    }
    virtual const char* radioText() override
    {
        Radio* r = radio();
        const char* name = (r != nullptr && radioIsOn) ? r->getStationName() : nullptr;
        return name != nullptr ? name : "";
    }

    virtual void leave(uint8_t where) override { DK->requestLeave(where); }

  private:
    UsbKeyboard* keyboard() { return platform ? platform->getUsbKeyboard() : nullptr; }
    Radio* radio() { return platform ? platform->getRadio() : nullptr; }
    bool radioIsOn = false;
};

TrackerServices s_services;
TrackerIo s_io;
i2deck::DeckApp s_app;

bool toButton(uint8_t b, Button& out)
{
    switch (b)
    {
        case interfaceButtonPlay: out = Button::Play; return true;
        case interfaceButtonLeft: out = Button::Left; return true;
        case interfaceButtonRight: out = Button::Right; return true;
        case interfaceButtonUp: out = Button::Up; return true;
        case interfaceButtonDown: out = Button::Down; return true;
        case interfaceButtonShift: out = Button::Shift; return true;
        case interfaceButtonDelete: out = Button::Delete; return true;
        case interfaceButtonCopy: out = Button::Copy; return true;
        case interfaceButtonInsert: out = Button::Insert; return true;
        default: break;
    }
    if (b >= interfaceButton0 && b <= interfaceButton7)
    {
        out = (Button)((uint8_t)Button::Screen0 + (b - interfaceButton0));
        return true;
    }
    return false;
}

const uint8_t kKeyButtons[] = {
    interfaceButtonPlay,  interfaceButtonLeft,   interfaceButtonRight, interfaceButtonUp,     interfaceButtonDown,
    interfaceButtonShift, interfaceButtonDelete, interfaceButtonCopy,  interfaceButtonInsert,
};

// The Tracker's own screens stay one press away.
const uint8_t kModuleButtons[] = {
    interfaceButtonPerformance, interfaceButtonParams,     interfaceButtonFile,      interfaceButtonConfig,
    interfaceButtonMaster,      interfaceButtonSamplePlay, interfaceButtonSampleEdit, interfaceButtonSampleRec,
    interfaceButtonSampleLoad,  interfaceButtonSong,       interfaceButtonPattern,
};

} // namespace

static void functPads(uint8_t pad, uint8_t state, int16_t velo);
static void functEncoder(int16_t value);
static void functButton(uint8_t button, uint8_t state);
static void functSwitchModule(uint8_t button);

//==============================================================================================================
bool cI2Deck::startsFirst(Platform* platform)
{
    s_services.platform = platform;
    return !i2deck::DeckApp::bootToMusic(s_services);
}

void cI2Deck::start([[maybe_unused]] uint32_t options)
{
    moduleRefresh = 1;
    s_services.platform = platform;
    s_io.platform = platform;
    s_services.resetLights();
    s_services.padsOff();
    s_services.resetLights();
    pendingLeave = -1;

    songTimer.hide();

    for (uint8_t b : kModuleButtons)
    {
        ip->setButtonObj(b, buttonPress, functSwitchModule);
    }
    for (uint8_t b = interfaceButton0; b <= interfaceButton7; b++)
    {
        ip->setButtonObj(b, functButton);
    }
    for (uint8_t b : kKeyButtons)
    {
        ip->setButtonObj(b, functButton);
    }
    ip->setPotObj(interfacePot0, functEncoder, false);
    ip->setPadsGlobal(functPads);

    s_app.setIo(&s_io);
    s_app.begin(s_services);
    for (uint8_t l = 0; l < kLayerCount; l++)
    {
        layerData[l].game = &s_app;
        layerData[l].layer = l;
        display.refreshControl(canvas[l]);
    }
    display.synchronizeRefresh();
    running = true;
}

void cI2Deck::stop()
{
    moduleRefresh = 0;
    if (running)
    {
        s_app.end();
    }
    running = false;
    for (uint8_t l = 0; l < kLayerCount; l++)
    {
        layerData[l].game = nullptr;
    }
    mtPadBoard.releaseAllInstruments();
    s_services.padsOff();
    songTimer.show();
}

void cI2Deck::update()
{
    if (!running)
    {
        return;
    }

    if (pendingLeave >= 0)
    {
        uint8_t button = pendingLeave == 1 ? interfaceButtonSampleRec : interfaceButtonPattern;
        pendingLeave = -1;
        eventFunct(eventSwitchModule, this, &button, 0);
        return;
    }

    uint32_t now = platform && platform->getTimer() ? platform->getTimer()->millis() : 0;
    s_app.tick(now);

    // Redraw what changed, only when the display is idle so a new request
    // never restarts a redraw in progress.
    if (display.isControlsRefreshProcessIdle())
    {
        bool any = false;
        for (uint8_t l = 0; l < kLayerCount; l++)
        {
            if (s_app.dirty(l))
            {
                display.refreshControl(canvas[l]);
                any = true;
            }
        }
        if (any)
        {
            display.synchronizeRefresh();
        }
    }
}

//==============================================================================================================
void cI2Deck::onPad(uint8_t pad, uint8_t state)
{
    if (!running) return;
    if (state == padPress) s_app.pad(pad, true);
    else if (state == padRelease) s_app.pad(pad, false);
}

void cI2Deck::onButton(uint8_t button, uint8_t state)
{
    if (!running) return;
    Button b;
    if (!toButton(button, b)) return;
    if (state == buttonPress) s_app.button(b, true);
    else if (state == buttonRelease) s_app.button(b, false);
}

void cI2Deck::onJog(int16_t value)
{
    if (running) s_app.jog(value);
}

static void functPads(uint8_t pad, uint8_t state, [[maybe_unused]] int16_t velo)
{
    DK->onPad(pad, state);
}

static void functEncoder(int16_t value)
{
    DK->onJog(value);
}

static void functButton(uint8_t button, uint8_t state)
{
    DK->onButton(button, state);
}

static void functSwitchModule(uint8_t button)
{
    DK->eventFunct(eventSwitchModule, DK, &button, 0);
}
