#include "i2Arcade.h"

#include <stdio.h>
#include <string.h>

#include "Platform.h"
#include "mtPadBoard.h"
#include "mtPadsBacklight.h"
#include "mtStructs.h"

#include "core/interfaceMenuPopup.h"
#include "core/songTimer.h"

#include "i2MedalGame.h"
#include "i2QuizGame.h"

extern uint8_t externalRamScratchBuff[];

cI2Arcade i2Arcade;
static cI2Arcade* ARC = &i2Arcade;

namespace
{
using namespace i2arc;

//==============================================================================================================
// Services for games: SD card files, random numbers, pad lights, scratch memory
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
        if (!fs->exists("i2/Arcade"))
        {
            fs->mkdir(0, "i2/Arcade", true);
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

    virtual void sfx(Sfx s) override
    {
        (void)s; // sound effects are a later step
    }

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

TrackerServices s_services;

//==============================================================================================================
// The arcade menu (a tiny game of its own)
//==============================================================================================================
class ArcadeMenu final : public Game
{
  public:
    uint8_t sel = 0;
    int8_t launchReq = -1;
    uint8_t dirtyMask = 7;

    virtual const char* name() const override { return "i2 ARCADE"; }
    virtual void begin(Services& sv) override
    {
        dirtyMask = 7;
        launchReq = -1;
        for (uint8_t p = 0; p < 48; p++) sv.padLight(p, (p / 12) == sel ? 2 : 1);
        this->sv = &sv;
    }
    virtual void tick(uint32_t) override {}
    virtual bool dirty(uint8_t layer) const override { return dirtyMask & (1u << layer); }
    virtual void pad(uint8_t pad, bool down) override
    {
        if (!down) return;
        uint8_t row = pad / 12;
        if (row < 2) launchReq = (int8_t)row;
    }
    virtual void button(Button b, bool down) override
    {
        if (!down) return;
        switch (b)
        {
            case Button::Up:
            case Button::Left: move(-1); break;
            case Button::Down:
            case Button::Right: move(1); break;
            case Button::Play: launchReq = (int8_t)sel; break;
            case Button::Screen0: launchReq = 0; break;
            case Button::Screen1: launchReq = 1; break;
            default: break;
        }
    }
    virtual void jog(int16_t steps) override { move(steps > 0 ? 1 : -1); }
    virtual const char* buttonLabel(uint8_t i) const override
    {
        return i == 0 ? "Quiz" : i == 1 ? "Medal" : nullptr;
    }
    virtual void draw(Gfx& g, uint8_t layer) override
    {
        dirtyMask &= (uint8_t)~(1u << layer);
        if (layer == 0)
        {
            const uint32_t bands[4] = { 0x0c0620, 0x160a34, 0x200e44, 0x2a1250 };
            for (int i = 0; i < 4; i++)
            {
                g.color(bands[i]);
                g.rect(kCanvasX, (int16_t)(kCanvasY + i * kCanvasH / 4), kCanvasX + kCanvasW,
                       (int16_t)(kCanvasY + (i + 1) * kCanvasH / 4 + 1));
            }
            g.color(0xffd040);
            g.text(400, kCanvasY + 18, kRom36, kCenterX, "i2 ARCADE");
            g.color(0xc0b0ff);
            g.text(400, kCanvasY + 62, kFont14B, kCenterX, "KAGEMORI CLASSICS - TRACKER EDITION");
            return;
        }
        if (layer != 1) return;
        const char* titles[2] = { "QUIZ KAGEMORI", "MEDAL MOUNTAIN" };
        const char* sub[2] = { "BRAIN BANZAI! You vs three CPU contestants, six rounds. Pad rows are your answer buttons.",
                               "A two-tier medal pusher. Jog wheel steers the chute, pads drop medals, the gate spins the reels." };
        const uint32_t accent[2] = { 0xe83838, 0xf0c030 };
        for (int i = 0; i < 2; i++)
        {
            int16_t y0 = (int16_t)(kCanvasY + 100 + i * 120), y1 = (int16_t)(y0 + 104);
            bool on = sel == i;
            g.color(on ? 0xffe040 : 0x6a8ae8);
            g.rect(40, y0, 760, y1, 10);
            g.color(on ? 0x20308a : 0x101a58);
            g.rect(43, y0 + 3, 757, y1 - 3, 8);
            g.color(accent[i]);
            g.rect(58, y0 + 18, 126, y1 - 18, 8);
            char n[4] = { (char)('1' + i), 0 };
            g.color(0xffffff);
            g.text(92, y0 + 34, kRom28, kCenterX, n);
            g.text(150, y0 + 22, kRom28, kLeft, titles[i]);
            g.color(0xc0c8f0);
            g.text(150, y0 + 62, kFont14, kLeft, sub[i]);
        }
        g.color(0x9a9ac0);
        g.text(400, kCanvasY + 360, kFont12, kCenterX,
               "Pad row 1 or 2 (or jog + Play) picks a game.  Exit leaves the arcade.  Scores save to the SD card.");
    }

  private:
    Services* sv = nullptr;
    void move(int d)
    {
        sel = (uint8_t)((sel + 2 + d) % 2);
        dirtyMask |= 2;
        if (sv)
        {
            for (uint8_t p = 0; p < 48; p++) sv->padLight(p, (p / 12) == sel ? 2 : 1);
        }
    }
};

ArcadeMenu s_menu;
QuizGame s_quiz;
MedalGame s_medal;

Game* gameFor(uint8_t i)
{
    switch (i)
    {
        case 1: return &s_quiz;
        case 2: return &s_medal;
        default: return &s_menu;
    }
}

// The screen's 8 buttons and the hardware keys games can use
Button toButton(uint8_t b, bool& ok)
{
    ok = true;
    switch (b)
    {
        case interfaceButtonPlay: return Button::Play;
        case interfaceButtonLeft: return Button::Left;
        case interfaceButtonRight: return Button::Right;
        case interfaceButtonUp: return Button::Up;
        case interfaceButtonDown: return Button::Down;
        case interfaceButtonShift: return Button::Shift;
        case interfaceButton0: return Button::Screen0;
        case interfaceButton1: return Button::Screen1;
        case interfaceButton2: return Button::Screen2;
        case interfaceButton3: return Button::Screen3;
        case interfaceButton4: return Button::Screen4;
        case interfaceButton5: return Button::Screen5;
        case interfaceButton6: return Button::Screen6;
        default: ok = false; return Button::Play;
    }
}

} // namespace

static void functPads(uint8_t pad, uint8_t state, int16_t velo);
static void functEncoder(int16_t value);
static void functButton(uint8_t button, uint8_t state);
static void functSwitchModule(uint8_t button);
static void functMenu(uint8_t state);

//==============================================================================================================
void cI2Arcade::start([[maybe_unused]] uint32_t options)
{
    moduleRefresh = 1;
    s_services.platform = platform;
    if (platform && platform->getTimer())
    {
        s_services.rng ^= platform->getTimer()->micros() * 2654435761u;
        if (s_services.rng == 0) s_services.rng = 0x2545F491;
    }
    s_services.resetLights();

    songTimer.hide();

    ip->setButtonObj(interfaceButtonPerformance, buttonPress, functSwitchModule);
    ip->setButtonObj(interfaceButtonParams, buttonPress, functSwitchModule);
    ip->setButtonObj(interfaceButtonFile, buttonPress, functSwitchModule);
    ip->setButtonObj(interfaceButtonConfig, buttonPress, functSwitchModule);
    ip->setButtonObj(interfaceButtonMaster, buttonPress, functSwitchModule);
    ip->setButtonObj(interfaceButtonSamplePlay, buttonPress, functSwitchModule);
    ip->setButtonObj(interfaceButtonSampleEdit, buttonPress, functSwitchModule);
    ip->setButtonObj(interfaceButtonSampleRec, buttonPress, functSwitchModule);
    ip->setButtonObj(interfaceButtonSampleLoad, buttonPress, functSwitchModule);
    ip->setButtonObj(interfaceButtonSong, buttonPress, functSwitchModule);
    ip->setButtonObj(interfaceButtonPattern, buttonPress, functSwitchModule);
    ip->setButtonObj(interfaceButtonMiniMenu, functMenu);

    for (uint8_t b = interfaceButton0; b <= interfaceButton7; b++)
    {
        ip->setButtonObj(b, functButton);
    }
    ip->setButtonObj(interfaceButtonPlay, functButton);
    ip->setButtonObj(interfaceButtonLeft, functButton);
    ip->setButtonObj(interfaceButtonRight, functButton);
    ip->setButtonObj(interfaceButtonUp, functButton);
    ip->setButtonObj(interfaceButtonDown, functButton);
    ip->setButtonObj(interfaceButtonShift, functButton);

    ip->setPotObj(interfacePot0, functEncoder, false);
    ip->setPadsGlobal(functPads);

    launch(0);
}

void cI2Arcade::stop()
{
    moduleRefresh = 0;
    if (active)
    {
        active->end();
    }
    active = nullptr;
    for (uint8_t l = 0; l < kLayerCount; l++)
    {
        layerData[l].game = nullptr;
    }
    mtPadBoard.releaseAllInstruments();
    s_services.padsOff();
    songTimer.show();
}

void cI2Arcade::launch(uint8_t index)
{
    if (active)
    {
        active->end();
    }
    s_services.padsOff();
    s_services.resetLights();
    activeIndex = index;
    active = gameFor(index);
    active->begin(s_services);
    for (uint8_t l = 0; l < kLayerCount; l++)
    {
        layerData[l].game = active;
        layerData[l].layer = l;
    }
    for (uint8_t i = 0; i < 8; i++) lastLabel[i] = (const char*)1; // force relabel
    showAll();
}

void cI2Arcade::update()
{
    if (active == nullptr)
    {
        return;
    }
    uint32_t now = platform && platform->getTimer() ? platform->getTimer()->millis() : 0;
    active->tick(now);

    if (activeIndex == 0)
    {
        int8_t req = s_menu.launchReq;
        if (req >= 0)
        {
            s_menu.launchReq = -1;
            launch((uint8_t)(req + 1));
            return;
        }
    }
    else if (active->wantsExit())
    {
        launch(0);
        return;
    }

    // Redraw dirty layers, one frame at a time (only when the display is idle,
    // so a new request never restarts a redraw in progress).
    if (display.isControlsRefreshProcessIdle())
    {
        bool any = false;
        for (uint8_t l = 0; l < kLayerCount; l++)
        {
            if (active->dirty(l))
            {
                display.refreshControl(canvas[l]);
                any = true;
            }
        }
        refreshLabels();
        if (any)
        {
            display.synchronizeRefresh();
        }
    }
}

//==============================================================================================================
void cI2Arcade::onPad(uint8_t pad, uint8_t state)
{
    if (!active) return;
    if (state == padPress) active->pad(pad, true);
    else if (state == padRelease) active->pad(pad, false);
}

void cI2Arcade::onButton(uint8_t button, uint8_t state)
{
    if (!active) return;
    if (button == interfaceButton7)
    {
        if (state != buttonPress) return;
        if (activeIndex == 0)
        {
            eventFunct(eventSwitchToPreviousModule, ARC, 0, 0);
        }
        else
        {
            launch(0);
        }
        return;
    }
    bool ok = false;
    Button b = toButton(button, ok);
    if (!ok) return;
    if (state == buttonPress) active->button(b, true);
    else if (state == buttonRelease) active->button(b, false);
}

void cI2Arcade::onJog(int16_t value)
{
    if (active) active->jog(value);
}

static void functPads(uint8_t pad, uint8_t state, [[maybe_unused]] int16_t velo)
{
    ARC->onPad(pad, state);
}

static void functEncoder(int16_t value)
{
    ARC->onJog(value);
}

static void functButton(uint8_t button, uint8_t state)
{
    ARC->onButton(button, state);
}

static void functSwitchModule(uint8_t button)
{
    ARC->eventFunct(eventSwitchModule, ARC, &button, 0);
}

static void functMenu(uint8_t state)
{
    if (state == buttonPress)
    {
        mtMenuPopup.showModulePopup(ARC);
    }
    else if (state == buttonRelease)
    {
        mtMenuPopup.hideModulePopup(ARC);
    }
}
