#include "i2ControllerProfile.h"

#include <stdio.h>
#include <string.h>

namespace i2ctl
{
namespace
{
struct NamedKey
{
    const char* name;
    uint8_t usage;
};

// Named keys (letters and digits are handled separately).
const NamedKey kNamedKeys[] = {
    { "ENTER", 0x28 },  { "RETURN", 0x28 }, { "ESC", 0x29 },     { "BKSP", 0x2A },
    { "BACKSPACE", 0x2A }, { "TAB", 0x2B }, { "SPACE", 0x2C },   { "MINUS", 0x2D },
    { "-", 0x2D },      { "EQUAL", 0x2E },  { "=", 0x2E },       { "[", 0x2F },
    { "]", 0x30 },      { "\\", 0x31 },     { "BACKSLASH", 0x31 },     { ";", 0x33 },       { "'", 0x34 },
    { "`", 0x35 },      { ",", 0x36 },      { ".", 0x37 },       { "/", 0x38 },
    { "INS", 0x49 },    { "HOME", 0x4A },   { "PGUP", 0x4B },    { "DEL", 0x4C },
    { "END", 0x4D },    { "PGDN", 0x4E },   { "RIGHT", 0x4F },   { "LEFT", 0x50 },
    { "DOWN", 0x51 },   { "UP", 0x52 },
};

// Short names used on screen, by usage ID.
const NamedKey kDisplayNames[] = {
    { "Enter", 0x28 }, { "Esc", 0x29 },  { "Bksp", 0x2A }, { "Tab", 0x2B },  { "Space", 0x2C },
    { "-", 0x2D },     { "=", 0x2E },    { "[", 0x2F },    { "]", 0x30 },    { "\\", 0x31 },
    { ";", 0x33 },     { "'", 0x34 },    { "`", 0x35 },    { ",", 0x36 },    { ".", 0x37 },
    { "/", 0x38 },     { "Ins", 0x49 },  { "Home", 0x4A }, { "PgUp", 0x4B }, { "Del", 0x4C },
    { "End", 0x4D },   { "PgDn", 0x4E }, { "Right", 0x4F }, { "Left", 0x50 }, { "Down", 0x51 },
    { "Up", 0x52 },
};

char upper(char c)
{
    return (c >= 'a' && c <= 'z') ? (char)(c - 'a' + 'A') : c;
}

bool tokenEquals(const char* token, size_t length, const char* name)
{
    size_t nameLength = strlen(name);
    if (nameLength != length)
    {
        return false;
    }
    for (size_t i = 0; i < length; i++)
    {
        if (upper(token[i]) != name[i])
        {
            return false;
        }
    }
    return true;
}

bool isSpace(char c)
{
    return c == ' ' || c == '\t' || c == '\r';
}

void trim(const char*& start, const char*& end)
{
    while (start < end && isSpace(*start))
    {
        start++;
    }
    while (end > start && isSpace(*(end - 1)))
    {
        end--;
    }
}

bool parseKeyToken(const char* token, size_t length, uint8_t& usage)
{
    if (length == 1)
    {
        char c = upper(token[0]);
        if (c >= 'A' && c <= 'Z')
        {
            usage = (uint8_t)(0x04 + (c - 'A'));
            return true;
        }
        if (c >= '1' && c <= '9')
        {
            usage = (uint8_t)(0x1E + (c - '1'));
            return true;
        }
        if (c == '0')
        {
            usage = 0x27;
            return true;
        }
    }

    if (length >= 2 && length <= 3 && upper(token[0]) == 'F')
    {
        int n = 0;
        for (size_t i = 1; i < length; i++)
        {
            if (token[i] < '0' || token[i] > '9')
            {
                n = -1;
                break;
            }
            n = n * 10 + (token[i] - '0');
        }
        if (n >= 1 && n <= 12)
        {
            usage = (uint8_t)(0x3A + (n - 1));
            return true;
        }
        if (n >= 13 && n <= 24)
        {
            usage = (uint8_t)(0x68 + (n - 13));
            return true;
        }
    }

    for (const NamedKey& k : kNamedKeys)
    {
        if (tokenEquals(token, length, k.name))
        {
            usage = k.usage;
            return true;
        }
    }
    return false;
}

void copyField(char* dest, size_t destLen, const char* start, const char* end)
{
    size_t n = (size_t)(end - start);
    if (n >= destLen)
    {
        n = destLen - 1;
    }
    memcpy(dest, start, n);
    dest[n] = 0;
}

const char* findChar(const char* start, const char* end, char c)
{
    for (const char* p = start; p < end; p++)
    {
        if (*p == c)
        {
            return p;
        }
    }
    return nullptr;
}

} // namespace

bool parseKeySpec(const char* spec, size_t length, bool forMac, KeyCombo& out)
{
    out.key = 0;
    out.mods = 0;

    const char* p = spec;
    const char* end = spec + length;

    while (p < end)
    {
        // A token runs to the next '+', except that a lone '+'-free key like "="
        // or "-" is a single character.
        const char* tokenEnd = p;
        while (tokenEnd < end && *tokenEnd != '+')
        {
            tokenEnd++;
        }
        const char* tokenStart = p;
        const char* tokenStop = tokenEnd;
        trim(tokenStart, tokenStop);
        size_t tokenLength = (size_t)(tokenStop - tokenStart);

        if (tokenLength > 0)
        {
            if (tokenEquals(tokenStart, tokenLength, "CMD"))
            {
                out.mods |= forMac ? kModGui : kModCtrl;
            }
            else if (tokenEquals(tokenStart, tokenLength, "CTRL"))
            {
                out.mods |= kModCtrl;
            }
            else if (tokenEquals(tokenStart, tokenLength, "SHIFT"))
            {
                out.mods |= kModShift;
            }
            else if (tokenEquals(tokenStart, tokenLength, "OPT") ||
                     tokenEquals(tokenStart, tokenLength, "ALT"))
            {
                out.mods |= kModAlt;
            }
            else if (tokenEquals(tokenStart, tokenLength, "WIN") ||
                     tokenEquals(tokenStart, tokenLength, "GUI"))
            {
                out.mods |= kModGui;
            }
            else
            {
                uint8_t usage = 0;
                if (!parseKeyToken(tokenStart, tokenLength, usage))
                {
                    out.key = 0;
                    out.mods = 0;
                    return false;
                }
                out.key = usage;
            }
        }

        p = (tokenEnd < end) ? tokenEnd + 1 : end;
    }

    return !out.isEmpty();
}

int parseProfile(const char* text, size_t length, Profile& out)
{
    memset(&out, 0, sizeof(out));
    strcpy(out.name, "Profile");

    int accepted = 0;
    int layer = -1;

    const char* p = text;
    const char* end = text + length;

    while (p < end)
    {
        const char* lineEnd = p;
        while (lineEnd < end && *lineEnd != '\n')
        {
            lineEnd++;
        }

        const char* s = p;
        const char* e = lineEnd;
        trim(s, e);
        p = (lineEnd < end) ? lineEnd + 1 : end;

        if (s >= e || *s == '#')
        {
            continue;
        }

        if ((e - s) > 5 && tokenEquals(s, 4, "NAME") && isSpace(s[4]))
        {
            const char* v = s + 5;
            const char* ve = e;
            trim(v, ve);
            copyField(out.name, sizeof(out.name), v, ve);
            continue;
        }

        if ((e - s) > 6 && tokenEquals(s, 5, "LAYER") && isSpace(s[5]))
        {
            if (out.layerCount >= kMaxLayers)
            {
                layer = -1; // ignore extra layers
                continue;
            }
            layer = out.layerCount++;
            const char* v = s + 6;
            const char* ve = e;
            trim(v, ve);
            copyField(out.layers[layer].name, sizeof(out.layers[layer].name), v, ve);
            continue;
        }

        // Pad line: <number> <label> | <spec> [| <windows spec>]
        if (*s < '0' || *s > '9')
        {
            continue;
        }

        if (layer < 0)
        {
            if (out.layerCount >= kMaxLayers)
            {
                continue;
            }
            layer = out.layerCount++;
            strcpy(out.layers[layer].name, "Layer");
        }

        int padNumber = 0;
        const char* q = s;
        while (q < e && *q >= '0' && *q <= '9')
        {
            padNumber = padNumber * 10 + (*q - '0');
            q++;
        }
        if (padNumber < 1 || padNumber > kPadCount)
        {
            continue;
        }

        const char* bar1 = findChar(q, e, '|');
        if (bar1 == nullptr)
        {
            continue;
        }
        const char* bar2 = findChar(bar1 + 1, e, '|');

        const char* labelStart = q;
        const char* labelEnd = bar1;
        trim(labelStart, labelEnd);

        const char* macStart = bar1 + 1;
        const char* macEnd = (bar2 != nullptr) ? bar2 : e;
        trim(macStart, macEnd);

        const char* winStart = macStart;
        const char* winEnd = macEnd;
        if (bar2 != nullptr)
        {
            winStart = bar2 + 1;
            winEnd = e;
            trim(winStart, winEnd);
        }

        PadAction& action = out.layers[layer].pads[padNumber - 1];
        KeyCombo mac{};
        KeyCombo win{};
        bool macOk = parseKeySpec(macStart, (size_t)(macEnd - macStart), true, mac);
        bool winOk = parseKeySpec(winStart, (size_t)(winEnd - winStart), false, win);
        if (!macOk && !winOk)
        {
            continue;
        }

        copyField(action.label, sizeof(action.label), labelStart, labelEnd);
        action.mac = mac;
        action.win = win;
        accepted++;
    }

    return accepted;
}

void formatCombo(const KeyCombo& combo, bool forMac, char* buf, size_t bufLen)
{
    if (bufLen == 0)
    {
        return;
    }
    buf[0] = 0;
    if (combo.isEmpty())
    {
        return;
    }

    char keyName[8] = "";
    uint8_t k = combo.key;
    if (k >= 0x04 && k <= 0x1D)
    {
        keyName[0] = (char)('A' + (k - 0x04));
        keyName[1] = 0;
    }
    else if (k >= 0x1E && k <= 0x26)
    {
        keyName[0] = (char)('1' + (k - 0x1E));
        keyName[1] = 0;
    }
    else if (k == 0x27)
    {
        strcpy(keyName, "0");
    }
    else if (k >= 0x3A && k <= 0x45)
    {
        snprintf(keyName, sizeof(keyName), "F%d", 1 + (k - 0x3A));
    }
    else if (k >= 0x68 && k <= 0x73)
    {
        snprintf(keyName, sizeof(keyName), "F%d", 13 + (k - 0x68));
    }
    else if (forMac && k == 0x2A)
    {
        strcpy(keyName, "Delete"); // Mac labels Backspace "Delete"
    }
    else if (forMac && k == 0x4C)
    {
        strcpy(keyName, "FwdDel");
    }
    else
    {
        for (const NamedKey& d : kDisplayNames)
        {
            if (d.usage == k)
            {
                strncpy(keyName, d.name, sizeof(keyName) - 1);
                keyName[sizeof(keyName) - 1] = 0;
                break;
            }
        }
    }

    snprintf(buf,
             bufLen,
             "%s%s%s%s%s",
             (combo.mods & kModCtrl) ? "Ctl+" : "",
             (combo.mods & kModAlt) ? (forMac ? "Opt+" : "Alt+") : "",
             (combo.mods & kModShift) ? "Sh+" : "",
             (combo.mods & kModGui) ? (forMac ? "Cmd+" : "Win+") : "",
             keyName);

    // Drop a trailing '+' for modifier-only combos.
    size_t n = strlen(buf);
    if (n > 0 && buf[n - 1] == '+')
    {
        buf[n - 1] = 0;
    }
}

} // namespace i2ctl
