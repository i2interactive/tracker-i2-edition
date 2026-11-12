#include "i2QuizGame.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Timings and scoring follow kquiz.js, which runs at 60 frames per second.
// This port steps the show at a fixed 60 Hz too, so the frame counts match.

namespace i2arc
{
namespace
{
constexpr uint32_t kFrameMs = 1000 / 60;

struct CatDef
{
    const char* key;
    const char* name;
};
const CatDef kCats[] = {
    { "SCI", "SCIENCE" },        { "NAT", "NATURE" },           { "GEO", "GEOGRAPHY" },
    { "MATH", "MATHS" },         { "FOOD", "FOOD" },            { "LANG", "LANGUAGE" },
    { "SPACE", "SPACE" },        { "HOW", "HOW IT WORKS" },     { "I2", "I2 WORLD" },
    { "VINT", "VINTAGE" },       { "NOW", "NOW" },              { "HIST", "HISTORY" },
    { "BODY", "HUMAN BODY" },    { "ART", "ARTS AND MUSIC" },   { "SPORT", "SPORTS AND GAMES" },
    { "WORD", "WORDPLAY" },      { "EARTH", "EARTH AND WEATHER" }, { "ANIM", "ANIMALS" },
    { "TIME", "TIME AND CALENDAR" },
};
constexpr int kCatCount = sizeof(kCats) / sizeof(kCats[0]);
constexpr int kCatI2 = 8, kCatVint = 9, kCatNow = 10;
// CATEGORY BLITZ draws from these (kquiz.js KQ_BLITZ)
const uint8_t kBlitz[] = { 0, 1, 2, 3, 4, 5, 6, 7, 11, 12, 13, 14, 15, 16, 17, 18 };

struct CpuDef
{
    const char* name;
    uint32_t col;
    float acc, spd;
};
const CpuDef kCpus[] = {
    { "PROF PIP", 0x3A9A58, .72f, 150 },
    { "DEE-DEE", 0xE8C020, .5f, 70 },
    { "BIG BORO", 0xE07020, .6f, 110 },
    { "NANA SUMI", 0x9858C8, .64f, 130 },
};

// Answer row colours (kquiz.js KQ_SLOT): row 1 red, 2 blue, 3 yellow, 4 green
const uint32_t kSlot[4] = { 0xE83838, 0x3878E8, 0xF0C020, 0x30C050 };

const char* const kSayWelcome = "WELCOME TO BRAIN BANZAI! I AM YOUR HOST, MR. MARU! THREE BRAINS, ONE BUZZER, ONE WINNER. LET US MEET THE CONTESTANTS!";
const char* const kSayBuzz[] = { "ROUND ONE: THE BUZZER ROUND! HIT YOUR BUZZER EARLY FOR DOUBLE POINTS, BUT WATCH OUT FOR WRONG ANSWERS!",
                                 "FINGERS ON BUZZERS! BUZZ BEFORE I FINISH READING AND IT IS DOUBLE!" };
const char* const kSayBonus[] = { "A BONUS ROUND! FRESH QUESTIONS FROM THE KAGEMORI VAULT!" };
const char* const kSayFast[] = { "FAST FINGER ROUND! PUT THE FOUR IN ORDER. FIRST CORRECT WINS THE MOST. A WRONG PRESS STARTS YOU OVER!",
                                 "FASTEST FINGER FIRST! GO, GO, GO!" };
const char* const kSayFinal[] = { "THE FINAL WAGER! THE TRAILING CONTESTANT PICKS THE CATEGORY. THEN BET YOUR POINTS. RIGHT WINS THEM, WRONG LOSES THEM!" };
const char* const kSayOdd[] = { "ODD ONE OUT! FOUR THINGS, ONE DOES NOT BELONG. EVERYBODY ANSWERS, AND FASTER SCORES MORE!",
                                "SPOT THE STRANGER IN THE GROUP!" };
const char* const kSayTrue[] = { "TRUE STORY! THREE OF THESE ARE TALL TALES. ONLY ONE IS TRUE. CAN YOU TELL WHICH?",
                                 "FACT OR FIB? ONLY ONE IS THE TRUTH!" };
const char* const kSayBlitz[] = { "CATEGORY BLITZ! FIVE QUICK ONES FROM ONE CATEGORY, AND THE CLOCK RUNS FAST!",
                                  "BLITZ TIME! QUICK, QUICK, QUICK!" };
const char* const kSayRight[] = { "CORRECT! WHAT A BRAIN!", "YES! THAT IS THE ONE!", "BANZAI! SPOT ON!",
                                  "RIGHT YOU ARE!", "DING DING DING! YES!", "ABSOLUTELY CORRECT!" };
const char* const kSayWrong[] = { "OH, SO CLOSE! BUT NO.", "OUCH! NOT QUITE.", "THE BUZZER DOES NOT LIE!",
                                  "OH DEAR. BETTER LUCK NEXT TIME.", "NOPE! BUT A GOOD TRY." };
const char* const kSayNobody[] = { "NOBODY? NO WORRIES, THE ANSWER IS UP THERE.", "TOO SLOW, TOO SLOW!", "THE CLOCK RAN OUT!" };

template <size_t N> const char* pickLine(Services& sv, const char* const (&a)[N])
{
    return a[irand(sv, (int)N)];
}

const char* roundName(uint8_t t, uint8_t cat, char* buf, size_t n)
{
    switch (t)
    {
        case 0: return "BUZZER ROUND";
        case 1: return "BONUS ROUND";
        case 2: return "ODD ONE OUT";
        case 3: return "TRUE STORY";
        case 4: snprintf(buf, n, "BLITZ: %s", kCats[cat].name); return buf;
        case 5: return "FAST FINGER";
        case 6: return "FINAL WAGER";
        default: return "";
    }
}

int catIndex(const char* key)
{
    for (int i = 0; i < kCatCount; i++)
    {
        if (strcmp(kCats[i].key, key) == 0)
        {
            return i;
        }
    }
    return -1;
}

const char* kSavePath = "i2/Arcade/scores_quiz.txt";

// Studio colours
constexpr uint32_t kInk = 0x140A24;
constexpr uint32_t kPanel = 0x101A58;
constexpr uint32_t kPanelEdge = 0x6A8AE8;
constexpr uint32_t kGold = 0xFFD040;
constexpr uint32_t kLilac = 0xC0B0FF;

// Layout (screen pixels)
constexpr int16_t kHeaderY = kCanvasY + 4;
constexpr int16_t kPanelY0 = kCanvasY + 40, kPanelY1 = kCanvasY + 140;
constexpr int16_t kChoiceY0 = kCanvasY + 148, kChoiceH = 40, kChoiceGap = 5;
constexpr int16_t kPodY0 = kCanvasY + 326, kPodY1 = kCanvasY + kCanvasH - 2;
} // namespace

//==============================================================================================================
// Data: the bank is four pipe-separated text files on the SD card
//==============================================================================================================
int QuizGame::splitFields(char* line, char** out, int maxFields)
{
    int n = 0;
    out[n++] = line;
    for (char* p = line; *p && n < maxFields; p++)
    {
        if (*p == '|')
        {
            *p = 0;
            out[n++] = p + 1;
        }
    }
    return n;
}

bool QuizGame::loadBank()
{
    size_t scratchSize = 0;
    uint8_t* mem = sv->scratch(scratchSize);
    if (mem == nullptr || scratchSize < 256 * 1024)
    {
        return false;
    }

    // Layout: [text files ... | tables]
    char* text = (char*)mem;
    size_t textCap = 200 * 1024;
    size_t used = 0;

    const char* files[4] = { "i2/Arcade/Quiz/questions.txt", "i2/Arcade/Quiz/order.txt",
                             "i2/Arcade/Quiz/odd.txt", "i2/Arcade/Quiz/true.txt" };
    char* fileText[4] = {};
    int32_t fileLen[4] = {};
    for (int f = 0; f < 4; f++)
    {
        fileText[f] = text + used;
        int32_t n = sv->loadText(files[f], fileText[f], textCap - used - 1);
        if (n < 0)
        {
            return false;
        }
        fileText[f][n] = 0;
        fileLen[f] = n;
        used += (size_t)n + 1;
        used = (used + 3) & ~(size_t)3;
    }

    // Tables after the text
    uint8_t* tab = mem + textCap;
    size_t tabCap = scratchSize - textCap;
    auto alloc = [&](size_t bytes) -> void* {
        bytes = (bytes + 3) & ~(size_t)3;
        if (bytes > tabCap)
        {
            return nullptr;
        }
        void* p = tab;
        tab += bytes;
        tabCap -= bytes;
        return p;
    };

    // Count lines per file to size the tables
    auto countLines = [](const char* s) {
        int n = 0;
        bool any = false;
        for (; *s; s++)
        {
            if (*s == '\n')
            {
                if (any) n++;
                any = false;
            }
            else if (*s != '\r')
            {
                any = true;
            }
        }
        return n + (any ? 1 : 0);
    };

    int cap[4];
    for (int f = 0; f < 4; f++)
    {
        cap[f] = countLines(fileText[f]);
    }

    bank = (TextQ*)alloc(sizeof(TextQ) * cap[0]);
    ords = (OrderQ*)alloc(sizeof(OrderQ) * cap[1]);
    odds = (OddQ*)alloc(sizeof(OddQ) * cap[2]);
    trues = (TrueQ*)alloc(sizeof(TrueQ) * cap[3]);
    usedText = (uint8_t*)alloc(cap[0] + 1);
    usedOrd = (uint8_t*)alloc(cap[1] + 1);
    usedOdd = (uint8_t*)alloc(cap[2] + 1);
    usedTrue = (uint8_t*)alloc(cap[3] + 1);
    if (!bank || !ords || !odds || !trues || !usedText || !usedOrd || !usedOdd || !usedTrue)
    {
        return false;
    }

    bankCount = ordCount = oddCount = trueCount = 0;
    for (int f = 0; f < 4; f++)
    {
        char* p = fileText[f];
        char* end = p + fileLen[f];
        while (p < end)
        {
            char* nl = p;
            while (nl < end && *nl != '\n')
            {
                nl++;
            }
            *nl = 0;
            if (nl > p && *(nl - 1) == '\r')
            {
                *(nl - 1) = 0;
            }
            char* fields[8];
            int n = (*p != 0 && *p != '#') ? splitFields(p, fields, 8) : 0;
            if (f == 0 && n >= 6 && bankCount < cap[0])
            {
                TextQ& t = bank[bankCount++];
                t.cat = fields[0];
                t.q = fields[1];
                for (int k = 0; k < 4; k++) t.a[k] = fields[2 + k];
            }
            else if (f == 1 && n >= 5 && ordCount < cap[1])
            {
                OrderQ& o = ords[ordCount++];
                o.q = fields[0];
                for (int k = 0; k < 4; k++) o.a[k] = fields[1 + k];
            }
            else if (f == 2 && n >= 5 && oddCount < cap[2])
            {
                OddQ& o = odds[oddCount++];
                for (int k = 0; k < 4; k++) o.a[k] = fields[k];
                o.why = fields[4];
            }
            else if (f == 3 && n >= 4 && trueCount < cap[3])
            {
                TrueQ& t = trues[trueCount++];
                for (int k = 0; k < 4; k++) t.a[k] = fields[k];
            }
            p = nl + 1;
        }
    }

    memset(usedText, 0, cap[0] + 1);
    memset(usedOrd, 0, cap[1] + 1);
    memset(usedOdd, 0, cap[2] + 1);
    memset(usedTrue, 0, cap[3] + 1);

    return bankCount >= 20 && ordCount >= 2 && oddCount >= 3 && trueCount >= 3;
}

void QuizGame::shuffle(int8_t* idx, int n)
{
    for (int i = n - 1; i > 0; i--)
    {
        int j = irand(*sv, i + 1);
        int8_t t = idx[i];
        idx[i] = idx[j];
        idx[j] = t;
    }
}

//==============================================================================================================
// Saves
//==============================================================================================================
void QuizGame::loadSave()
{
    char buf[256];
    int32_t n = sv->loadText(kSavePath, buf, sizeof(buf) - 1);
    if (n <= 0)
    {
        return;
    }
    buf[n] = 0;
    const char* b = strstr(buf, "best=");
    if (b) best = (int32_t)strtol(b + 5, nullptr, 10);
    const char* w = strstr(buf, "wins=");
    if (w) wins = (int32_t)strtol(w + 5, nullptr, 10);
    const char* p = strstr(buf, "plays=");
    if (p) plays = (int32_t)strtol(p + 6, nullptr, 10);
}

void QuizGame::persist()
{
    char buf[96];
    int n = snprintf(buf, sizeof(buf), "best=%ld\nwins=%ld\nplays=%ld\n", (long)best, (long)wins, (long)plays);
    if (n > 0)
    {
        sv->saveText(kSavePath, buf, (size_t)n);
    }
}

//==============================================================================================================
// Lifecycle and input
//==============================================================================================================
void QuizGame::begin(Services& services)
{
    sv = &services;
    exitRequested = false;
    lastMs = 0;
    accMs = 0;
    frame = 0;
    dirtyMask = 3;
    rowPressed = -1;
    confirmPressed = llPressed = false;
    jogSteps = 0;
    best = wins = plays = 0;
    loadSave();
    bankOk = loadBank();
    ph = bankOk ? phTitle : phMissing;
    pt = 0;
    refreshPadLights();
}

void QuizGame::pad(uint8_t pad, bool down)
{
    uint8_t row = pad / 12;
    if (row > 3)
    {
        return;
    }
    if (down)
    {
        rowHeld[row]++;
        rowPressed = (int8_t)row;
    }
    else if (rowHeld[row] > 0)
    {
        rowHeld[row]--;
    }
    refreshPadLights();
}

void QuizGame::button(Button b, bool down)
{
    if (b == Button::Shift)
    {
        shiftHeld = down;
        return;
    }
    if (!down)
    {
        return;
    }
    switch (b)
    {
        case Button::Play: confirmPressed = true; break;
        case Button::Screen0:
            if (ph == phTitle || ph == phOver || ph == phMissing)
            {
                confirmPressed = true;
            }
            else
            {
                llPressed = true; // 50:50
            }
            break;
        case Button::Up: jogSteps += 1; break;
        case Button::Down: jogSteps -= 1; break;
        case Button::Right: jogSteps += 5; break;
        case Button::Left: jogSteps -= 5; break;
        default: break;
    }
}

void QuizGame::jog(int16_t steps)
{
    jogSteps += shiftHeld ? steps * 5 : steps;
}

const char* QuizGame::buttonLabel(uint8_t i) const
{
    if (i == 0)
    {
        if (ph == phTitle || ph == phOver)
        {
            return "Start";
        }
        if (ph == phMissing)
        {
            return "Retry";
        }
        return P[0].ll ? "50:50" : "50:50 used";
    }
    return nullptr;
}

bool QuizGame::takeConfirm()
{
    bool c = confirmPressed || rowPressed >= 0;
    confirmPressed = false;
    rowPressed = -1;
    return c;
}

int QuizGame::takeRow()
{
    int r = rowPressed;
    rowPressed = -1;
    return r;
}

bool QuizGame::skipPressed()
{
    return takeConfirm();
}

void QuizGame::refreshPadLights()
{
    if (sv == nullptr)
    {
        return;
    }
    // Light the rows that can be pressed right now.
    bool rowsLive = false;
    bool rowLive[4] = { false, false, false, false };
    switch (ph)
    {
        case phPick:
        case phAnswer:
        case phRead:
            rowsLive = true;
            for (int r = 0; r < 4; r++)
            {
                rowLive[r] = !elim[r] && !P[0].el[r];
            }
            break;
        case phFCat:
            rowsLive = true;
            rowLive[0] = rowLive[1] = rowLive[2] = true;
            break;
        default: break;
    }
    for (uint8_t pad = 0; pad < 48; pad++)
    {
        uint8_t row = pad / 12;
        uint8_t level = 0;
        if (rowsLive && rowLive[row])
        {
            level = rowHeld[row] ? 2 : 1;
        }
        else if (!rowsLive && (ph == phTitle || ph == phOver || ph == phWager))
        {
            level = 1;
        }
        sv->padLight(pad, level);
    }
}

void QuizGame::tick(uint32_t nowMs)
{
    if (lastMs == 0)
    {
        lastMs = nowMs;
    }
    uint32_t dt = nowMs - lastMs;
    lastMs = nowMs;
    if (dt > 200)
    {
        dt = 200;
    }
    accMs += dt;

    Phase before = ph;
    bool titleBefore = ph == phTitle || ph == phMissing || ph == phOver;
    int steps = 0;
    while (accMs >= kFrameMs && steps < 6)
    {
        accMs -= kFrameMs;
        step();
        steps++;
    }
    if (steps > 0)
    {
        dirtyMask |= 2;
    }
    if (ph != before)
    {
        refreshPadLights();
        bool titleNow = ph == phTitle || ph == phMissing || ph == phOver;
        if (titleNow != titleBefore)
        {
            dirtyMask |= 1;
        }
    }
}

//==============================================================================================================
// The show (ported from kquiz.js)
//==============================================================================================================
void QuizGame::say(const char* txt)
{
    strncpy(sayText, txt, sizeof(sayText) - 1);
    sayText[sizeof(sayText) - 1] = 0;
    sayT = 0;
}

void QuizGame::sayf(const char* fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(sayText, sizeof(sayText), fmt, ap);
    va_end(ap);
    sayT = 0;
}

void QuizGame::popOn(Player& p, const char* txt, uint32_t col)
{
    strncpy(p.pop, txt, sizeof(p.pop) - 1);
    p.pop[sizeof(p.pop) - 1] = 0;
    p.popT = 70;
    p.popCol = col;
}

void QuizGame::newShow()
{
    // You + three CPU contestants, picked at random
    int8_t order[4] = { 0, 1, 2, 3 };
    shuffle(order, 4);
    np = 4;
    memset(P, 0, sizeof(P));
    P[0].name = "YOU";
    P[0].col = 0xD83838;
    P[0].cpu = false;
    for (int i = 1; i < 4; i++)
    {
        const CpuDef& c = kCpus[order[i - 1]];
        P[i].name = c.name;
        P[i].col = c.col;
        P[i].cpu = true;
        P[i].acc = c.acc;
        P[i].spd = c.spd;
    }
    for (int i = 0; i < 4; i++)
    {
        P[i].score = 0;
        P[i].streak = 0;
        P[i].ll = true;
        P[i].lock = -1;
        P[i].fin = -1;
        P[i].ex = 'n';
    }

    // Plan: two of the three new rounds, in random order (kquiz.js, Nov 6)
    int8_t ex[3] = { 0, 1, 2 }; // odd, true, blitz
    shuffle(ex, 3);
    auto xr = [&](int8_t e) {
        Round r{};
        if (e == 2)
        {
            r.t = rtBlitz;
            r.n = 5;
            r.cat = kBlitz[irand(*sv, (int)sizeof(kBlitz))];
        }
        else
        {
            r.t = e == 0 ? rtOdd : rtTrue;
            r.n = 3;
        }
        return r;
    };
    planLen = 0;
    plan[planLen++] = Round{ rtBuzz, 4, 0 };
    plan[planLen++] = xr(ex[0]);
    plan[planLen++] = Round{ rtBonus, 3, 0 };
    plan[planLen++] = xr(ex[1]);
    plan[planLen++] = Round{ rtFast, 2, 0 };
    plan[planLen++] = Round{ rtFinal, 1, 0 };

    ri = -1;
    qi = -1;
    qn = 0;
    skill = 1.0f;
    flash = 0;
    got = 0;
    winner = -1;
    buz = -1;
    plays++;
    persist();
    say(kSayWelcome);
    ph = phIntro;
    pt = 0;
}

int QuizGame::pickTextQ(int cat, bool /*preferVintNow*/)
{
    // Unasked question from the category (or any category when cat < 0).
    int candidates = 0;
    for (int pass = 0; pass < 3; pass++)
    {
        candidates = 0;
        for (int i = 0; i < bankCount; i++)
        {
            bool catOk = cat < 0 || catIndex(bank[i].cat) == cat;
            bool fresh = pass == 1 || !usedText[i];
            if (catOk && fresh)
            {
                candidates++;
            }
        }
        if (candidates > 0)
        {
            int pickN = irand(*sv, candidates);
            for (int i = 0; i < bankCount; i++)
            {
                bool catOk = cat < 0 || catIndex(bank[i].cat) == cat;
                bool fresh = pass == 1 || !usedText[i];
                if (catOk && fresh && pickN-- == 0)
                {
                    usedText[i] = 1;
                    return i;
                }
            }
        }
        if (pass == 1)
        {
            // the whole bank (or category) has been asked: start it over
            memset(usedText, 0, (size_t)bankCount);
        }
    }
    return irand(*sv, bankCount);
}

void QuizGame::makeTextQ(int bi)
{
    Question& q = qs[qn];
    const TextQ& t = bank[bi];
    int ci = catIndex(t.cat);
    q.kind = kText;
    q.cat = ci >= 0 ? kCats[ci].name : t.cat;
    q.q = t.q;
    int8_t idx[4] = { 0, 1, 2, 3 };
    shuffle(idx, 4);
    for (int k = 0; k < 4; k++)
    {
        q.ch[k] = t.a[idx[k]];
        if (idx[k] == 0)
        {
            q.ans = (int8_t)k;
        }
    }
    q.oddIndex = -1;
}

void QuizGame::beginRound()
{
    ri++;
    if (ri >= planLen)
    {
        finish();
        return;
    }
    Round& R = plan[ri];
    qi = -1;
    qn = 0;
    for (int i = 0; i < np; i++)
    {
        P[i].ex = 'n';
    }

    switch (R.t)
    {
        case rtBuzz:
        case rtBonus:
        {
            // Buzzer: VINTAGE and NOW always represented; bonus: I2 WORLD questions
            int want[6];
            int nw = 0;
            if (R.t == rtBuzz)
            {
                want[nw++] = kCatVint;
                want[nw++] = kCatNow;
            }
            else
            {
                for (int k = 0; k < R.n; k++) want[nw++] = kCatI2;
            }
            for (int k = 0; k < R.n && qn < 6; k++)
            {
                int bi = pickTextQ(k < nw ? want[k] : -1, false);
                makeTextQ(bi);
                qn++;
            }
            // shuffle question order
            for (int i = qn - 1; i > 0; i--)
            {
                int j = irand(*sv, i + 1);
                Question tmp = qs[i];
                qs[i] = qs[j];
                qs[j] = tmp;
            }
            break;
        }
        case rtBlitz:
            for (int k = 0; k < R.n && qn < 6; k++)
            {
                makeTextQ(pickTextQ(R.cat, false));
                qs[qn].kind = kChoose;
                qn++;
            }
            break;
        case rtOdd:
        case rtTrue:
        {
            int count = R.t == rtOdd ? oddCount : trueCount;
            uint8_t* used = R.t == rtOdd ? usedOdd : usedTrue;
            int fresh = 0;
            for (int i = 0; i < count; i++) fresh += !used[i];
            if (fresh < R.n)
            {
                memset(used, 0, (size_t)count);
            }
            for (int k = 0; k < R.n && qn < 6; k++)
            {
                int pick = 0;
                for (int guard = 0; guard < 200; guard++)
                {
                    pick = irand(*sv, count);
                    if (!used[pick]) break;
                }
                used[pick] = 1;
                Question& q = qs[qn];
                q.kind = kChoose;
                int8_t idx[4] = { 0, 1, 2, 3 };
                shuffle(idx, 4);
                const char* const* src = R.t == rtOdd ? odds[pick].a : trues[pick].a;
                for (int j = 0; j < 4; j++)
                {
                    q.ch[j] = src[idx[j]];
                    if (idx[j] == 0) q.ans = (int8_t)j;
                }
                if (R.t == rtOdd)
                {
                    q.cat = "ODD ONE OUT";
                    q.q = "WHICH IS THE ODD ONE OUT?";
                    q.oddIndex = (int16_t)pick;
                }
                else
                {
                    q.cat = "TRUE STORY";
                    q.q = "ONLY ONE OF THESE IS TRUE. WHICH ONE?";
                    q.oddIndex = -1;
                }
                qn++;
            }
            break;
        }
        case rtFast:
            for (int k = 0; k < R.n && qn < 6; k++)
            {
                int pick = 0;
                for (int guard = 0; guard < 200; guard++)
                {
                    pick = irand(*sv, ordCount);
                    if (!usedOrd[pick]) break;
                }
                usedOrd[pick] = 1;
                Question& q = qs[qn];
                q.kind = kOrder;
                q.cat = "FAST FINGER";
                q.q = ords[pick].q;
                int8_t idx[4] = { 0, 1, 2, 3 };
                shuffle(idx, 4);
                for (int j = 0; j < 4; j++)
                {
                    q.ch[j] = ords[pick].a[idx[j]];
                }
                // order[n] = which shown choice is the n-th in the right order
                for (int n = 0; n < 4; n++)
                {
                    for (int j = 0; j < 4; j++)
                    {
                        if (idx[j] == n) q.order[n] = (int8_t)j;
                    }
                }
                q.ans = -1;
                q.oddIndex = -1;
                qn++;
            }
            break;
        case rtFinal:
        {
            // the special category plus two others
            const int specs[3] = { kCatVint, kCatNow, kCatI2 };
            int spec = specs[irand(*sv, 3)];
            fc[0] = (uint8_t)spec;
            for (int k = 1; k < 3; k++)
            {
                int c;
                bool dup;
                do
                {
                    c = irand(*sv, kCatCount);
                    dup = c == spec;
                    for (int m = 1; m < k; m++) dup = dup || fc[m] == c;
                } while (dup);
                fc[k] = (uint8_t)c;
            }
            int8_t idx[3] = { 0, 1, 2 };
            shuffle(idx, 3);
            uint8_t tmp[3] = { fc[idx[0]], fc[idx[1]], fc[idx[2]] };
            memcpy(fc, tmp, 3);
            break;
        }
    }

    ph = phRIntro;
    pt = 0;
    switch (R.t)
    {
        case rtBuzz: say(pickLine(*sv, kSayBuzz)); break;
        case rtBonus: say(pickLine(*sv, kSayBonus)); break;
        case rtOdd: say(pickLine(*sv, kSayOdd)); break;
        case rtTrue: say(pickLine(*sv, kSayTrue)); break;
        case rtBlitz: say(pickLine(*sv, kSayBlitz)); break;
        case rtFast: say(pickLine(*sv, kSayFast)); break;
        case rtFinal: say(kSayFinal[0]); break;
    }
    sv->sfx(Sfx::Select);
}

void QuizGame::nextQ()
{
    if (plan[ri].t == rtFinal)
    {
        if (ph == phRIntro)
        {
            beginFinal();
            return;
        }
        finish();
        return;
    }
    qi++;
    if (qi >= qn)
    {
        beginRound();
        return;
    }
    Q = qs[qi];
    beginQ();
}

void QuizGame::planCpu(Player& p)
{
    float acc = p.acc * skill;
    if (acc > .96f) acc = .96f;
    bool kn = p.know = frand(*sv) < acc;
    p.pick = kn ? Q.ans : (int8_t)irand(*sv, 4);
    if (!kn && Q.kind != kOrder && frand(*sv) < .5f)
    {
        p.pick = (int8_t)((Q.ans + 1 + irand(*sv, 3)) % 4);
    }
    const float inf = 1e9f;
    if (ph == phRead)
    {
        float rd = strlen(Q.q) / .6f;
        p.tb = kn ? 18 + frand(*sv) * p.spd * 1.3f : (frand(*sv) < .35f ? rd + 20 + frand(*sv) * 220 : inf);
        if (kn && p.tb < rd * .5f && frand(*sv) < .15f)
        {
            p.know = false;
            p.pick = (int8_t)((Q.ans + 1) % 4);
        }
        p.pdelay = 40 + frand(*sv) * 70;
    }
    else if (Q.kind == kOrder)
    {
        p.tf = kn ? 200 + frand(*sv) * p.spd * 4 : inf;
    }
    else
    {
        p.tl = kn ? 90 + frand(*sv) * p.spd * 3.2f : (frand(*sv) < .7f ? 200 + frand(*sv) * 500 : inf);
    }
}

void QuizGame::beginQ()
{
    pt = 0;
    memset(elim, 0, sizeof(elim));
    buz = -1;
    rt = 0;
    bool fin = plan[ri].t == rtFinal;
    cOn = Q.kind != kText || fin;
    cOnT = 0;
    early = false;
    winner = -1;
    sayText[0] = 0;
    got = 0;
    ph = (Q.kind == kText && !fin) ? phRead : phRead2;
    lim = Q.kind == kOrder ? 1300 : Q.kind == kChoose ? (plan[ri].t == rtBlitz ? 720 : 1100) : 1500;
    for (int i = 0; i < np; i++)
    {
        Player& p = P[i];
        p.lock = -1;
        p.lockT = 0;
        p.out = false;
        memset(p.el, 0, sizeof(p.el));
        p.prog = 0;
        p.frz = 0;
        p.fin = -1;
        p.know = false;
        p.ex = 'n';
        if (p.cpu)
        {
            planCpu(p);
        }
    }
    if (Q.kind == kText && fin)
    {
        for (int i = 0; i < np; i++)
        {
            if (P[i].cpu) P[i].tl = (P[i].know ? 150 : 250) + frand(*sv) * P[i].spd * 4;
        }
    }
    sv->sfx(Sfx::Select);
}

void QuizGame::beginFinal()
{
    int low = 0;
    for (int i = 1; i < np; i++)
    {
        if (P[i].score < P[low].score) low = i;
    }
    chooser = (int8_t)low;
    fcur = 1;
    ph = phFCat;
    pt = 0;
    sayf("%s %s IN LAST PLACE, SO %s PICK THE CATEGORY!", P[low].name, P[low].cpu ? "IS" : "ARE",
         P[low].cpu ? "THEY" : "YOU");
}

void QuizGame::pickCat(int ci)
{
    fcat = fc[ci];
    qn = 0;
    makeTextQ(pickTextQ(fcat, false));
    qn = 1;
    qi = 0;
    Q = qs[0];
    ph = phWager;
    pt = 0;
    sayf("THE CATEGORY IS %s! PLACE YOUR WAGERS!", kCats[fcat].name);
    sv->sfx(Sfx::Confirm);
    for (int i = 0; i < np; i++)
    {
        Player& p = P[i];
        p.lock = -1;
        p.ex = 'n';
        int32_t cap = p.score > 100 ? p.score : 100;
        p.cap = cap;
        int32_t half = (int32_t)((cap / 2.0f / 50.0f) + .5f) * 50;
        p.bet = half > cap ? cap : (half > 0 ? half : 50);
        p.wl = false;
        if (p.cpu)
        {
            float r = .2f + p.acc * .55f + (frand(*sv) - .5f) * .25f;
            int32_t b = (int32_t)((cap * r / 50.0f) + .5f) * 50;
            if (b < 50) b = 50;
            if (b > cap) b = cap;
            p.bet = b;
            p.wt = 60 + frand(*sv) * 200;
        }
    }
}

bool QuizGame::lockAns(Player& p, int i, int32_t t)
{
    if (p.lock >= 0 || p.out || i < 0 || i > 3 || elim[i] || p.el[i])
    {
        return false;
    }
    p.lock = (int8_t)i;
    p.lockT = t < 0 ? pt : t;
    if (!p.cpu)
    {
        sv->sfx(Sfx::Confirm);
    }
    return true;
}

void QuizGame::ordPress(Player& p, int i)
{
    if (p.fin >= 0 || p.frz > 0)
    {
        return;
    }
    if (Q.order[p.prog] == i)
    {
        p.prog++;
        sv->sfx(Sfx::Confirm);
        if (p.prog >= 4)
        {
            p.fin = pt;
            sv->sfx(Sfx::Right);
        }
    }
    else
    {
        p.prog = 0;
        p.frz = 40;
        sv->sfx(Sfx::Wrong);
        p.ex = 'l';
    }
}

void QuizGame::useLL(Player& p)
{
    if (!p.ll || Q.kind == kOrder)
    {
        return;
    }
    int8_t w[4];
    int nw = 0;
    for (int i = 0; i < 4; i++)
    {
        if (i != Q.ans && !elim[i] && !p.el[i]) w[nw++] = (int8_t)i;
    }
    shuffle(w, nw);
    for (int k = 0; k < 2 && k < nw; k++)
    {
        p.el[w[k]] = true;
    }
    p.ll = false;
    sv->sfx(Sfx::Confirm);
    popOn(p, "50:50", 0x80E0FF);
}

void QuizGame::buzzIn(Player& p)
{
    buz = (int8_t)(&p - P);
    early = rt < strlen(Q.q) / .6f;
    cOn = true;
    ph = phPick;
    pt = 0;
    cd = 420;
    p.ex = 't';
    sv->sfx(Sfx::Buzz);
    popOn(p, early ? "EARLY!" : "BUZZ!", 0xFFFFFF);
    if (p.cpu)
    {
        p.pdelay = 40 + frand(*sv) * 70;
        int k = p.pick;
        if (elim[k])
        {
            for (int i = 0; i < 4; i++)
            {
                if (!elim[i])
                {
                    k = i;
                    break;
                }
            }
        }
        p.pick = (int8_t)k;
    }
}

void QuizGame::award(Player& p, int32_t pts)
{
    p.score += pts;
    if (p.score < 0) p.score = 0;
    char b[20];
    snprintf(b, sizeof(b), "%s%ld", pts >= 0 ? "+" : "", (long)pts);
    popOn(p, b, pts >= 0 ? kGold : 0xFF7070);
}

void QuizGame::correct(Player& p, int32_t pts)
{
    p.streak++;
    int32_t b = (p.streak - 1) * 50;
    if (b < 0) b = 0;
    if (b > 150) b = 150;
    award(p, pts + b);
    p.ex = 'w';
    if (!p.cpu)
    {
        got++;
    }
    if (b)
    {
        char s[20];
        snprintf(s, sizeof(s), "STREAK +%ld", (long)b);
        popOn(p, s, 0x80FF90);
    }
}

void QuizGame::resolvePick(Player& p, int i)
{
    if (i == Q.ans)
    {
        correct(p, early ? 200 : 100);
        winner = (int8_t)(&p - P);
        sv->sfx(Sfx::Right);
        toReveal();
        return;
    }
    p.streak = 0;
    p.out = true;
    p.ex = 'l';
    award(p, -50);
    if (i >= 0) elim[i] = true;
    sv->sfx(Sfx::Wrong);
    int left = 0, ch = 0;
    for (int k = 0; k < np; k++) left += !P[k].out;
    for (int k = 0; k < 4; k++) ch += !elim[k];
    if (left == 0 || ch <= 1)
    {
        toReveal();
        return;
    }
    // reopen the question for the others
    ph = phRead;
    buz = -1;
    cOn = true;
    cOnT = 200;
    for (int k = 0; k < np; k++)
    {
        Player& q = P[k];
        if (!q.cpu || q.out) continue;
        q.tb = q.know ? rt + 30 + frand(*sv) * q.spd : (frand(*sv) < .25f ? rt + 100 + frand(*sv) * 200 : 1e9f);
        if (q.know)
        {
            q.pick = Q.ans;
        }
        else
        {
            int8_t open[4];
            int no = 0;
            for (int m = 0; m < 4; m++)
            {
                if (!elim[m]) open[no++] = (int8_t)m;
            }
            q.pick = open[irand(*sv, no)];
        }
    }
}

void QuizGame::settle()
{
    bool fin = plan[ri].t == rtFinal;
    if (Q.kind == kOrder)
    {
        // rank finishers by time
        int8_t done[4];
        int nd = 0;
        for (int i = 0; i < np; i++)
        {
            if (P[i].fin >= 0) done[nd++] = (int8_t)i;
        }
        for (int a = 0; a < nd; a++)
        {
            for (int b = a + 1; b < nd; b++)
            {
                if (P[done[b]].fin < P[done[a]].fin)
                {
                    int8_t t = done[a];
                    done[a] = done[b];
                    done[b] = t;
                }
            }
        }
        const int32_t pts[4] = { 300, 200, 100, 50 };
        for (int k = 0; k < nd; k++) correct(P[done[k]], pts[k]);
        for (int i = 0; i < np; i++)
        {
            if (P[i].fin < 0)
            {
                P[i].streak = 0;
                P[i].ex = 'l';
            }
        }
        if (nd > 0) winner = done[0];
    }
    else
    {
        for (int i = 0; i < np; i++)
        {
            Player& p = P[i];
            bool ok = p.lock == Q.ans;
            if (fin)
            {
                if (ok)
                {
                    award(p, p.bet);
                    p.ex = 'w';
                    if (!p.cpu) got++;
                }
                else
                {
                    award(p, -p.bet);
                    p.ex = 'l';
                }
                continue;
            }
            if (p.lock < 0)
            {
                p.streak = 0;
                continue;
            }
            if (ok)
            {
                int32_t pts = 100 + (int32_t)((1.0f - (float)p.lockT / lim) * 10 + .5f) * 10;
                correct(p, pts);
            }
            else
            {
                p.streak = 0;
                p.ex = 'l';
                award(p, -50);
            }
        }
    }
    toReveal();
}

void QuizGame::toReveal()
{
    ph = phReveal;
    pt = 0;
    bool anyH = false, gotH = false, anyRight = false, anyTried = false;
    for (int i = 0; i < np; i++)
    {
        Player& p = P[i];
        if (p.ex != 'w') p.streak = 0;
        if (!p.cpu)
        {
            anyH = true;
            gotH = gotH || p.ex == 'w';
        }
        anyRight = anyRight || p.ex == 'w';
        anyTried = anyTried || p.out || p.lock >= 0;
    }
    (void)gotH;
    if (winner >= 0 || anyRight)
    {
        say(pickLine(*sv, kSayRight));
    }
    else
    {
        say(anyH && anyTried ? pickLine(*sv, kSayWrong) : pickLine(*sv, kSayNobody));
    }
    if (Q.oddIndex >= 0 && Q.oddIndex < oddCount)
    {
        // "THE ODD ONE OUT IS X. <why>"
        const OddQ& o = odds[Q.oddIndex];
        sayf("THE ODD ONE OUT IS %s. %s", o.a[0], o.why);
        for (char* c = sayText; *c; c++)
        {
            if (*c >= 'a' && *c <= 'z') *c = (char)(*c - 'a' + 'A');
        }
    }
    if (anyRight)
    {
        flash = .3f;
        sv->sfx(Sfx::Win);
    }
    else
    {
        sv->sfx(Sfx::Tick);
    }
}

void QuizGame::toAnswer()
{
    ph = phAnswer;
    pt = 0;
}

void QuizGame::finish()
{
    ph = phEnd;
    pt = 0;
    int top = 0;
    for (int i = 1; i < np; i++)
    {
        if (P[i].score > P[top].score) top = i;
    }
    topIdx = (int8_t)top;
    bonus = 0;
    for (int i = 0; i < np; i++)
    {
        P[i].ex = i == top ? 'w' : 'n';
    }
    if (!P[top].cpu)
    {
        bonus = 500;
        P[top].score += 500;
        wins++;
    }
    persist();
    sayf("AND THE WINNER IS... %s!", P[top].name);
    strcpy(sayNext, "WHAT A SHOW! THANK YOU FOR PLAYING BRAIN BANZAI!");
    sv->sfx(Sfx::Big);
}

void QuizGame::gameOver()
{
    lastScore = P[0].score;
    lastTop = topIdx == 0;
    if (lastScore > best)
    {
        best = lastScore;
    }
    persist();
    ph = phOver;
    pt = 0;
}

void QuizGame::debugSkipToRound(int round)
{
    if (ph == phTitle)
    {
        newShow();
    }
    ri = (int8_t)(round - 1);
    beginRound();
}

void QuizGame::step()
{
    frame++;
    pt++;
    sayT++;
    if (flash > 0) flash -= .02f;
    for (int i = 0; i < np; i++)
    {
        if (P[i].popT > 0) P[i].popT--;
        if (P[i].frz > 0) P[i].frz--;
    }

    switch (ph)
    {
        case phMissing:
            if (takeConfirm())
            {
                bankOk = loadBank();
                if (bankOk) ph = phTitle;
                pt = 0;
            }
            break;
        case phTitle:
            if (pt > 20 && takeConfirm())
            {
                newShow();
            }
            break;
        case phIntro:
            if (pt > 380 || (pt > 40 && skipPressed())) beginRound();
            break;
        case phRIntro:
            if (pt >= 170 || (pt > 50 && skipPressed())) nextQ();
            break;
        case phRead:
        {
            rt++;
            int n = (int)strlen(Q.q);
            if (!cOn && rt * .6f >= n)
            {
                cOn = true;
                cOnT = 0;
            }
            if (cOn && ++cOnT > 420)
            {
                toReveal();
                break;
            }
            for (int i = 0; i < np; i++)
            {
                Player& p = P[i];
                if (p.out) continue;
                if (!p.cpu)
                {
                    int row = rowPressed;
                    bool b = takeConfirm();
                    if (b)
                    {
                        buzzIn(p);
                        // A pad row press both buzzes and answers once choices are showing.
                        if (row >= 0 && cOnT > 0 && !elim[row] && !p.el[row])
                        {
                            p.lock = (int8_t)row;
                            resolvePick(p, row);
                        }
                        break;
                    }
                }
                else if (rt >= p.tb)
                {
                    buzzIn(p);
                    break;
                }
            }
            break;
        }
        case phPick:
        {
            Player& p = P[buz];
            cd--;
            int sel = -1;
            if (!p.cpu)
            {
                if (llPressed)
                {
                    llPressed = false;
                    useLL(p);
                }
                int row = takeRow();
                confirmPressed = false;
                if (row >= 0 && !elim[row] && !p.el[row]) sel = row;
            }
            else if (pt >= p.pdelay)
            {
                sel = p.pick;
            }
            if (sel >= 0)
            {
                p.lock = (int8_t)sel;
                resolvePick(p, sel);
            }
            else if (cd <= 0)
            {
                resolvePick(p, -1);
            }
            break;
        }
        case phRead2:
            if (pt >= 45) toAnswer();
            break;
        case phAnswer:
        {
            bool ord = Q.kind == kOrder;
            bool humDone = true;
            int32_t endT = 0;
            for (int i = 0; i < np; i++)
            {
                Player& p = P[i];
                if (!p.cpu)
                {
                    if (llPressed)
                    {
                        llPressed = false;
                        useLL(p);
                    }
                    int row = takeRow();
                    confirmPressed = false;
                    if (ord)
                    {
                        if (p.fin < 0)
                        {
                            humDone = false;
                            if (row >= 0) ordPress(p, row);
                        }
                        else if (p.fin > endT)
                        {
                            endT = p.fin;
                        }
                    }
                    else if (p.lock < 0)
                    {
                        humDone = false;
                        if (row >= 0) lockAns(p, row, -1);
                    }
                    else if (p.lockT > endT)
                    {
                        endT = p.lockT;
                    }
                }
                else if (ord)
                {
                    if (p.fin < 0 && pt >= p.tf) p.fin = (int32_t)p.tf;
                    if (p.tf <= lim && p.tf > endT) endT = (int32_t)p.tf;
                }
                else
                {
                    if (p.lock < 0 && p.tl <= lim && pt >= p.tl) lockAns(p, p.pick, (int32_t)p.tl);
                    if (p.tl <= lim && p.tl > endT) endT = (int32_t)p.tl;
                }
            }
            if (pt >= lim || (humDone && pt >= endT + 50)) settle();
            break;
        }
        case phFCat:
        {
            Player& ch = P[chooser];
            int pickIt = -1;
            if (!ch.cpu)
            {
                int row = takeRow();
                if (jogSteps != 0)
                {
                    fcur = (int8_t)((fcur + (jogSteps > 0 ? 1 : -1) + 3) % 3);
                    jogSteps = 0;
                    sv->sfx(Sfx::Select);
                }
                if (row >= 0 && row < 3) pickIt = row;
                if (pickIt < 0 && confirmPressed)
                {
                    confirmPressed = false;
                    pickIt = fcur;
                }
            }
            else if (pt >= 100)
            {
                pickIt = irand(*sv, 3);
            }
            if (pickIt < 0 && pt > 700) pickIt = fcur;
            if (pickIt >= 0) pickCat(pickIt);
            break;
        }
        case phWager:
        {
            bool all = true;
            for (int i = 0; i < np; i++)
            {
                Player& p = P[i];
                if (p.wl) continue;
                if (p.cpu)
                {
                    if (pt >= p.wt) p.wl = true;
                }
                else
                {
                    if (jogSteps != 0)
                    {
                        p.bet += jogSteps * 50;
                        if (p.bet < 0) p.bet = 0;
                        if (p.bet > p.cap) p.bet = p.cap;
                        jogSteps = 0;
                        sv->sfx(Sfx::Select);
                    }
                    if (takeConfirm())
                    {
                        p.wl = true;
                        sv->sfx(Sfx::Confirm);
                    }
                }
                if (!p.wl) all = false;
            }
            if (all || pt > 900)
            {
                for (int i = 0; i < np; i++) P[i].wl = true;
                beginQ();
            }
            break;
        }
        case phReveal:
            if (pt >= 170 || (pt > 50 && skipPressed())) nextQ();
            break;
        case phEnd:
            if (pt == 200)
            {
                say(sayNext);
            }
            if (pt > 620 || (pt > 150 && skipPressed())) gameOver();
            break;
        case phOver:
            if (pt > 40 && takeConfirm())
            {
                newShow();
            }
            break;
    }

    // Drop input nobody used this frame (keeps a stale press from firing later)
    if (ph != phRead)
    {
        rowPressed = -1;
        confirmPressed = false;
    }
    if (ph != phPick && ph != phAnswer)
    {
        llPressed = false;
    }
    if (ph != phWager && ph != phFCat)
    {
        jogSteps = 0;
    }
}

//==============================================================================================================
// Drawing
//==============================================================================================================
int QuizGame::wrap(Gfx& g, uint8_t font, const char* s, int16_t width, int maxLines, int* starts, int* lens)
{
    int n = (int)strlen(s);
    int lines = 0;
    int pos = 0;
    while (pos < n && lines < maxLines)
    {
        while (pos < n && s[pos] == ' ') pos++;
        int lineEnd = pos;
        int lastBreak = -1;
        while (lineEnd < n)
        {
            int next = lineEnd;
            while (next < n && s[next] != ' ') next++;
            if (g.textWidth(font, s + pos, next - pos) > width)
            {
                break;
            }
            lastBreak = next;
            lineEnd = next;
            if (lineEnd < n) lineEnd++;
        }
        int len;
        if (lastBreak < 0)
        {
            // a single word wider than the box: hard break
            len = 1;
            while (pos + len < n && g.textWidth(font, s + pos, len + 1) <= width) len++;
        }
        else
        {
            len = lastBreak - pos;
        }
        if (lines == maxLines - 1 && pos + len < n)
        {
            len = n - pos; // last line takes the rest (clipped by the caller)
        }
        starts[lines] = pos;
        lens[lines] = len;
        lines++;
        pos += len;
    }
    return lines;
}

void QuizGame::para(Gfx& g, uint8_t font, const char* s, int16_t x, int16_t y, int16_t width, int16_t lineH,
                    int maxLines, int maxChars, uint8_t align)
{
    int starts[8], lens[8];
    if (maxLines > 8) maxLines = 8;
    int n = wrap(g, font, s, width, maxLines, starts, lens);
    for (int i = 0; i < n; i++)
    {
        int len = lens[i];
        if (maxChars >= 0)
        {
            if (starts[i] >= maxChars) break;
            if (starts[i] + len > maxChars) len = maxChars - starts[i];
        }
        if (len <= 0) continue;
        int16_t lx = align & kCenterX ? (int16_t)(x + width / 2) : x;
        g.text(lx, (int16_t)(y + i * lineH), font, align & kCenterX ? kCenterX : kLeft, s + starts[i], len);
    }
}

void QuizGame::draw(Gfx& g, uint8_t layer)
{
    dirtyMask &= (uint8_t)~(1u << layer);
    if (layer == 0)
    {
        drawBackground(g);
    }
    else if (layer == 1)
    {
        drawForeground(g);
    }
}

void QuizGame::drawBackground(Gfx& g)
{
    // TV studio: deep purple wash with soft stage lights and a lit floor
    const uint32_t bands[6] = { 0x0C0620, 0x120A30, 0x1A0E40, 0x221250, 0x1A0E40, 0x100828 };
    int16_t bh = kCanvasH / 6;
    for (int i = 0; i < 6; i++)
    {
        g.color(bands[i]);
        g.rect(kCanvasX, (int16_t)(kCanvasY + i * bh), kCanvasX + kCanvasW, (int16_t)(kCanvasY + (i + 1) * bh + 1));
    }
    g.alpha(40);
    g.color(0x8060FF);
    g.circle(120, kCanvasY + 30, 120);
    g.circle(680, kCanvasY + 30, 120);
    g.color(0xFFC060);
    g.circle(400, kCanvasY + 10, 90);
    g.alpha(255);

    if (ph == phTitle || ph == phMissing || ph == phOver)
    {
        return; // no podiums on the title screens
    }

    // podium bases
    for (int i = 0; i < 4; i++)
    {
        int16_t x0 = (int16_t)(i * 200 + 6), x1 = (int16_t)(x0 + 188);
        g.color(0x2A1A5A);
        g.rect(x0, kPodY0, x1, kPodY1, 6);
        g.color(0x3A2A7A);
        g.rect(x0 + 2, kPodY0 + 2, x1 - 2, kPodY0 + 10, 4);
    }
}

void QuizGame::drawHeader(Gfx& g)
{
    char buf[48];
    const char* left = "BRAIN BANZAI!";
    const char* right = "";
    char rbuf[24];
    if (ri >= 0 && ri < planLen && ph != phEnd && ph != phOver)
    {
        left = roundName(plan[ri].t, plan[ri].cat, buf, sizeof(buf));
        snprintf(rbuf, sizeof(rbuf), "ROUND %d OF %d", ri + 1, planLen);
        right = rbuf;
    }
    g.color(kGold);
    g.text(14, kHeaderY + 2, kRom25, kLeft, left);
    g.color(kLilac);
    g.text(786, kHeaderY + 8, kFont14B, kRight, right);

    // timer bar
    float f = -1;
    if (ph == phRead && cOn) f = 1 - cOnT / 420.0f;
    else if (ph == phPick) f = cd / 420.0f;
    else if (ph == phAnswer) f = 1 - (float)pt / lim;
    else if (ph == phWager) f = 1 - pt / 900.0f;
    if (f >= 0)
    {
        if (f > 1) f = 1;
        int16_t w = (int16_t)(f * 772);
        g.color(0x241848);
        g.rect(14, kHeaderY + 30, 786, kHeaderY + 34, 2);
        g.color(f > .3f ? 0x40E0FF : 0xFF5050);
        g.rect(14, kHeaderY + 30, (int16_t)(14 + w), kHeaderY + 34, 2);
    }
}

void QuizGame::drawPanel(Gfx& g)
{
    g.color(kPanelEdge);
    g.rect(10, kPanelY0, 790, kPanelY1, 8);
    g.color(kPanel);
    g.rect(13, kPanelY0 + 3, 787, kPanelY1 - 3, 6);

    const int16_t tx = 26, tw = 748;
    bool hostOnly = ph == phIntro || ph == phRIntro || ph == phFCat || ph == phEnd || ph == phOver ||
                    ph == phTitle || ph == phMissing || (ph == phWager && pt < 150);

    if (ph == phWager && pt >= 150)
    {
        char b[64];
        snprintf(b, sizeof(b), "CATEGORY: %s", kCats[fcat].name);
        g.color(kGold);
        g.text(400, kPanelY0 + 14, kFont17B, kCenterX, b);
        g.color(0xFFFFFF);
        g.text(400, kPanelY0 + 42, kRom25, kCenterX, "PLACE YOUR WAGER");
        g.color(kLilac);
        g.text(400, kPanelY0 + 74, kFont14, kCenterX, "Jog wheel or Up/Down: change bet  -  Play or a pad: lock in");
        return;
    }

    if (hostOnly)
    {
        if (ph == phRIntro)
        {
            char b[32];
            snprintf(b, sizeof(b), "ROUND %d OF %d", ri + 1, planLen);
            g.color(kLilac);
            g.text(400, kPanelY0 + 10, kFont14B, kCenterX, b);
        }
        else if (ph == phIntro)
        {
            g.color(kLilac);
            g.text(400, kPanelY0 + 10, kFont14B, kCenterX, "KAGEMORI PRESENTS");
        }
        else if (ph == phEnd)
        {
            g.color(kLilac);
            g.text(400, kPanelY0 + 10, kFont14B, kCenterX, "FINAL RESULT");
        }
        else if (ph == phFCat)
        {
            g.color(kLilac);
            g.text(400, kPanelY0 + 10, kFont14B, kCenterX, "FINAL ROUND - PICK A CATEGORY");
        }
        g.color(kGold);
        g.text(tx, kPanelY0 + 30, kFont12, kLeft, "MR. MARU:");
        int chars = sayT * 2; // host talks at ~120 chars/s
        g.color(0xFFFFFF);
        para(g, kFont17B, sayText, tx, kPanelY0 + 46, tw, 22, 3, chars, kLeft);
        return;
    }

    // Question
    bool reveal = ph == phReveal;
    g.color(reveal ? 0x80FF90 : kGold);
    g.text(tx, kPanelY0 + 10, kFont14B, kLeft, Q.cat ? Q.cat : "");

    if (reveal)
    {
        g.color(kLilac);
        g.text(tx, kPanelY0 + 32, kFont14, kLeft, Q.kind == kOrder ? "THE ORDER:" : "THE ANSWER IS");
        char ans[200];
        if (Q.kind == kOrder)
        {
            snprintf(ans, sizeof(ans), "%s > %s > %s > %s", Q.ch[Q.order[0]], Q.ch[Q.order[1]], Q.ch[Q.order[2]],
                     Q.ch[Q.order[3]]);
        }
        else
        {
            snprintf(ans, sizeof(ans), "%s", Q.ans >= 0 ? Q.ch[Q.ans] : "");
        }
        g.color(0xFFFFFF);
        para(g, kFont17B, ans, tx, kPanelY0 + 50, tw, 22, 1, -1, kLeft);
        g.color(kGold);
        para(g, kFont14, sayText, tx, kPanelY0 + 76, tw, 18, 1, sayT * 2, kLeft);
        return;
    }

    int n = (int)strlen(Q.q);
    int cnt = ph == phRead ? (int)(rt * .6f) : n;
    if (cnt > n) cnt = n;
    g.color(0xFFFFFF);
    para(g, kFont17B, Q.q, tx, kPanelY0 + 32, tw, 22, 3, cnt, kLeft);

    char worth[72] = "";
    uint32_t wc = 0xA0A0D0;
    if (ph == phRead || ph == phPick)
    {
        bool isEarly = rt < n / .6f;
        snprintf(worth, sizeof(worth), "WORTH %s", isEarly ? "200  BUZZ EARLY!" : "100");
        wc = isEarly ? 0xFFB040 : 0xA0A0D0;
    }
    else if (ph == phAnswer && Q.kind == kOrder)
    {
        snprintf(worth, sizeof(worth), "PRESS THE ROWS IN ORDER  -  FIRST CORRECT SCORES 300");
        wc = 0xFFB040;
    }
    else if (ph == phAnswer && plan[ri].t == rtFinal)
    {
        snprintf(worth, sizeof(worth), "FOR YOUR WAGER OF %ld", (long)P[0].bet);
        wc = 0xFFB040;
    }
    else if (ph == phAnswer)
    {
        snprintf(worth, sizeof(worth), "EVERYBODY ANSWERS  -  FASTER SCORES MORE");
        wc = 0xFFB040;
    }
    if (worth[0])
    {
        g.color(wc);
        g.text(tx, kPanelY1 - 22, kFont14B, kLeft, worth);
    }
    if (ph == phRead && !P[0].out)
    {
        g.color(0xFFFFFF);
        g.text(774, kPanelY1 - 22, kFont14B, kRight, cOn ? "PRESS A ROW TO ANSWER" : "PLAY OR ANY PAD = BUZZ");
    }
    if (ph == phPick && buz >= 0)
    {
        char b[48];
        snprintf(b, sizeof(b), "%s BUZZED!", P[buz].name);
        g.color(P[buz].col);
        g.text(774, kPanelY1 - 22, kFont14B, kRight, b);
    }
}

void QuizGame::drawChoices(Gfx& g)
{
    if (ph == phFCat)
    {
        for (int i = 0; i < 3; i++)
        {
            int16_t y0 = (int16_t)(kChoiceY0 + i * (kChoiceH + kChoiceGap)), y1 = (int16_t)(y0 + kChoiceH);
            bool sel = fcur == i;
            g.color(sel ? 0xFFE040 : kPanelEdge);
            g.rect(10, y0, 790, y1, 6);
            g.color(sel ? 0x20308A : kPanel);
            g.rect(13, y0 + 3, 787, y1 - 3, 4);
            g.color(kSlot[i]);
            g.rect(16, y0 + 6, 52, y1 - 6, 4);
            char n[4] = { (char)('1' + i), 0 };
            g.color(0xFFFFFF);
            g.text(34, y0 + 9, kFont17B, kCenterX, n);
            g.text(66, y0 + 10, kFont17B, kLeft, kCats[fc[i]].name);
        }
        return;
    }

    bool show = cOn && (ph == phRead || ph == phPick || ph == phRead2 || ph == phAnswer || ph == phReveal);
    if (!show)
    {
        if (ph == phRead)
        {
            g.color(kLilac);
            g.text(400, kChoiceY0 + 70, kRom25, kCenterX, "LISTEN... THEN BUZZ!");
        }
        return;
    }
    bool rv = ph == phReveal;
    bool ord = Q.kind == kOrder;
    for (int i = 0; i < 4; i++)
    {
        int16_t y0 = (int16_t)(kChoiceY0 + i * (kChoiceH + kChoiceGap)), y1 = (int16_t)(y0 + kChoiceH);
        bool gone = elim[i] || P[0].el[i];
        bool right = rv && !ord && i == Q.ans;
        bool mine = !ord && P[0].lock == i;
        uint32_t edge = right ? 0x40FF70 : mine ? 0xFFE040 : kPanelEdge;
        uint32_t fill = right ? 0x145A28 : gone ? 0x0A0E30 : mine ? 0x20308A : kPanel;
        g.color(edge);
        g.rect(10, y0, 790, y1, 6);
        g.color(fill);
        g.rect(13, y0 + 3, 787, y1 - 3, 4);
        g.color(gone ? 0x404060 : kSlot[i]);
        g.rect(16, y0 + 6, 52, y1 - 6, 4);
        char n[4] = { (char)('1' + i), 0 };
        g.color(0xFFFFFF);
        g.text(34, y0 + 9, kFont17B, kCenterX, n);
        g.color(gone ? 0x6060A0 : 0xFFFFFF);
        g.clip(60, y0, 640, kChoiceH);
        g.text(66, y0 + 10, kFont17B, kLeft, Q.ch[i]);
        g.unclip();

        // Who picked this one (shown after the reveal), or order progress
        if (ord)
        {
            int k = -1;
            if (rv)
            {
                for (int m = 0; m < 4; m++)
                {
                    if (Q.order[m] == i) k = m + 1;
                }
            }
            else
            {
                for (int m = 0; m < P[0].prog; m++)
                {
                    if (Q.order[m] == i) k = m + 1;
                }
            }
            if (k > 0)
            {
                char b[4] = { (char)('0' + k), 0 };
                g.color(0x40FF70);
                g.circle(766, (int16_t)(y0 + kChoiceH / 2), 13);
                g.color(0x0A2010);
                g.text(766, y0 + 11, kFont17B, kCenterX, b);
            }
        }
        else if (rv || ph == phAnswer)
        {
            int16_t mx = 772;
            for (int p = np - 1; p >= 0; p--)
            {
                bool showIt = P[p].lock == i && (rv || !P[p].cpu);
                if (!showIt) continue;
                g.color(P[p].col);
                g.circle(mx, (int16_t)(y0 + kChoiceH / 2), 9);
                mx -= 22;
            }
        }
    }
}

void QuizGame::drawPodiums(Gfx& g)
{
    for (int i = 0; i < 4; i++)
    {
        const Player& p = P[i];
        int16_t x0 = (int16_t)(i * 200 + 6), x1 = (int16_t)(x0 + 188), cx = (int16_t)((x0 + x1) / 2);
        bool lit = (ph == phPick && buz == i) || (ph == phEnd && topIdx == i);
        bool locked = (ph == phAnswer || ph == phRead2) && (p.lock >= 0 || p.fin >= 0);
        if (lit || locked)
        {
            g.color(lit ? ((frame >> 3) & 1 ? 0xFFFFFF : kGold) : kGold);
            g.rect(x0, kPodY0, x1, kPodY1, 6);
            g.color(0x2A1A5A);
            g.rect(x0 + 3, kPodY0 + 3, x1 - 3, kPodY1 - 3, 5);
        }
        // name plate in the contestant's colour, streak on its right
        g.color(p.col);
        g.rect(x0 + 8, kPodY0 + 6, x1 - 8, kPodY0 + 27, 4);
        g.color(0xFFFFFF);
        g.text(cx, kPodY0 + 9, kFont14B, kCenterX, p.name ? p.name : "");
        if (p.streak >= 2 && ph != phEnd)
        {
            char b[8];
            snprintf(b, sizeof(b), "x%d", p.streak);
            g.color(0x200818);
            g.text(x1 - 14, kPodY0 + 9, kFont14B, kRight, b);
        }

        // score on the left, status (or the latest pop) on the right
        char s[16];
        snprintf(s, sizeof(s), "%ld", (long)p.score);
        g.color(0xFFFFFF);
        g.text(x0 + 14, kPodY0 + 32, kRom28, kLeft, s);

        const char* st = nullptr;
        uint32_t sc = kLilac;
        char sb[24];
        if (p.ex == 'w')
        {
            st = "RIGHT!";
            sc = 0x80FF90;
        }
        else if (p.ex == 'l')
        {
            st = "WRONG";
            sc = 0xFF7070;
        }
        else if (ph == phWager || (plan[ri < 0 ? 0 : ri].t == rtFinal && (ph == phRead2 || ph == phAnswer)))
        {
            snprintf(sb, sizeof(sb), p.wl ? "BET %ld" : "BET ...", (long)p.bet);
            st = (p.cpu && !p.wl) ? "THINKING" : sb;
            sc = kGold;
        }
        else if (locked)
        {
            st = "LOCKED";
            sc = kGold;
        }
        if (ph == phAnswer && Q.kind == kOrder && !p.cpu)
        {
            snprintf(sb, sizeof(sb), "%d OF 4", p.prog);
            st = p.fin >= 0 ? "DONE!" : sb;
            sc = p.frz > 0 ? 0xFF7070 : kGold;
        }
        if (p.popT > 0)
        {
            st = p.pop;
            sc = p.popCol;
        }
        if (st)
        {
            g.color(sc);
            g.text(x1 - 12, kPodY0 + 40, kFont14B, kRight, st);
        }
    }
}

void QuizGame::drawTitle(Gfx& g)
{
    g.color(0xFFB018);
    g.text(400, kCanvasY + 50, kRom49, kCenterX, "BRAIN BANZAI!");
    g.color(kLilac);
    g.text(400, kCanvasY + 108, kRom25, kCenterX, "QUIZ KAGEMORI");
    g.color(0xFFFFFF);
    g.text(400, kCanvasY + 150, kFont14, kCenterX, "You against three CPU contestants. Six rounds, one winner.");

    if (ph == phMissing)
    {
        g.color(0xFF7070);
        g.text(400, kCanvasY + 196, kFont17B, kCenterX, "QUESTION BANK NOT FOUND");
        g.color(0xFFFFFF);
        g.text(400, kCanvasY + 224, kFont14, kCenterX, "Copy the Quiz folder into i2/Arcade/ on the SD card,");
        g.text(400, kCanvasY + 244, kFont14, kCenterX, "so the card has i2/Arcade/Quiz/questions.txt, then press Retry.");
        return;
    }

    if (ph == phOver)
    {
        char b[64];
        snprintf(b, sizeof(b), "YOUR SCORE  %ld", (long)lastScore);
        g.color(kGold);
        g.text(400, kCanvasY + 188, kRom28, kCenterX, b);
        g.color(lastTop ? 0x80FF90 : kLilac);
        g.text(400, kCanvasY + 226, kFont17B, kCenterX, lastTop ? "YOU WON THE SHOW!" : "BETTER LUCK NEXT SHOW!");
    }

    char b[64];
    snprintf(b, sizeof(b), "BEST %ld   -   WINS %ld", (long)best, (long)wins);
    g.color(kLilac);
    g.text(400, kCanvasY + 262, kFont14B, kCenterX, b);

    if ((frame / 30) & 1)
    {
        g.color(0xFFFFFF);
        g.text(400, kCanvasY + 300, kRom25, kCenterX, "PRESS PLAY OR ANY PAD");
    }
    g.color(0x9A9AC0);
    g.text(400, kCanvasY + 346, kFont12, kCenterX,
           "Pad rows 1-4 pick answers 1-4.  Play or any pad buzzes.  Screen button 1: 50:50 lifeline.");
}

void QuizGame::drawForeground(Gfx& g)
{
    if (ph == phTitle || ph == phMissing || ph == phOver)
    {
        drawTitle(g);
        return;
    }
    drawHeader(g);
    drawPanel(g);
    drawChoices(g);
    drawPodiums(g);
    if (ph == phEnd && topIdx >= 0)
    {
        char b[48];
        snprintf(b, sizeof(b), "%s %s!", P[topIdx].name, P[topIdx].cpu ? "WINS" : "WIN");
        g.color(kGold);
        g.text(400, kChoiceY0 + 40, kRom49, kCenterX, b);
        if (bonus)
        {
            g.color(0x80FF90);
            g.text(400, kChoiceY0 + 104, kFont17B, kCenterX, "+500 CHAMPION BONUS");
        }
    }
    if (flash > 0)
    {
        g.alpha((uint8_t)(flash * 150 > 45 ? 45 : flash * 150));
        g.color(0xFFFFFF);
        g.rect(kCanvasX, kCanvasY, kCanvasX + kCanvasW, kCanvasY + kCanvasH);
        g.alpha(255);
    }
}

} // namespace i2arc
