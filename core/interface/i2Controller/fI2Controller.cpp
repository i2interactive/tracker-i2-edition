#include "i2Controller.h"

#include <stdio.h>
#include <string.h>

#include "Platform.h"
#include "cr_section_macros.h"
#include "mtPadBoard.h"
#include "mtPadsBacklight.h"
#include "mtStructs.h"

#include "core/interfaceMenuPopup.h"
#include "core/songTimer.h"

cI2Controller i2Controller;
static cI2Controller* I2C = &i2Controller;

namespace
{
constexpr const char* kFolder = "i2/Controller";
constexpr const char* kProfilePath = "i2/Controller/premiere.txt";
constexpr const char* kSettingsPath = "i2/Controller/settings.txt";

constexpr size_t kFileBufferSize = 12 * 1024;
constexpr size_t kIoChunk = 256; // small chunks go through FatFs' own buffer

constexpr uint8_t kMaxJogTaps = 8;

// Big buffers live in external SDRAM (not zeroed at boot).
__NOINIT(EXTERNAL_RAM) i2ctl::Profile s_profile;
__NOINIT(EXTERNAL_RAM) __attribute__((aligned(4))) char s_fileBuffer[kFileBufferSize];

const char* const kJogModeNames[cI2Controller::jogModeCount] = {
    "Jog: Frame",
    "Jog: 5 Fr",
    "Jog: Edits",
    "Jog: Zoom",
};

// HID usages used by the jog wheel and hardware buttons
constexpr uint8_t kKeyRight = 0x4F;
constexpr uint8_t kKeyLeft = 0x50;
constexpr uint8_t kKeyDown = 0x51;
constexpr uint8_t kKeyUp = 0x52;
constexpr uint8_t kKeyEqual = 0x2E;
constexpr uint8_t kKeyMinus = 0x2D;
constexpr uint8_t kKeySpace = 0x2C;
constexpr uint8_t kKeyBackspace = 0x2A;
constexpr uint8_t kKeyC = 0x06;
constexpr uint8_t kKeyV = 0x19;

FileSystem* fs(Platform* platform)
{
    return platform != nullptr ? platform->getFileSystem() : nullptr;
}

// Reads a whole small text file into s_fileBuffer. Returns length or -1.
int readTextFile(Platform* platform, const char* path)
{
    FileSystem* fileSystem = fs(platform);
    if (fileSystem == nullptr || !fileSystem->exists(path))
    {
        return -1;
    }

    auto [isOpen, handle] = fileSystem->openFile(path, FileSystem::kRead);
    if (!isOpen || handle == nullptr)
    {
        return -1;
    }

    size_t total = 0;
    while (total < kFileBufferSize - 1)
    {
        size_t want = kFileBufferSize - 1 - total;
        if (want > kIoChunk)
        {
            want = kIoChunk;
        }
        int32_t got = fileSystem->readFile(s_fileBuffer + total, want, handle);
        if (got <= 0)
        {
            break;
        }
        total += (size_t)got;
        if ((size_t)got < want)
        {
            break;
        }
    }
    fileSystem->closeFile(handle);

    s_fileBuffer[total] = 0;
    return (int)total;
}

// Writes s_fileBuffer[0..length) to path, replacing any existing file.
bool writeTextFile(Platform* platform, const char* path, size_t length)
{
    FileSystem* fileSystem = fs(platform);
    if (fileSystem == nullptr)
    {
        return false;
    }

    if (!fileSystem->exists(kFolder))
    {
        fileSystem->mkdir(0, kFolder, true);
    }
    if (fileSystem->exists(path))
    {
        fileSystem->remove(path);
    }

    auto [isOpen, handle] = fileSystem->openFile(path, FileSystem::kWriteCreateNew);
    if (!isOpen || handle == nullptr)
    {
        return false;
    }

    bool ok = true;
    size_t written = 0;
    while (written < length)
    {
        size_t chunk = length - written;
        if (chunk > kIoChunk)
        {
            chunk = kIoChunk;
        }
        if (fileSystem->writeFile(s_fileBuffer + written, chunk, handle) != (int32_t)chunk)
        {
            ok = false;
            break;
        }
        written += chunk;
    }
    fileSystem->closeFile(handle);
    return ok;
}

} // namespace

static void functPads(uint8_t pad, uint8_t state, int16_t velo);
static void functEncoder(int16_t value);
static void functActionButton(uint8_t button, uint8_t state);
static void functShift(uint8_t button, uint8_t state);
static void functHardwareKey(uint8_t button, uint8_t state);
static void functSwitchModule(uint8_t button);
static void functMenu(uint8_t state);

const i2ctl::Profile& i2ControllerProfile()
{
    return s_profile;
}

const char* i2ControllerJogLabel(uint8_t mode)
{
    return mode < cI2Controller::jogModeCount ? kJogModeNames[mode] : "";
}

//==============================================================================================================
UsbKeyboard* cI2Controller::keyboard()
{
    return platform != nullptr ? platform->getUsbKeyboard() : nullptr;
}

void cI2Controller::update()
{
    UsbKeyboard* kb = keyboard();
    bool connected = (kb != nullptr) && kb->isConnected();
    if (connected != lastUsbState)
    {
        lastUsbState = connected;
        refreshTitle();
    }
}

void cI2Controller::start([[maybe_unused]] uint32_t options)
{
    moduleRefresh = 1;

    memset(heldKey, 0, sizeof(heldKey));
    memset(heldValid, 0, sizeof(heldValid));
    shiftHeld = false;

    if (!settingsLoaded)
    {
        loadSettings();
        settingsLoaded = true;
    }

    if (!profileLoaded)
    {
        profileLoaded = loadProfile();
    }
    if (layer >= s_profile.layerCount)
    {
        layer = 0;
    }

    UsbKeyboard* kb = keyboard();
    lastUsbState = (kb != nullptr) && kb->isConnected();

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
        ip->setButtonObj(b, functActionButton);
    }

    ip->setButtonObj(interfaceButtonShift, functShift);

    ip->setButtonObj(interfaceButtonPlay, functHardwareKey);
    ip->setButtonObj(interfaceButtonLeft, functHardwareKey);
    ip->setButtonObj(interfaceButtonRight, functHardwareKey);
    ip->setButtonObj(interfaceButtonUp, functHardwareKey);
    ip->setButtonObj(interfaceButtonDown, functHardwareKey);
    ip->setButtonObj(interfaceButtonDelete, functHardwareKey);
    ip->setButtonObj(interfaceButtonCopy, functHardwareKey);
    ip->setButtonObj(interfaceButtonInsert, functHardwareKey);

    ip->setPotObj(interfacePot0, functEncoder, false);
    ip->setPadsGlobal(functPads);

    showScreen();
    refreshPadLights();
}

void cI2Controller::stop()
{
    moduleRefresh = 0;

    UsbKeyboard* kb = keyboard();
    if (kb != nullptr)
    {
        kb->releaseAll();
    }
    memset(heldValid, 0, sizeof(heldValid));
    gridData.pressedMask = 0;

    mtPadBoard.releaseAllInstruments();
    padsBacklight.clearGlobalPads(1, 1, 1);

    songTimer.show();
}

//==============================================================================================================
bool cI2Controller::loadProfile()
{
    int length = readTextFile(platform, kProfilePath);

    if (length <= 0)
    {
        // First launch: write the default Premiere profile so it can be edited.
        size_t n = i2ctl::kDefaultPremiereProfileLength;
        if (n > kFileBufferSize - 1)
        {
            n = kFileBufferSize - 1;
        }
        memcpy(s_fileBuffer, i2ctl::kDefaultPremiereProfile, n);
        s_fileBuffer[n] = 0;
        writeTextFile(platform, kProfilePath, n);
        length = (int)n;
    }

    int accepted = i2ctl::parseProfile(s_fileBuffer, (size_t)length, s_profile);
    if (accepted == 0 || s_profile.layerCount == 0)
    {
        // Broken file: fall back to the built-in profile (file is left alone).
        i2ctl::parseProfile(
          i2ctl::kDefaultPremiereProfile, i2ctl::kDefaultPremiereProfileLength, s_profile);
        return false;
    }
    return true;
}

void cI2Controller::reloadProfile()
{
    UsbKeyboard* kb = keyboard();
    if (kb != nullptr)
    {
        kb->releaseAll();
    }
    memset(heldValid, 0, sizeof(heldValid));
    gridData.pressedMask = 0;

    profileLoaded = loadProfile();
    if (layer >= s_profile.layerCount)
    {
        layer = 0;
    }
    showScreen();
    refreshPadLights();
}

void cI2Controller::loadSettings()
{
    int length = readTextFile(platform, kSettingsPath);
    if (length <= 0)
    {
        return;
    }

    if (strstr(s_fileBuffer, "os=win") != nullptr)
    {
        mac = false;
    }
    else if (strstr(s_fileBuffer, "os=mac") != nullptr)
    {
        mac = true;
    }

    const char* j = strstr(s_fileBuffer, "jog=");
    if (j != nullptr && j[4] >= '0' && j[4] < '0' + jogModeCount)
    {
        jogMode = (uint8_t)(j[4] - '0');
    }
}

void cI2Controller::saveSettings()
{
    int n = snprintf(s_fileBuffer, kFileBufferSize, "os=%s\njog=%d\n", mac ? "mac" : "win", jogMode);
    if (n > 0)
    {
        writeTextFile(platform, kSettingsPath, (size_t)n);
    }
}

//==============================================================================================================
void cI2Controller::padEvent(uint8_t pad, uint8_t state)
{
    if (pad >= i2ctl::kPadCount || s_profile.layerCount == 0)
    {
        return;
    }

    UsbKeyboard* kb = keyboard();

    if (state == padPress)
    {
        const i2ctl::PadAction& a = s_profile.layers[layer].pads[pad];
        i2ctl::KeyCombo combo = mac ? a.mac : a.win;
        if (combo.isEmpty())
        {
            return;
        }
        if (shiftHeld)
        {
            combo.mods |= i2ctl::kModShift;
        }

        if (kb != nullptr)
        {
            kb->press(combo.key, combo.mods);
        }
        heldKey[pad] = combo.key;
        heldValid[pad] = 1;

        gridData.pressedMask |= (1ULL << pad);
        padsBacklight.setFrontLayer(1, mtConfig.values.padsLightFront, pad);
        refreshGrid();
    }
    else if (state == padRelease)
    {
        if (heldValid[pad])
        {
            if (kb != nullptr)
            {
                kb->release(heldKey[pad]);
            }
            heldValid[pad] = 0;
        }

        if (gridData.pressedMask & (1ULL << pad))
        {
            gridData.pressedMask &= ~(1ULL << pad);
            padsBacklight.setFrontLayer(0, 0, pad);
            refreshGrid();
        }
    }
}

void cI2Controller::tapCombo(i2ctl::KeyCombo combo)
{
    UsbKeyboard* kb = keyboard();
    if (kb == nullptr || combo.isEmpty())
    {
        return;
    }
    kb->press(combo.key, combo.mods);
    kb->release(combo.key);
}

void cI2Controller::jog(int16_t value)
{
    if (value == 0)
    {
        return;
    }

    bool forward = value > 0;
    uint8_t taps = (uint8_t)((value > 0 ? value : -value));
    if (taps > kMaxJogTaps)
    {
        taps = kMaxJogTaps;
    }

    i2ctl::KeyCombo combo{};
    switch (jogMode)
    {
        case jogFrames: combo.key = forward ? kKeyRight : kKeyLeft; break;
        case jogFiveFrames:
            combo.key = forward ? kKeyRight : kKeyLeft;
            combo.mods = i2ctl::kModShift;
            break;
        case jogEdits: combo.key = forward ? kKeyDown : kKeyUp; break;
        case jogZoom: combo.key = forward ? kKeyEqual : kKeyMinus; break;
        default: return;
    }
    if (shiftHeld)
    {
        combo.mods |= i2ctl::kModShift;
    }

    for (uint8_t i = 0; i < taps; i++)
    {
        tapCombo(combo);
    }
}

void cI2Controller::hardwareKey(uint8_t button, uint8_t state)
{
    i2ctl::KeyCombo combo{};
    const uint8_t cmd = mac ? i2ctl::kModGui : i2ctl::kModCtrl;

    switch (button)
    {
        case interfaceButtonPlay: combo.key = kKeySpace; break;
        case interfaceButtonLeft: combo.key = kKeyLeft; break;
        case interfaceButtonRight: combo.key = kKeyRight; break;
        case interfaceButtonUp: combo.key = kKeyUp; break;
        case interfaceButtonDown: combo.key = kKeyDown; break;
        case interfaceButtonDelete: combo.key = kKeyBackspace; break;
        case interfaceButtonCopy:
            combo.key = kKeyC;
            combo.mods = cmd;
            break;
        case interfaceButtonInsert:
            combo.key = kKeyV;
            combo.mods = cmd;
            break;
        default: return;
    }
    if (shiftHeld)
    {
        combo.mods |= i2ctl::kModShift;
    }

    UsbKeyboard* kb = keyboard();
    if (kb == nullptr)
    {
        return;
    }

    // Arrow keys and Space follow the button (hold to repeat on the host).
    if (state == buttonPress)
    {
        kb->press(combo.key, combo.mods);
    }
    else if (state == buttonRelease)
    {
        kb->release(combo.key);
    }
}

void cI2Controller::setShift(bool held)
{
    if (shiftHeld == held)
    {
        return;
    }
    shiftHeld = held;
    refreshTitle();
}

void cI2Controller::nextLayer(int8_t direction)
{
    if (s_profile.layerCount <= 1)
    {
        return;
    }

    // Let go of anything held on the old layer.
    UsbKeyboard* kb = keyboard();
    if (kb != nullptr)
    {
        kb->releaseAll();
    }
    memset(heldValid, 0, sizeof(heldValid));
    gridData.pressedMask = 0;

    int next = (int)layer + direction;
    if (next < 0)
    {
        next = s_profile.layerCount - 1;
    }
    if (next >= s_profile.layerCount)
    {
        next = 0;
    }
    layer = (uint8_t)next;

    refreshTitle();
    refreshGrid();
    refreshPadLights();
}

void cI2Controller::nextJogMode()
{
    jogMode = (uint8_t)((jogMode + 1) % jogModeCount);
    saveSettings();
    refreshBottomLabels();
}

void cI2Controller::toggleOs()
{
    UsbKeyboard* kb = keyboard();
    if (kb != nullptr)
    {
        kb->releaseAll();
    }
    memset(heldValid, 0, sizeof(heldValid));

    mac = !mac;
    saveSettings();
    refreshTitle();
    refreshGrid();
    refreshBottomLabels();
}

void cI2Controller::refreshPadLights()
{
    padsBacklight.clearGlobalPads(1, 1, 1);
    if (s_profile.layerCount == 0)
    {
        return;
    }

    const i2ctl::Layer& l = s_profile.layers[layer];
    for (uint8_t pad = 0; pad < i2ctl::kPadCount; pad++)
    {
        if (l.pads[pad].label[0] != 0)
        {
            padsBacklight.setBackLayer(1, mtConfig.values.padsLightBack, pad);
        }
    }
}

//==============================================================================================================
static void functPads(uint8_t pad, uint8_t state, [[maybe_unused]] int16_t velo)
{
    I2C->padEvent(pad, state);
}

static void functEncoder(int16_t value)
{
    I2C->jog(value);
}

static void functShift([[maybe_unused]] uint8_t button, uint8_t state)
{
    if (state == buttonPress)
    {
        I2C->setShift(true);
    }
    else if (state == buttonRelease)
    {
        I2C->setShift(false);
    }
}

static void functHardwareKey(uint8_t button, uint8_t state)
{
    I2C->hardwareKey(button, state);
}

// Screen buttons: 0 Layer <, 1 Layer >, 2 Jog mode, 3 Mac/Win, 4 Reload, 7 Close
static void functActionButton(uint8_t button, uint8_t state)
{
    if (state != buttonPress)
    {
        return;
    }

    switch (button)
    {
        case 0: I2C->nextLayer(-1); break;
        case 1: I2C->nextLayer(1); break;
        case 2: I2C->nextJogMode(); break;
        case 3: I2C->toggleOs(); break;
        case 4: I2C->reloadProfile(); break;
        case 7: I2C->eventFunct(eventSwitchToPreviousModule, I2C, 0, 0); break;
        default: break;
    }
}

static void functSwitchModule(uint8_t button)
{
    I2C->eventFunct(eventSwitchModule, I2C, &button, 0);
}

static void functMenu(uint8_t state)
{
    if (state == buttonPress)
    {
        mtMenuPopup.showModulePopup(I2C);
    }
    else if (state == buttonRelease)
    {
        mtMenuPopup.hideModulePopup(I2C);
    }
}
