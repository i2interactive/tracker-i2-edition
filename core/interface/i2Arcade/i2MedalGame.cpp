#include "i2MedalGame.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Tuning, payouts and timings follow kmedal.js (60 frames per second). The
// game runs in the original's 384x224 coordinates and is drawn 1.75x larger.

namespace i2arc
{
namespace
{
constexpr uint32_t kFrameMs = 1000 / 60;

// --- kmedal.js constants
constexpr float kFX = 82, kTW = 220, kKY = .6f, kR = 6, kPOCK = 38, kDROPY = 12;
const float kTY[2] = { 84, 136 };
const float kE[2] = { 60, 70 };
const float kPR[2][2] = { { 6, 20 }, { 8, 24 } };
constexpr int32_t kLegT = 40 * 60;
constexpr int kNLeg = 5;
constexpr float kRide = .92f, kBack = .5f, kStroke = 150;
constexpr int16_t kDropCd = 11, kHoldDelay = 18, kHoldRate = 16;
constexpr float kChAcc = .32f, kChMax = 2.6f;
const int16_t kReelStop[3] = { 30, 44, 58 };
constexpr uint8_t kRouSteps = 11;
constexpr float kLane0 = 88, kLane1 = 132;
constexpr uint8_t kWfN = 12;
// share of an overlap taken by the coin nearer the back (0.5 = even)
float kPushA = .5f;
int kSettleSteps = 600;

struct Leg
{
    const char* n;
    int m;
    float gs, gw;
    uint32_t sky[5];
    uint32_t far, mtn, mtn2, tree, sun;
    bool night;
};
const Leg kLegs[kNLeg] = {
    { "BASE CAMP", 1, .022f, 18, { 0x5aaee8, 0x7ec0f0, 0xa4d4f4, 0xcce6f6, 0xf4f2d8 }, 0x8aa8d0, 0x6a88b8, 0x4a6898, 0x2a7040, 0xfff4b0, false },
    { "PINE FOREST", 1, .028f, 16, { 0x2a80d8, 0x3a92e0, 0x5aa8ea, 0x7cbcf0, 0xa2d2f6 }, 0x7a9ccc, 0x5a7cb0, 0x3c5c90, 0x1e6034, 0xfff8d0, false },
    { "CLOUD PASS", 2, .034f, 15, { 0x4a98c8, 0x6aaed4, 0x8ec4e0, 0xb4d8ea, 0xdceef4 }, 0x9ab4d0, 0x6a86a8, 0x4a6688, 0x2a6a48, 0xffffff, false },
    { "SNOW FIELD", 2, .04f, 14, { 0x3a2a68, 0x8a3a7a, 0xd85a68, 0xf08a50, 0xffc070 }, 0xa86a8a, 0x7a5a8a, 0x5a3a6a, 0x3a2a4a, 0xffe080, false },
    { "SUMMIT", 3, .046f, 12, { 0x060820, 0x0e1238, 0x18204c, 0x22305e, 0x2e4072 }, 0x2a3a68, 0x3a4a7a, 0x222e58, 0x101a30, 0xf4f0d0, true },
};

// Reel symbols (KM_SYM order) and strips
enum Sym : uint8_t
{
    sApple,
    sGrape,
    sPeach,
    sBunny,
    sBear,
    sBird,
    sCar,
    sSun,
    sMtn,
};
const uint8_t kStrip[3][9] = { { 0, 3, 7, 1, 4, 8, 2, 5, 6 }, { 2, 6, 0, 5, 8, 3, 1, 7, 4 }, { 1, 4, 8, 0, 6, 2, 7, 3, 5 } };

enum Out : uint8_t
{
    oMtn,
    oCar,
    oSun,
    oBunny,
    oBear,
    oBird,
    oApple,
    oGrape,
    oPeach,
    oSalad,
    oZoo,
    oMiss,
};
const float kOutW[11] = { 1.6f, 3.5f, 9, 5, 3, 2, 11, 7, 5, 6, 5 };

// Sky roulette (KM_ROU): t = tickets, m = medal rain, car = cable car bonus
struct RouPrize
{
    int8_t t, m, car;
};
const RouPrize kRou[12] = { { 5, 0, 0 }, { 2, 0, 0 }, { 10, 0, 0 }, { 0, 8, 0 }, { 3, 0, 0 }, { 20, 0, 0 },
                            { 2, 0, 0 }, { 0, 0, 1 }, { 5, 0, 0 }, { 3, 0, 0 }, { 30, 0, 0 }, { 0, 12, 0 } };
const uint32_t kRouC[4] = { 0xff5050, 0xffd040, 0x40c0ff, 0x60e070 };

// Cable car stations (KM_CARV)
struct CarStop
{
    int8_t t, m;
    bool top;
    const char* l;
};
const CarStop kCar[6] = { { 5, 0, false, "5" }, { 0, 8, false, "M8" }, { 10, 0, false, "10" },
                          { 0, 15, false, "M15" }, { 20, 0, false, "20" }, { 0, 0, true, "TOP" } };

// Cabinet colours (CHERRY RED skin)
constexpr uint32_t kSkinA = 0xd02838, kSkinB = 0xf4e4c8, kSkinC = 0xffd040, kOL = 0x2a1206;
const uint32_t kMedalCol[3] = { 0xf0c030, 0xdcdce8, 0xe08848 };
const uint32_t kMedalHi[3] = { 0xfff0a0, 0xffffff, 0xffc090 };
const uint32_t kBallCol[3] = { 0xff70b0, 0x40a0ff, 0xffc020 };

// original coordinates -> Tracker screen
inline int16_t X(float ox) { return (int16_t)(64 + ox * 1.75f + .5f); }
inline int16_t Y(float oy) { return (int16_t)(kCanvasY + oy * 1.75f + .5f); }
inline int16_t S(float o) { return (int16_t)(o * 1.75f + .5f); }

const char* kSavePath = "i2/Arcade/scores_medal.txt";

int16_t s_xy[2 * 340];
} // namespace

//==============================================================================================================
// Saves and lifecycle
//==============================================================================================================
void MedalGame::loadSave()
{
    char buf[256];
    int32_t n = sv->loadText(kSavePath, buf, sizeof(buf) - 1);
    if (n <= 0)
    {
        return;
    }
    buf[n] = 0;
    auto num = [&](const char* key, int32_t& out, int32_t lo, int32_t hi) {
        const char* p = strstr(buf, key);
        if (!p) return;
        long v = strtol(p + strlen(key), nullptr, 10);
        if (v < lo) v = lo;
        if (v > hi) v = hi;
        out = (int32_t)v;
    };
    num("bank=", bank, 0, 9999);
    num("jp=", jp, 50, 300);
    num("tickets=", lifetimeTix, 0, 99999999);
    num("best=", bestTix, 0, 99999999);
    num("sessions=", sessions, 0, 9999999);
}

void MedalGame::persist()
{
    char buf[160];
    int n = snprintf(buf, sizeof(buf), "bank=%ld\njp=%ld\ntickets=%ld\nbest=%ld\nsessions=%ld\n", (long)bank, (long)jp,
                     (long)lifetimeTix, (long)bestTix, (long)sessions);
    if (n > 0)
    {
        sv->saveText(kSavePath, buf, (size_t)n);
    }
}

void MedalGame::begin(Services& services)
{
    sv = &services;
    lastMs = accMs = 0;
    frame = 0;
    dirtyMask = 7;
    bgLeg = 255;
    ph = phTitle;
    coinCount = 0;
    jogAcc = 0;
    leftHeld = rightHeld = dropHeld = dropPressed = startPressed = false;
    padsDown = 0;
    bank = 100;
    jp = 50;
    lifetimeTix = bestTix = sessions = 0;
    loadSave();
    refreshPadLights();
}

void MedalGame::end()
{
    // Leaving mid-session: medals still in hand go back to the bank.
    if (ph == phGo || ph == phFinal || ph == phReady)
    {
        bank = med > 9999 ? 9999 : med;
        lifetimeTix += tix;
        persist();
    }
}

//==============================================================================================================
// Input
//==============================================================================================================
void MedalGame::pad(uint8_t pad, bool down)
{
    if (down)
    {
        padsDown++;
        if (ph == phTitle || ph == phResults)
        {
            startPressed = true;
            return;
        }
        // The pad's column is where the chute goes
        uint8_t col = pad % 12;
        padTarget = (int8_t)col;
        dropPressed = true;
        dropHeld = true;
        holdT = 0;
    }
    else
    {
        if (padsDown > 0) padsDown--;
        if (padsDown == 0)
        {
            dropHeld = false;
            padTarget = -1;
        }
    }
}

void MedalGame::button(Button b, bool down)
{
    switch (b)
    {
        case Button::Left: leftHeld = down; break;
        case Button::Right: rightHeld = down; break;
        case Button::Play:
            if (down && (ph == phTitle || ph == phResults))
            {
                startPressed = true;
                break;
            }
            dropHeld = down;
            if (down)
            {
                dropPressed = true;
                holdT = 0;
            }
            break;
        case Button::Screen0:
            if (down && (ph == phTitle || ph == phResults)) startPressed = true;
            break;
        default: break;
    }
}

void MedalGame::jog(int16_t steps)
{
    jogAcc += steps;
}

const char* MedalGame::buttonLabel(uint8_t i) const
{
    if (i == 0 && (ph == phTitle || ph == phResults))
    {
        return "Start";
    }
    return nullptr;
}

void MedalGame::refreshPadLights()
{
    if (!sv) return;
    int chuteCol = (int)((cx - 8) / (204.0f / 11.0f) + .5f);
    for (uint8_t p = 0; p < 48; p++)
    {
        uint8_t level = 1;
        if ((ph == phGo || ph == phFinal) && (p % 12) == chuteCol)
        {
            level = 2;
        }
        else if (ph == phReady)
        {
            level = 0;
        }
        sv->padLight(p, level);
    }
}

//==============================================================================================================
// Physics: coins are discs on two shelves. x across (0..220), y depth (0 back .. edge).
//==============================================================================================================
MedalGame::Coin* MedalGame::addCoin(uint8_t tier, float x, float y, uint8_t look, uint8_t air)
{
    if (coinCount >= kMaxCoins)
    {
        compact();
        if (coinCount >= kMaxCoins) return nullptr;
    }
    Coin& c = coins[coinCount++];
    c.x = x;
    c.y = y;
    c.r = look >= 10 ? 9 : kR;
    c.vx = 0;
    c.tier = tier;
    c.look = look;
    c.air = air;
    c.airMax = air ? air : 1;
    c.dead = false;
    c.counts = true;
    return &c;
}

void MedalGame::compact()
{
    int w = 0;
    for (int i = 0; i < coinCount; i++)
    {
        if (!coins[i].dead) coins[w++] = coins[i];
    }
    coinCount = w;
}

void MedalGame::seedBed(uint8_t tier)
{
    float lo = kPR[tier][1] + kR + .5f, e = kE[tier];
    int row = 0;
    for (float y = lo; y < e - 1; y += kR * 1.74f, row++)
    {
        for (float x = kR + (row & 1) * kR; x <= kTW - kR; x += kR * 2.02f)
        {
            Coin* c = addCoin(tier, x + (frand(*sv) - .5f) * .4f, y, (uint8_t)irand(*sv, 3), 0);
            if (c) c->counts = false;
        }
    }
    for (int i = 0; i < 6; i++)
    {
        Coin* c = addCoin(tier, 20 + frand(*sv) * 180, kR + frand(*sv) * 3, (uint8_t)irand(*sv, 3), 0);
        if (c) c->counts = false;
    }
}

void MedalGame::relax(uint8_t tier)
{
    // Sort coin indices on this tier by x (insertion sort: nearly sorted frame to frame)
    static int16_t idx[kMaxCoins];
    int n = 0;
    for (int i = 0; i < coinCount; i++)
    {
        if (!coins[i].dead && coins[i].tier == tier && coins[i].air == 0) idx[n++] = (int16_t)i;
    }
    for (int i = 1; i < n; i++)
    {
        int16_t k = idx[i];
        float kx = coins[k].x;
        int j = i - 1;
        while (j >= 0 && coins[idx[j]].x > kx)
        {
            idx[j + 1] = idx[j];
            j--;
        }
        idx[j + 1] = k;
    }

    const Shelf& sh = P[tier];
    for (int a = 0; a < n; a++)
    {
        Coin& A = coins[idx[a]];
        for (int b = a + 1; b < n; b++)
        {
            Coin& B = coins[idx[b]];
            float rr = A.r + B.r;
            float dx = B.x - A.x;
            if (dx >= rr) break;
            float dy = B.y - A.y;
            float d2 = dx * dx + dy * dy;
            if (d2 >= rr * rr) continue;
            // only coins on the same level touch (both on the plate, or both on the bed)
            bool aPlate = A.y < sh.front, bPlate = B.y < sh.front;
            if (aPlate != bPlate) continue;
            float d = sqrtf(d2);
            float nx, ny;
            if (d < .001f)
            {
                nx = 1;
                ny = 0;
                d = .001f;
            }
            else
            {
                nx = dx / d;
                ny = dy / d;
            }
            float ov = rr - d;
            float wa = kPushA, wb = 1 - kPushA;
            if (ny < 0)
            {
                wa = 1 - kPushA;
                wb = kPushA;
            }
            bool aPinned = !aPlate && A.y - A.r <= sh.front + .05f;
            bool bPinned = !bPlate && B.y - B.r <= sh.front + .05f;
            if (aPinned && ny > 0)
            {
                wa = 0;
                wb = 1;
            }
            if (bPinned && ny < 0)
            {
                wa = 1;
                wb = 0;
            }
            A.x -= nx * ov * wa;
            A.y -= ny * ov * wa;
            B.x += nx * ov * wb;
            B.y += ny * ov * wb;
        }
    }
}

void MedalGame::stepShelf(uint8_t tier, bool quiet)
{
    Shelf& sh = P[tier];
    float f0 = sh.front;
    sh.ph += sh.spd;
    sh.front = sh.lo + (sh.hi - sh.lo) * (.5f - .5f * cosf(sh.ph));
    sh.dp = sh.front - f0;

    for (int i = 0; i < coinCount; i++)
    {
        Coin& c = coins[i];
        if (c.dead || c.tier != tier) continue;
        if (c.air > 0)
        {
            c.air--;
            continue;
        }
        if (c.y < f0)
        {
            // riding on top of the pusher plate
            c.y += sh.dp * (sh.dp > 0 ? kRide : kBack);
            if (c.y < c.r) c.y = c.r;
        }
        else if (c.y - c.r < sh.front)
        {
            // the plate's face shoves the bed
            c.y = sh.front + c.r;
        }
        if (c.vx != 0)
        {
            c.x += c.vx;
            c.vx *= .9f;
            if (fabsf(c.vx) < .01f) c.vx = 0;
        }
    }

    for (int it = 0; it < 4; it++)
    {
        relax(tier);
        for (int i = 0; i < coinCount; i++)
        {
            Coin& c = coins[i];
            if (c.dead || c.tier != tier || c.air) continue;
            if (c.y >= f0 && c.y - c.r < sh.front) c.y = sh.front + c.r;
            if (c.y < c.r) c.y = c.r;
            bool walled = tier == 0 || c.y < kPOCK;
            if (walled)
            {
                if (c.x < c.r) c.x = c.r;
                if (c.x > kTW - c.r) c.x = kTW - c.r;
            }
        }
    }

    for (int i = 0; i < coinCount; i++)
    {
        Coin& c = coins[i];
        if (c.dead || c.tier != tier || c.air) continue;
        if (c.y > kE[tier] || c.x < 0 || c.x > kTW)
        {
            fallOff(c, quiet);
        }
    }
}

void MedalGame::fallOff(Coin& c, bool quiet)
{
    if (c.tier == 0 && c.y > kE[0])
    {
        // tumbles onto the lower shelf
        c.tier = 1;
        c.y = 6.5f + frand(*sv) * 4;
        c.vx *= .5f;
        c.air = 8;
        c.airMax = 8;
        return;
    }
    c.dead = true;
    if (quiet || !c.counts)
    {
        return;
    }
    if (c.tier == 1 && c.y > kE[1])
    {
        onWin(c);
    }
    else
    {
        onLost(c);
    }
}

//==============================================================================================================
// A session: five legs of the climb, 40 seconds each
//==============================================================================================================
int MedalGame::mult() const
{
    return kLegs[leg].m;
}

void MedalGame::newSession()
{
    coinCount = 0;
    for (int i = 0; i < 2; i++)
    {
        P[i].lo = kPR[i][0];
        P[i].hi = kPR[i][1];
        P[i].front = P[i].lo;
        P[i].ph = i ? 3.14159265f : 0;
        P[i].spd = 2 * 3.14159265f / kStroke;
        P[i].dp = 0;
    }
    seedBed(0);
    seedBed(1);
    // settle the bed: the shelves run a few strokes before the doors open
    for (int i = 0; i < kSettleSteps; i++)
    {
        stepShelf(0, true);
        stepShelf(1, true);
    }
    compact();
    for (int i = 0; i < coinCount; i++) coins[i].counts = true;

    t = 0;
    total = kNLeg * kLegT;
    left = total;
    leg = 0;
    ph = phReady;
    ready = 110;
    endT = 0;
    gatePh = frand(*sv) * 6;
    gateX = 110;
    gateW = kLegs[0].gw;
    gateLit = 0;
    cx = 110;
    cvx = 0;
    cd = 0;
    med = bank > 30 ? bank : 30;
    med0 = med;
    tix = won = drops = 0;
    showerT = medFlash = tixFlash = 0;
    stockN = 0;
    memset(&slot, 0, sizeof(slot));
    memset(&F, 0, sizeof(F));
    fqN = 0;
    carPos = 0;
    rainN = ballN = 0;
    memset(&ava, 0, sizeof(ava));
    winN = 0;
    wfT = 0;
    memset(pops, 0, sizeof(pops));
    bannerT = bannerLife = 0;
    flash = 0;
    shake = 0;
    sessions++;
    ticker("WELCOME TO MEDAL MOUNTAIN! DROP MEDALS AND WATCH THEM POUR!");
    dirtyMask = 7;
    refreshPadLights();
}

void MedalGame::endSession()
{
    bank = med > 9999 ? 9999 : med;
    lifetimeTix += tix;
    lastTix = tix;
    lastMedals = med;
    if (tix > bestTix) bestTix = tix;
    persist();
    ph = phResults;
    dirtyMask = 7;
    refreshPadLights();
}

void MedalGame::legUp(int lg)
{
    leg = (uint8_t)lg;
    char sub[48];
    snprintf(sub, sizeof(sub), "TICKETS X%d%s", kLegs[lg].m, lg ? "  -  THE GATE SPEEDS UP" : "");
    char t1[32];
    snprintf(t1, sizeof(t1), "%s!", kLegs[lg].n);
    banner(t1, sub, lg == 4 ? 0xff60c8 : kSkinC, 130);
    char tk[96];
    snprintf(tk, sizeof(tk), "LEG %d OF 5: %s. TICKETS PAY X%d.", lg + 1, kLegs[lg].n, kLegs[lg].m);
    ticker(tk);
    sv->sfx(Sfx::Big);
    dirtyMask |= 1; // new sky
}

void MedalGame::pop(int16_t x, int16_t y, const char* txt, uint32_t col, int16_t life)
{
    // keep the two newest
    pops[0] = pops[1];
    pops[1] = pops[2];
    Pop& p = pops[2];
    p.x = x;
    p.y = y;
    p.t = 0;
    p.life = life;
    p.col = col;
    strncpy(p.txt, txt, sizeof(p.txt) - 1);
    p.txt[sizeof(p.txt) - 1] = 0;
}

void MedalGame::banner(const char* txt, const char* sub, uint32_t col, int16_t life)
{
    strncpy(bannerTxt, txt, sizeof(bannerTxt) - 1);
    bannerTxt[sizeof(bannerTxt) - 1] = 0;
    strncpy(bannerSub, sub ? sub : "", sizeof(bannerSub) - 1);
    bannerSub[sizeof(bannerSub) - 1] = 0;
    bannerCol = col;
    bannerT = 0;
    bannerLife = life;
}

void MedalGame::ticker(const char* txt)
{
    strncpy(tickerTxt, txt, sizeof(tickerTxt) - 1);
    tickerTxt[sizeof(tickerTxt) - 1] = 0;
    tickerT = 0;
}

void MedalGame::addTix(int32_t n)
{
    tix += n;
    tixFlash = 8;
}

void MedalGame::steer(bool l, bool r)
{
    if (l && !r)
    {
        cvx = cvx - kChAcc < -kChMax ? -kChMax : cvx - kChAcc;
    }
    else if (r && !l)
    {
        cvx = cvx + kChAcc > kChMax ? kChMax : cvx + kChAcc;
    }
    else
    {
        cvx *= .5f;
    }
    cx += cvx;
}

bool MedalGame::tryDrop()
{
    if (cd > 0 || med <= 0 || ph != phGo || ava.on)
    {
        return false;
    }
    cd = kDropCd;
    med--;
    drops++;
    Coin* c = addCoin(0, cx + (frand(*sv) - .5f) * .8f, kDROPY + (frand(*sv) - .5f) * 1.5f, (uint8_t)irand(*sv, 3), 10);
    (void)c;
    sv->sfx(Sfx::Coin);
    if (fabsf(cx - gateX) <= gateW / 2 && !debugNoBonus)
    {
        gateIn();
    }
    return true;
}

void MedalGame::gateIn()
{
    gateLit = 30;
    sv->sfx(Sfx::Select);
    if (stockN < 4)
    {
        stock[stockN++] = 1;
    }
    else
    {
        addTix(1);
    }
}

void MedalGame::onWin(Coin& c)
{
    if (winN < 24) wins[winN++] = t;
    if (c.look >= 10)
    {
        int32_t b = (c.look == 12 ? 10 : 5) * mult();
        addTix(b);
        char s[24];
        snprintf(s, sizeof(s), "BALL! +%ld", (long)b);
        pop(192, 166, s, kSkinC);
        sv->sfx(Sfx::Win);
        return;
    }
    med = med + 1 > 9999 ? 9999 : med + 1;
    won++;
    medFlash = 6;
    if (c.x >= kLane0 && c.x <= kLane1)
    {
        addTix(1);
    }
}

void MedalGame::onLost(Coin& c)
{
    if (c.look >= 10)
    {
        addTix(3);
        ticker("THE BONUS BALL ROLLED HOME. +3 TICKETS");
        return;
    }
    jp = jp + 1 > 300 ? 300 : jp + 1;
    heapT = 8;
}

void MedalGame::waterfall()
{
    wfT = 240;
    int n = winN;
    int32_t b = (n - kWfN + 2) * mult();
    addTix(b);
    char s[24];
    snprintf(s, sizeof(s), "WATERFALL! +%ld", (long)b);
    pop(192, 150, s, 0x80e0ff, 70);
    shake = shake > 2 ? shake : 2;
    sv->sfx(Sfx::Win);
}

//==============================================================================================================
// The mountain reels and bonus features
//==============================================================================================================
void MedalGame::pickOutcome(uint8_t& kind, uint8_t sym[3])
{
    float r = frand(*sv) * 100;
    kind = oMiss;
    for (int k = 0; k < 11; k++)
    {
        r -= kOutW[k];
        if (r < 0)
        {
            kind = (uint8_t)k;
            break;
        }
    }
    if (debugForceOut >= 0)
    {
        kind = (uint8_t)debugForceOut;
        debugForceOut = -1;
    }
    auto shuffled3 = [&](uint8_t a, uint8_t b, uint8_t c) {
        uint8_t v[3] = { a, b, c };
        for (int i = 2; i > 0; i--)
        {
            int j = irand(*sv, i + 1);
            uint8_t tmp = v[i];
            v[i] = v[j];
            v[j] = tmp;
        }
        memcpy(sym, v, 3);
    };
    const uint8_t symFor[9] = { sMtn, sCar, sSun, sBunny, sBear, sBird, sApple, sGrape, sPeach };
    if (kind == oSalad)
    {
        shuffled3(sApple, sGrape, sPeach);
    }
    else if (kind == oZoo)
    {
        shuffled3(sBunny, sBear, sBird);
    }
    else if (kind != oMiss)
    {
        sym[0] = sym[1] = sym[2] = symFor[kind];
    }
    else
    {
        for (;;)
        {
            for (int i = 0; i < 3; i++) sym[i] = (uint8_t)irand(*sv, 9);
            if (sym[0] == sym[1] && sym[1] == sym[2]) continue;
            uint8_t s2[3] = { sym[0], sym[1], sym[2] };
            // sort
            for (int a = 0; a < 2; a++)
                for (int b = a + 1; b < 3; b++)
                    if (s2[b] < s2[a])
                    {
                        uint8_t tmp = s2[a];
                        s2[a] = s2[b];
                        s2[b] = tmp;
                    }
            if (s2[0] == 0 && s2[1] == 1 && s2[2] == 2) continue;
            if (s2[0] == 3 && s2[1] == 4 && s2[2] == 5) continue;
            break;
        }
    }
}

void MedalGame::stepSlot()
{
    Slot& s = slot;
    if (s.st == 0)
    {
        if (stockN > 0 && !ava.on && ph != phReady)
        {
            for (int i = 0; i < stockN - 1; i++) stock[i] = stock[i + 1];
            stockN--;
            pickOutcome(s.outKind, s.sym);
            s.st = 1;
            s.t = 0;
            memset(s.done, 0, sizeof(s.done));
            memset(s.bump, 0, sizeof(s.bump));
            s.win = false;
            sv->sfx(Sfx::Tick);
        }
        return;
    }
    s.t++;
    for (int i = 0; i < 3; i++)
    {
        if (s.done[i]) continue;
        s.pos[i] = fmodf(s.pos[i] + .34f, 9);
        if (s.t >= kReelStop[i])
        {
            s.done[i] = true;
            for (int k = 0; k < 9; k++)
            {
                if (kStrip[i][k] == s.sym[i]) s.pos[i] = (float)k;
            }
            s.bump[i] = 6;
            sv->sfx(Sfx::Tick);
        }
    }
    for (int i = 0; i < 3; i++)
    {
        if (s.bump[i] > 0) s.bump[i]--;
    }
    if (s.st == 1 && s.done[0] && s.done[1] && s.done[2])
    {
        s.st = 2;
        s.t = 0;
        slotPay();
    }
    else if (s.st == 2 && s.t > (s.outKind == oMiss ? 16 : 50))
    {
        s.st = 0;
    }
}

void MedalGame::slotPay()
{
    uint8_t k = slot.outKind;
    int m = mult();
    slot.win = k != oMiss;
    char tk[64];
    switch (k)
    {
        case oMtn: avalanche(); return;
        case oCar:
        case oSun:
            if (fqN < 6) fq[fqN++] = k == oCar ? 1 : 0;
            sv->sfx(Sfx::Right);
            return;
        case oBunny: dropBall(10, 0); sv->sfx(Sfx::Right); return;
        case oBird: dropBall(12, 0); sv->sfx(Sfx::Right); return;
        case oBear:
            dropBall(11, 0);
            dropBall(11, 50);
            sv->sfx(Sfx::Right);
            return;
        case oApple:
        case oGrape:
        case oPeach:
        {
            int n = k == oApple ? 6 : k == oGrape ? 9 : 12;
            if (rainN < 6) rain[rainN++] = Rain{ (int16_t)n, 0 };
            snprintf(tk, sizeof(tk), "MEDAL RAIN! %d MEDALS FALL ON THE SHELF.", n);
            ticker(tk);
            sv->sfx(Sfx::Win);
            return;
        }
        case oSalad:
        case oZoo:
        {
            int32_t b = (k == oSalad ? 3 : 5) * m;
            addTix(b);
            snprintf(tk, sizeof(tk), "%s+%ld TICKETS", k == oSalad ? "FRUIT SALAD! " : "ZOO PARADE! ", (long)b);
            ticker(tk);
            sv->sfx(Sfx::Right);
            return;
        }
        default: return;
    }
}

void MedalGame::stepRain()
{
    for (int i = 0; i < rainN; i++)
    {
        Rain& r = rain[i];
        if (r.t++ % 4 == 0 && r.n > 0)
        {
            r.n--;
            addCoin(0, 12 + frand(*sv) * 196, 18 + frand(*sv) * 38, (uint8_t)irand(*sv, 3), 14);
        }
    }
    int w = 0;
    for (int i = 0; i < rainN; i++)
    {
        if (rain[i].n > 0) rain[w++] = rain[i];
    }
    rainN = (uint8_t)w;
}

void MedalGame::dropBall(uint8_t look, int16_t delay)
{
    if (ballN < 4) balls[ballN++] = Ball{ look, (int16_t)-delay };
}

void MedalGame::startFeat(uint8_t kind)
{
    memset(&F, 0, sizeof(F));
    F.active = true;
    F.kind = kind;
    F.i = (uint8_t)irand(*sv, 12);
    F.iv = 2;
    F.v = .035f;
    F.dir = 1;
    F.tg = -1;
    if (kind == 0)
    {
        banner("SKY ROULETTE!", "PRESS PLAY OR A PAD TO STOP THE LIGHT", 0x40c0ff, 90);
    }
    else
    {
        banner("CABLE CAR BONUS!", "PRESS PLAY OR A PAD TO STOP AT A STATION", 0xff5050, 90);
        F.pos = carPos;
    }
    sv->sfx(Sfx::Big);
}

void MedalGame::featPress()
{
    if (!F.active || F.stop) return;
    F.stop = true;
    sv->sfx(Sfx::Confirm);
    if (F.kind == 0) F.left = kRouSteps;
}

void MedalGame::rouPay()
{
    const RouPrize& p = kRou[F.i];
    int m = mult();
    char s[24];
    if (p.t)
    {
        addTix(p.t * m);
        snprintf(s, sizeof(s), "+%d TICKETS", p.t * m);
        pop(276, 44, s, kSkinC);
    }
    else if (p.m)
    {
        if (rainN < 6) rain[rainN++] = Rain{ p.m, 0 };
        snprintf(s, sizeof(s), "MEDAL RAIN %d", p.m);
        pop(276, 44, s, 0x80e0ff);
    }
    else if (p.car)
    {
        if (fqN < 6) fq[fqN++] = 1;
    }
    sv->sfx(Sfx::Win);
}

void MedalGame::carPay()
{
    const CarStop& s = kCar[F.tg];
    int m = mult();
    char b[24];
    if (s.t)
    {
        addTix(s.t * m);
        snprintf(b, sizeof(b), "+%d TICKETS", s.t * m);
        pop(F.tg < 3 ? 140 : 200, 30, b, kSkinC);
    }
    else if (s.m)
    {
        if (rainN < 6) rain[rainN++] = Rain{ s.m, 0 };
        snprintf(b, sizeof(b), "MEDAL RAIN %d", s.m);
        pop(150, 30, b, 0x80e0ff);
    }
    else
    {
        if (rainN < 6) rain[rainN++] = Rain{ 20, 0 };
        addTix(20 * m);
        snprintf(b, sizeof(b), "+%d TICKETS AND 20 MEDALS", 20 * m);
        banner("THE SUMMIT!", b, kSkinC, 110);
    }
    sv->sfx(Sfx::Win);
}

void MedalGame::avalanche()
{
    int n = jp < 90 ? jp : 90;
    ava.on = true;
    ava.t = 0;
    ava.n = (int16_t)n;
    ava.left = (int16_t)n;
    ava.b = (int32_t)(jp / 2.0f + .5f) * mult();
    shake = 4;
    flash = .5f;
    banner("THE AVALANCHE!", "THE MOUNTAIN LETS GO!", kSkinC, 150);
    sv->sfx(Sfx::Big);
}

void MedalGame::stepAva()
{
    if (!ava.on) return;
    ava.t++;
    if (ava.t % 10 == 0 && ava.t < 70) shake = shake > 2 ? shake : 2;
    if (ava.t >= 50 && ava.left > 0)
    {
        int k = ava.t < 90 ? 1 : 2;
        for (int i = 0; i < k && ava.left > 0; i++)
        {
            ava.left--;
            float spread = (ava.t - 40) / 50.0f;
            if (spread > 1) spread = 1;
            float x = 110 + (frand(*sv) - .5f) * 200 * spread;
            if (x < 8) x = 8;
            if (x > 212) x = 212;
            addCoin(0, x, 18 + frand(*sv) * 40, (uint8_t)irand(*sv, 3), 16);
        }
    }
    if (ava.t == 140)
    {
        addTix(ava.b);
        char s[96];
        snprintf(s, sizeof(s), "THE AVALANCHE! +%ld TICKETS. THE MOUNTAIN STARTS TO GROW AGAIN.", (long)ava.b);
        ticker(s);
        snprintf(s, sizeof(s), "+%ld TICKETS", (long)ava.b);
        pop(192, 100, s, kSkinC, 90);
    }
    if (ava.left <= 0 && ava.t > 160)
    {
        jp = 50;
        ava.on = false;
    }
}

void MedalGame::stepFeat()
{
    stepSlot();
    stepRain();
    // bonus balls drop in after their delay
    for (int i = 0; i < ballN; i++)
    {
        if (++balls[i].t == 1)
        {
            addCoin(0, 30 + frand(*sv) * 160, 40 + frand(*sv) * 12, balls[i].look, 14);
            sv->sfx(Sfx::Coin);
        }
    }
    int w = 0;
    for (int i = 0; i < ballN; i++)
    {
        if (balls[i].t < 1) balls[w++] = balls[i];
    }
    ballN = (uint8_t)w;
    stepAva();
    if (ava.on) return;

    if (!F.active)
    {
        if (fqN > 0 && slot.st != 1)
        {
            uint8_t k = fq[0];
            for (int i = 0; i < fqN - 1; i++) fq[i] = fq[i + 1];
            fqN--;
            startFeat(k);
        }
        return;
    }
    F.t++;
    if (F.kind == 0)
    {
        if (F.done)
        {
            if (F.t > F.done + 70) F.active = false;
            return;
        }
        if (!F.stop && F.t > 300) featPress();
        if (--F.nt <= 0)
        {
            F.i = (uint8_t)((F.i + 1) % 12);
            sv->sfx(Sfx::Tick);
            if (F.stop)
            {
                F.iv += 1.1f;
                if (--F.left <= 0)
                {
                    F.done = F.t;
                    rouPay();
                    return;
                }
            }
            F.nt = (int16_t)(F.iv + .5f);
        }
    }
    else
    {
        if (F.done)
        {
            if (F.t > F.done + 80)
            {
                F.active = false;
                carPos = 0;
            }
            else
            {
                carPos = carPos - .04f > 0 ? carPos - .04f : 0;
            }
            return;
        }
        if (!F.stop)
        {
            F.v = .035f + F.t * .00025f;
            if (F.v > .11f) F.v = .11f;
            if (F.t > 420) featPress();
        }
        else
        {
            if (F.tg < 0)
            {
                F.tg = F.dir > 0 ? (int8_t)fminf(5, ceilf(F.pos + .25f)) : (int8_t)fmaxf(0, floorf(F.pos - .25f));
            }
            F.v = F.v * .9f > .012f ? F.v * .9f : .012f;
        }
        F.pos += F.v * F.dir;
        if (F.pos >= 5)
        {
            F.pos = 5;
            if (!F.stop) F.dir = -1;
        }
        if (F.pos <= 0)
        {
            F.pos = 0;
            if (!F.stop) F.dir = 1;
        }
        if (F.stop && F.tg >= 0 &&
            ((F.dir > 0 && F.pos >= F.tg) || (F.dir < 0 && F.pos <= F.tg) || F.pos == F.tg))
        {
            F.pos = F.tg;
            F.done = F.t;
            carPay();
        }
        carPos = F.pos;
    }
}

//==============================================================================================================
// Frame step
//==============================================================================================================
void MedalGame::stepPlay()
{
    if (ph == phReady)
    {
        if (--ready <= 0)
        {
            ph = phGo;
            flash = .3f;
            sv->sfx(Sfx::Big);
            refreshPadLights();
        }
    }
    else if (ph == phGo)
    {
        t++;
        left--;
        int lg = t / kLegT;
        if (lg > kNLeg - 1) lg = kNLeg - 1;
        if (lg != leg) legUp(lg);
        if (left == 600) ticker("TEN SECONDS LEFT ON THE MOUNTAIN CLOCK!");
        if (left <= 600 && left % 60 == 0 && left > 0) sv->sfx(Sfx::Tick);
        if (left <= 0)
        {
            ph = phFinal;
            endT = 0;
            banner("TIME UP!", "THE SHELVES GIVE ONE LAST PUSH...", 0x40c0ff, 150);
            refreshPadLights();
        }
    }
    else if (ph == phFinal)
    {
        if (++endT > 330 && !F.active && fqN == 0 && !ava.on && slot.st == 0)
        {
            endSession();
            return;
        }
    }

    // the CHECK gate swings along the back of the upper shelf
    const Leg& L = kLegs[leg];
    gatePh += L.gs;
    gateX = 110 + 94 * sinf(gatePh);
    gateW = L.gw;
    if (gateLit > 0) gateLit--;

    // the chute
    if (cd > 0) cd--;
    if (medFlash > 0) medFlash--;
    if (tixFlash > 0) tixFlash--;
    if (ph == phGo || (ph == phFinal && F.active))
    {
        bool featOwn = F.active && !F.stop;
        if (jogAcc != 0)
        {
            cx += jogAcc * 3.0f;
            cvx = 0;
            jogAcc = 0;
        }
        if (padTarget >= 0)
        {
            float target = 8 + padTarget * (204.0f / 11.0f);
            float d = target - cx;
            cx += fabsf(d) < 6 ? d : (d > 0 ? 6 : -6);
            cvx = 0;
        }
        else
        {
            steer(leftHeld, rightHeld);
        }
        if (cx < 8) cx = 8, cvx = 0;
        if (cx > 212) cx = 212, cvx = 0;

        if (featOwn)
        {
            if (dropPressed) featPress();
            holdT = 0;
        }
        else if (dropPressed)
        {
            // a pad press drops once the chute has reached its column
            if (padTarget < 0 || fabsf(8 + padTarget * (204.0f / 11.0f) - cx) < 1)
            {
                tryDrop();
                dropPressed = false;
            }
            holdT = 0;
        }
        else if (dropHeld)
        {
            if (++holdT > kHoldDelay && holdT % kHoldRate == 0) tryDrop();
        }
        else
        {
            holdT = 0;
        }
        if (featOwn) dropPressed = false;
    }
    else
    {
        jogAcc = 0;
        dropPressed = false;
    }

    // medal shower when you run dry
    bool ownInAir = false;
    for (int i = 0; i < coinCount; i++)
    {
        if (!coins[i].dead && coins[i].tier == 0 && coins[i].air > 4) ownInAir = true;
    }
    if (med <= 0 && ph == phGo && !ownInAir)
    {
        if (++showerT == 50)
        {
            med += 10;
            medFlash = 30;
            ticker("MEDAL SHOWER! THE MOUNTAIN GIVES TEN FREE MEDALS.");
            sv->sfx(Sfx::Win);
        }
    }
    else
    {
        showerT = 0;
    }

    stepShelf(0, false);
    stepShelf(1, false);
    stepFeat();

    // waterfalls: 12+ medals won within one second
    int w = 0;
    for (int i = 0; i < winN; i++)
    {
        if (wins[i] >= t - 60) wins[w++] = wins[i];
    }
    winN = (uint8_t)w;
    if (wfT > 0) wfT--;
    if (winN >= kWfN && wfT <= 0 && ph != phReady) waterfall();

    if (frame % 8 == 0) compact();

    for (int i = 0; i < 3; i++)
    {
        if (pops[i].life > 0 && ++pops[i].t >= pops[i].life) pops[i].life = 0;
    }
    if (bannerLife > 0 && ++bannerT > bannerLife) bannerLife = 0;
    tickerT++;
    if (flash > 0) flash -= .02f;
    if (shake > 0 && frame % 4 == 0) shake--;
    if (heapT > 0) heapT--;
}

void MedalGame::tick(uint32_t nowMs)
{
    if (lastMs == 0) lastMs = nowMs;
    uint32_t dt = nowMs - lastMs;
    lastMs = nowMs;
    if (dt > 120) dt = 120;
    accMs += dt;

    int steps = 0;
    int chuteColBefore = (int)((cx - 8) / (204.0f / 11.0f) + .5f);
    while (accMs >= kFrameMs && steps < 4)
    {
        accMs -= kFrameMs;
        frame++;
        steps++;
        if (ph == phTitle || ph == phResults)
        {
            if (startPressed)
            {
                startPressed = false;
                newSession();
            }
            continue;
        }
        stepPlay();
    }
    if (accMs > kFrameMs * 4) accMs = 0; // too far behind: drop time rather than spiral
    if (steps > 0) dirtyMask |= 6;
    int chuteCol = (int)((cx - 8) / (204.0f / 11.0f) + .5f);
    if (chuteCol != chuteColBefore) refreshPadLights();
    if (ph != bgPhase || (bgLeg != leg && ph != phTitle))
    {
        dirtyMask |= 1;
    }
}

void MedalGame::debugPrint()
{
    int n[2] = { 0, 0 };
    for (int i = 0; i < coinCount; i++)
    {
        if (!coins[i].dead) n[coins[i].tier]++;
    }
    printf("medal: phase %d t %ld leg %d med %ld tix %ld won %ld drops %ld jp %ld coins %d/%d bank %ld stock %d slot %d feat %d ava %d\n",
           ph, (long)t, leg, (long)med, (long)tix, (long)won, (long)drops, (long)jp, n[0], n[1], (long)bank, stockN, slot.st,
           F.active, ava.on);
}

//==============================================================================================================
// Drawing
//==============================================================================================================
void MedalGame::draw(Gfx& g, uint8_t layer)
{
    dirtyMask &= (uint8_t)~(1u << layer);
    if (layer == 0)
    {
        bgLeg = leg;
        bgPhase = ph;
        drawBackground(g);
    }
    else if (layer == 1)
    {
        if (ph == phTitle || ph == phResults)
        {
            drawTitle(g);
        }
        else
        {
            drawField(g);
        }
    }
    else
    {
        if (ph != phTitle && ph != phResults)
        {
            drawForeground(g);
        }
    }
}

void MedalGame::drawBackground(Gfx& g)
{
    // cabinet body
    g.color(0x1a0a10);
    g.rect(kCanvasX, kCanvasY, kCanvasX + kCanvasW, kCanvasY + kCanvasH);
    if (ph == phTitle || ph == phResults)
    {
        // title: sunset sky and mountains behind the logo
        const Leg& L = kLegs[3];
        for (int i = 0; i < 5; i++)
        {
            g.color(L.sky[i]);
            g.rect(kCanvasX, (int16_t)(kCanvasY + i * 50), kCanvasX + kCanvasW, (int16_t)(kCanvasY + (i + 1) * 50 + 1));
        }
        g.clip(kCanvasX, kCanvasY, kCanvasW, kCanvasH);
        const int16_t far[] = { 0, 220, 120, 150, 260, 200, 400, 110, 560, 190, 680, 140, 800, 210 };
        int16_t near[] = { 0, 300, 160, 230, 300, 280, 420, 200, 600, 290, 800, 250 };
        for (int i = 1; i < 12; i += 2) near[i] = (int16_t)(near[i] + kCanvasY);
        int16_t far2[14];
        for (int i = 0; i < 14; i++) far2[i] = (int16_t)(far[i] + (i & 1 ? kCanvasY : 0));
        g.color(L.far);
        g.fillBelow(far2, 7);
        g.color(L.mtn2);
        g.fillBelow(near, 6);
        g.unclip();
        return;
    }

    const Leg& L = kLegs[leg];
    // sky and mountains in the diorama window (original 82..302 x 0..64)
    int16_t x0 = X(kFX), x1 = X(kFX + kTW), y0 = Y(0), y1 = Y(64);
    for (int i = 0; i < 5; i++)
    {
        g.color(L.sky[i]);
        g.rect(x0, (int16_t)(y0 + (y1 - y0) * i / 5), x1, (int16_t)(y0 + (y1 - y0) * (i + 1) / 5 + 1));
    }
    g.clip(x0, y0, x1 - x0, y1 - y0);
    g.color(L.sun);
    g.circle(X(268), Y(16), S(L.night ? 7 : 9));
    int16_t far[] = { X(82), Y(44), X(110), Y(30), X(140), Y(38), X(170), Y(22), X(200), Y(34), X(240), Y(26), X(280), Y(36), X(302), Y(30) };
    g.color(L.far);
    g.fillBelow(far, 8);
    // the mountain itself, summit at the heap (x 192)
    int16_t mtn[] = { X(82), Y(62), X(130), Y(36), X(160), Y(24), X(192), Y(8), X(224), Y(26), X(262), Y(40), X(302), Y(56) };
    g.color(L.mtn);
    g.fillBelow(mtn, 7);
    int16_t mtn2[] = { X(82), Y(64), X(120), Y(54), X(150), Y(58), X(190), Y(46), X(232), Y(56), X(270), Y(50), X(302), Y(60) };
    g.color(L.mtn2);
    g.fillBelow(mtn2, 7);
    // snow cap
    int16_t cap[] = { X(176), Y(16), X(184), Y(12), X(192), Y(8), X(200), Y(12), X(208), Y(16) };
    g.color(0xffffff);
    g.fillBelow(cap, 5);
    g.color(L.mtn);
    int16_t capCut[] = { X(176), Y(17), X(184), Y(15), X(192), Y(16), X(200), Y(14), X(208), Y(18) };
    g.fillBelow(capCut, 5);
    // pines
    g.color(L.tree);
    for (int i = 0; i < 9; i++)
    {
        int16_t tx = X(90 + i * 25 + (i % 3) * 4), ty = Y(58 - (i % 2) * 3);
        int16_t tri[] = { (int16_t)(tx - 7), (int16_t)(ty + 2), tx, (int16_t)(ty - 14), (int16_t)(tx + 7), (int16_t)(ty + 2) };
        g.fillBelow(tri, 3);
    }
    g.unclip();

    // cable car wire and stations (local wire: x 14..106, y 50..10, +76)
    g.color(0x303040);
    for (int i = 0; i < 12; i++)
    {
        float a = i / 12.0f, b = (i + 1) / 12.0f;
        float ax = 76 + 14 + 92 * a, ay = 50 - 40 * a + sinf(3.14159f * a) * 3;
        float bx = 76 + 14 + 92 * b, by = 50 - 40 * b + sinf(3.14159f * b) * 3;
        g.line(X(ax), Y(ay), X(bx), Y(by), 2);
    }
    for (int k = 0; k < 6; k++)
    {
        float a = k / 5.0f;
        float sx = 76 + 14 + 92 * a, syy = 50 - 40 * a + sinf(3.14159f * a) * 3;
        g.color(k == 5 ? kSkinC : 0xf8f0e0);
        g.rect((int16_t)(X(sx) - 12), (int16_t)(Y(syy) + 6), (int16_t)(X(sx) + 12), (int16_t)(Y(syy) + 20), 3);
        g.color(0x2a1206);
        g.text(X(sx), (int16_t)(Y(syy) + 6), kFont12, kCenterX, kCar[k].l);
    }

    // cabinet frame around the field
    g.color(kSkinA);
    g.rect(X(76), Y(64), X(308), Y(214), 6);
    g.color(0x2a0a10);
    g.rect(X(80), Y(66), X(304), Y(210), 4);

    for (uint8_t tier = 0; tier < 2; tier++)
    {
        float ty = kTY[tier];
        // shelf bed
        g.color(tier ? 0x3a2a50 : 0x42305a);
        g.rect(X(kFX), Y(ty - 4), X(kFX + kTW), Y(ty + kE[tier] * kKY + 2));
        // front lip
        g.color(kSkinB);
        g.rect(X(kFX - 2), Y(ty + kE[tier] * kKY + 1), X(kFX + kTW + 2), Y(ty + kE[tier] * kKY + 4), 1);
    }
    // lucky lane on the lower lip and the side pockets
    g.color(kSkinC);
    g.rect(X(kFX + kLane0), Y(kTY[1] + kE[1] * kKY + 1), X(kFX + kLane1), Y(kTY[1] + kE[1] * kKY + 4), 1);
    g.color(0x0a0408);
    g.rect(X(kFX - 4), Y(kTY[1] + kPOCK * kKY), X(kFX), Y(kTY[1] + kE[1] * kKY + 4));
    g.rect(X(kFX + kTW), Y(kTY[1] + kPOCK * kKY), X(kFX + kTW + 4), Y(kTY[1] + kE[1] * kKY + 4));
    // payout tray
    g.color(0x14080c);
    g.rect(X(86), Y(190), X(298), Y(208), 4);
    g.color(kSkinC);
    g.text(X(192), Y(kTY[1] + kE[1] * kKY + 4), kFont12, kCenterX, "LUCKY LANE");

    // side panels
    g.color(0x2a0a14);
    g.rect(X(2), Y(64), X(74), Y(214), 6);
    g.rect(X(310), Y(2), X(382), Y(214), 6);
    g.rect(X(2), Y(2), X(74), Y(60), 6);
    g.color(kSkinC);
    g.text(X(38), Y(68), kFont12, kCenterX, "MEDALS");
    g.text(X(38), Y(100), kFont12, kCenterX, "TICKETS");
    g.text(X(38), Y(134), kFont12, kCenterX, "LEG");
    g.text(X(38), Y(172), kFont12, kCenterX, "CLOCK");
    g.text(X(38), Y(6), kFont12, kCenterX, "MOUNTAIN");
    g.text(X(346), Y(6), kFont12, kCenterX, "REELS");
    g.text(X(346), Y(61), kFont12, kCenterX, "SKY ROULETTE");
    g.text(X(346), Y(150), kFont12, kCenterX, "CHECK GATE");
}

void MedalGame::drawSymbol(Gfx& g, uint8_t sym, int16_t cx0, int16_t cy0, int16_t s)
{
    switch (sym)
    {
        case sApple:
            g.color(0xe82828);
            g.circle(cx0, cy0 + 1, s);
            g.color(0x40a040);
            g.rect(cx0, (int16_t)(cy0 - s - 3), (int16_t)(cx0 + 5), (int16_t)(cy0 - s + 1), 2);
            break;
        case sGrape:
        {
            g.color(0x8838c8);
            int16_t xy[] = { (int16_t)(cx0 - 4), (int16_t)(cy0 - 4), (int16_t)(cx0 + 4), (int16_t)(cy0 - 4), cx0, (int16_t)(cy0 + 2),
                             (int16_t)(cx0 - 5), (int16_t)(cy0 + 3), (int16_t)(cx0 + 5), (int16_t)(cy0 + 3), cx0, (int16_t)(cy0 + 8) };
            g.points(xy, 6, (int16_t)(s / 2));
            break;
        }
        case sPeach:
            g.color(0xffa080);
            g.circle(cx0, cy0, s);
            g.color(0xff7060);
            g.circle((int16_t)(cx0 + 3), cy0, (int16_t)(s - 4));
            break;
        case sBunny:
            g.color(0xf8f0f8);
            g.rect((int16_t)(cx0 - 5), (int16_t)(cy0 - s - 6), (int16_t)(cx0 - 1), cy0, 2);
            g.rect((int16_t)(cx0 + 1), (int16_t)(cy0 - s - 6), (int16_t)(cx0 + 5), cy0, 2);
            g.circle(cx0, (int16_t)(cy0 + 2), (int16_t)(s - 2));
            g.color(0xff70b0);
            g.circle(cx0, (int16_t)(cy0 + 3), 2);
            break;
        case sBear:
            g.color(0x8a5a30);
            g.circle((int16_t)(cx0 - 6), (int16_t)(cy0 - 6), 4);
            g.circle((int16_t)(cx0 + 6), (int16_t)(cy0 - 6), 4);
            g.circle(cx0, cy0, s - 1);
            g.color(0xd8b080);
            g.circle(cx0, (int16_t)(cy0 + 3), 4);
            break;
        case sBird:
            g.color(0xffc020);
            g.circle(cx0, cy0, s - 1);
            g.color(0xff7010);
            g.rect((int16_t)(cx0 + s - 3), (int16_t)(cy0 - 2), (int16_t)(cx0 + s + 3), (int16_t)(cy0 + 2), 1);
            g.color(0x200810);
            g.circle((int16_t)(cx0 + 3), (int16_t)(cy0 - 3), 2);
            break;
        case sCar:
            g.color(0x4a4a58);
            g.rect((int16_t)(cx0 - 1), (int16_t)(cy0 - s - 2), (int16_t)(cx0 + 1), (int16_t)(cy0 - s + 4));
            g.color(0xe02838);
            g.rect((int16_t)(cx0 - s), (int16_t)(cy0 - s + 4), (int16_t)(cx0 + s), (int16_t)(cy0 + s), 2);
            g.color(0xa8e0ff);
            g.rect((int16_t)(cx0 - s + 3), (int16_t)(cy0 - s + 7), (int16_t)(cx0 + s - 3), (int16_t)(cy0 - 1), 1);
            break;
        case sSun:
            g.color(0xffd020);
            for (int i = 0; i < 8; i++)
            {
                float a = i * 3.14159f / 4;
                g.line((int16_t)(cx0 + cosf(a) * (s - 2)), (int16_t)(cy0 + sinf(a) * (s - 2)),
                       (int16_t)(cx0 + cosf(a) * (s + 3)), (int16_t)(cy0 + sinf(a) * (s + 3)), 2);
            }
            g.circle(cx0, cy0, (int16_t)(s - 3));
            break;
        case sMtn:
        {
            int16_t tri[] = { (int16_t)(cx0 - s - 2), (int16_t)(cy0 + s), cx0, (int16_t)(cy0 - s), (int16_t)(cx0 + s + 2), (int16_t)(cy0 + s) };
            g.clip((int16_t)(cx0 - s - 3), (int16_t)(cy0 - s - 2), (int16_t)(2 * s + 6), (int16_t)(2 * s + 3));
            g.color(0x5a7cb0);
            g.fillBelow(tri, 3);
            int16_t snow[] = { (int16_t)(cx0 - 5), (int16_t)(cy0 - s + 7), cx0, (int16_t)(cy0 - s), (int16_t)(cx0 + 5), (int16_t)(cy0 - s + 7) };
            g.clip((int16_t)(cx0 - 6), (int16_t)(cy0 - s), 12, 8);
            g.color(0xffffff);
            g.fillBelow(snow, 3);
            g.unclip();
            break;
        }
    }
}

void MedalGame::drawSlot(Gfx& g)
{
    // three reel windows in the right panel
    for (int i = 0; i < 3; i++)
    {
        int16_t wx = (int16_t)(X(318) + i * 40), wy = Y(16);
        g.color(slot.st == 2 && slot.win ? kSkinC : 0xf8f0e0);
        g.rect(wx, wy, (int16_t)(wx + 36), (int16_t)(wy + 48), 4);
        g.color(0x0c0a10);
        g.rect((int16_t)(wx + 2), (int16_t)(wy + 2), (int16_t)(wx + 34), (int16_t)(wy + 46), 3);
        uint8_t sym;
        if (slot.st == 0 && slot.pos[i] == 0 && slot.sym[i] == 0)
        {
            sym = kStrip[i][0];
        }
        else
        {
            sym = kStrip[i][((int)slot.pos[i]) % 9];
        }
        g.clip((int16_t)(wx + 2), (int16_t)(wy + 2), 32, 44);
        int16_t bob = slot.bump[i] ? (int16_t)(slot.bump[i] / 2) : 0;
        if (slot.st == 1 && !slot.done[i])
        {
            float f = slot.pos[i] - floorf(slot.pos[i]);
            bob = (int16_t)(f * 22 - 11);
        }
        drawSymbol(g, sym, (int16_t)(wx + 18), (int16_t)(wy + 24 + bob), 10);
        g.unclip();
    }
    // queued spins
    for (int i = 0; i < 4; i++)
    {
        g.color(i < stockN ? kSkinC : 0x40303a);
        g.circle((int16_t)(X(328) + i * 28), Y(56), 5);
    }
}

void MedalGame::drawRoulette(Gfx& g)
{
    int16_t rcx = X(346), rcy = Y(108);
    for (int i = 0; i < 12; i++)
    {
        float a = -3.14159f / 2 + i * 3.14159f / 6;
        int16_t lx = (int16_t)(rcx + cosf(a) * 45), ly = (int16_t)(rcy + sinf(a) * 45);
        bool lit = F.active && F.kind == 0 && F.i == i;
        bool blink = lit && F.done && ((frame >> 3) & 1);
        g.color(lit && !blink ? 0xffffff : kRouC[i % 4]);
        g.circle(lx, ly, lit ? 14 : 12);
        const RouPrize& p = kRou[i];
        char b[6];
        if (p.t) snprintf(b, sizeof(b), "%d", p.t);
        else if (p.m) snprintf(b, sizeof(b), "M%d", p.m);
        else snprintf(b, sizeof(b), "C");
        g.color(0x200818);
        g.text(lx, (int16_t)(ly - 7), kFont12, kCenterX, b);
    }
    g.color(kLegs[leg].sky[0]);
    g.circle(rcx, rcy, 22);
    g.color(0xffffff);
    char m[8];
    snprintf(m, sizeof(m), "X%d", mult());
    g.text(rcx, (int16_t)(rcy - 9), kFont17B, kCenterX, m);
}

void MedalGame::drawField(Gfx& g)
{
    int16_t ox = (int16_t)(shake ? ((frame & 1) ? shake : -shake) : 0);

    for (uint8_t tier = 0; tier < 2; tier++)
    {
        const Shelf& sh = P[tier];
        float ty = kTY[tier];
        // pusher plate
        g.color(0x9aa0b8);
        g.rect((int16_t)(X(kFX) + ox), Y(ty - 6), (int16_t)(X(kFX + kTW) + ox), Y(ty + sh.front * kKY), 2);
        g.color(0x6a7088);
        g.rect((int16_t)(X(kFX) + ox), Y(ty + sh.front * kKY - 2), (int16_t)(X(kFX + kTW) + ox), Y(ty + sh.front * kKY), 1);

        // coins on this tier, settled ones first, falling ones on top
        for (int pass = 0; pass < 2; pass++)
        {
            int n = 0;
            for (int i = 0; i < coinCount; i++)
            {
                const Coin& c = coins[i];
                if (c.dead || c.tier != tier || c.look >= 10) continue;
                if ((c.air > 0) != (pass == 1)) continue;
                float z = c.air ? c.air * (tier == 0 ? 2.2f : 1.6f) : 0;
                s_xy[2 * n] = (int16_t)(X(kFX + c.x) + ox);
                s_xy[2 * n + 1] = Y(ty + c.y * kKY - z);
                n++;
            }
            if (n == 0) continue;
            g.color(kOL);
            g.points(s_xy, n, 10);
            for (uint8_t look = 0; look < 3; look++)
            {
                int m = 0;
                for (int i = 0; i < coinCount; i++)
                {
                    const Coin& c = coins[i];
                    if (c.dead || c.tier != tier || c.look != look) continue;
                    if ((c.air > 0) != (pass == 1)) continue;
                    float z = c.air ? c.air * (tier == 0 ? 2.2f : 1.6f) : 0;
                    s_xy[2 * m] = (int16_t)(X(kFX + c.x) + ox);
                    s_xy[2 * m + 1] = Y(ty + c.y * kKY - z);
                    m++;
                }
                if (m == 0) continue;
                g.color(kMedalCol[look]);
                g.points(s_xy, m, 8);
                for (int k = 0; k < m; k++)
                {
                    s_xy[2 * k] -= 2;
                    s_xy[2 * k + 1] -= 2;
                }
                g.color(kMedalHi[look]);
                g.points(s_xy, m, 3);
            }
        }
        // bonus balls
        for (int i = 0; i < coinCount; i++)
        {
            const Coin& c = coins[i];
            if (c.dead || c.tier != tier || c.look < 10) continue;
            float z = c.air ? c.air * 2.0f : 0;
            int16_t bx = (int16_t)(X(kFX + c.x) + ox), by = Y(ty + c.y * kKY - z);
            g.color(kOL);
            g.circle(bx, by, 16);
            g.color(kBallCol[c.look - 10]);
            g.circle(bx, by, 14);
            g.color(0xffffff);
            g.circle((int16_t)(bx - 5), (int16_t)(by - 5), 4);
        }
    }

}

void MedalGame::drawRail(Gfx& g)
{
    // CHECK gate on the rail above the upper shelf
    int16_t gx0 = X(kFX + gateX - gateW / 2), gx1 = X(kFX + gateX + gateW / 2);
    g.color(gateLit ? 0xffffff : 0xffe070);
    g.rect(gx0, Y(70), gx1, Y(76), 3);
    g.color(0x200818);
    g.text((int16_t)((gx0 + gx1) / 2 + 1), (int16_t)(Y(70) - 15), kFont12, kCenterX, "CHECK");
    g.color(gateLit ? 0xffffff : 0xffe070);
    g.text((int16_t)((gx0 + gx1) / 2), (int16_t)(Y(70) - 16), kFont12, kCenterX, "CHECK");

    // the chute
    if (ph == phGo || ph == phReady || ph == phFinal)
    {
        int16_t chx = X(kFX + cx);
        bool inGate = fabsf(cx - gateX) <= gateW / 2;
        g.color(0x6a6a7a);
        g.rect(X(kFX), Y(64), X(kFX + kTW), Y(66));
        g.color(inGate ? kSkinC : 0xe8e8f0);
        g.rect((int16_t)(chx - 12), Y(58), (int16_t)(chx + 12), Y(68), 3);
        g.color(kOL);
        g.rect((int16_t)(chx - 7), Y(63), (int16_t)(chx + 7), Y(68), 2);
    }

    // payout tray: a pile that shows how much came back this session
    int pile = won < 40 ? won : 40;
    for (int i = 0; i < pile; i++)
    {
        s_xy[2 * i] = (int16_t)(X(96 + (i * 37) % 190));
        s_xy[2 * i + 1] = (int16_t)(Y(203 - (i / 12) * 3));
    }
    if (pile > 0)
    {
        g.color(kOL);
        g.points(s_xy, pile, 9);
        g.color(kMedalCol[0]);
        g.points(s_xy, pile, 7);
    }
}

void MedalGame::drawHud(Gfx& g)
{
    char b[32];
    snprintf(b, sizeof(b), "%ld", (long)med);
    g.color(medFlash ? 0xffffff : 0xfff070);
    g.text(X(38), Y(76), kRom28, kCenterX, b);
    snprintf(b, sizeof(b), "%ld", (long)tix);
    g.color(tixFlash ? 0xffffff : 0xff9a30);
    g.text(X(38), Y(108), kRom28, kCenterX, b);
    g.color(0xffffff);
    g.text(X(38), Y(142), kFont14B, kCenterX, kLegs[leg].n);
    snprintf(b, sizeof(b), "%d OF 5  -  X%d", leg + 1, mult());
    g.color(0xc0b0e0);
    g.text(X(38), Y(154), kFont12, kCenterX, b);
    int32_t secs = (left + 59) / 60;
    if (secs < 0) secs = 0;
    snprintf(b, sizeof(b), "%ld:%02ld", (long)(secs / 60), (long)(secs % 60));
    g.color(secs <= 10 && ph == phGo ? 0xff5050 : 0xffffff);
    g.text(X(38), Y(180), kRom28, kCenterX, b);

    // the mountain heap (jackpot) at top left
    snprintf(b, sizeof(b), "%ld", (long)jp);
    g.color(heapT ? 0xffffff : kSkinC);
    g.text(X(38), Y(16), kRom28, kCenterX, b);
    g.color(0xc0b0e0);
    g.text(X(38), Y(42), kFont12, kCenterX, "MEDALS ON TOP");

    // heap of medals on the summit
    int hn = jp / 6;
    if (ava.on)
    {
        hn = ava.t < 60 ? hn * (60 - ava.t) / 60 : 0;
    }
    if (hn > 40) hn = 40;
    for (int i = 0; i < hn; i++)
    {
        float a = fmodf(i * 2.399f, 1.0f);
        float hr = 4 + jp / 14.0f;
        if (hr > 18) hr = 18;
        s_xy[2 * i] = X(192 + (a - .5f) * hr * 1.8f);
        s_xy[2 * i + 1] = Y(8 - (1 - fabsf(a - .5f) * 2) * hr * .45f + (i % 3));
    }
    if (hn > 0)
    {
        g.color(kOL);
        g.points(s_xy, hn, 6);
        g.color(kMedalCol[0]);
        g.points(s_xy, hn, 5);
    }

    // cable car
    {
        float a = carPos / 5.0f;
        float wx = 76 + 14 + 92 * a, wy = 50 - 40 * a + sinf(3.14159f * a) * 3;
        bool lit = F.active && F.kind == 1;
        g.color(lit ? 0xff5050 : 0xe02838);
        g.rect((int16_t)(X(wx) - 10), (int16_t)(Y(wy) + 1), (int16_t)(X(wx) + 10), (int16_t)(Y(wy) + 17), 3);
        g.color(lit ? 0xffffa0 : 0xa8e0ff);
        g.rect((int16_t)(X(wx) - 7), (int16_t)(Y(wy) + 4), (int16_t)(X(wx) + 7), (int16_t)(Y(wy) + 9), 1);
    }

    // check gate panel: where the gate is now
    g.color(0x40303a);
    g.rect(X(318), Y(162), X(374), Y(170), 3);
    g.color(gateLit ? 0xffffff : 0xffe070);
    int16_t gp = (int16_t)(X(318) + (gateX - 16) / 188.0f * (X(374) - X(318)));
    g.rect((int16_t)(gp - 6), Y(162), (int16_t)(gp + 6), Y(170), 3);
    g.color(0xc0b0e0);
    g.text(X(346), Y(176), kFont12, kCenterX, "DROP THROUGH IT");
    g.text(X(346), Y(186), kFont12, kCenterX, "TO SPIN THE REELS");
}

void MedalGame::drawTitle(Gfx& g)
{
    g.color(0x200818);
    g.text(402, kCanvasY + 42, kRom49, kCenterX, "MEDAL MOUNTAIN");
    g.color(0xffd040);
    g.text(400, kCanvasY + 40, kRom49, kCenterX, "MEDAL MOUNTAIN");
    g.color(0xffffff);
    g.text(400, kCanvasY + 100, kFont17B, kCenterX, "KAGEMORI CLASSICS - TRACKER EDITION");

    char b[80];
    if (ph == phResults)
    {
        g.color(0x200818);
        g.alpha(170);
        g.rect(200, kCanvasY + 130, 600, kCanvasY + 250, 10);
        g.alpha(255);
        snprintf(b, sizeof(b), "TICKETS THIS CLIMB  %ld", (long)lastTix);
        g.color(0xff9a30);
        g.text(400, kCanvasY + 142, kRom28, kCenterX, b);
        snprintf(b, sizeof(b), "%ld medals go back to your bank", (long)lastMedals);
        g.color(0xffffff);
        g.text(400, kCanvasY + 182, kFont17B, kCenterX, b);
        g.color(lastTix >= bestTix && lastTix > 0 ? 0x80ff90 : 0xc0b0e0);
        g.text(400, kCanvasY + 212, kFont14B, kCenterX, lastTix >= bestTix && lastTix > 0 ? "NEW BEST!" : "THE MOUNTAIN WAITS FOR YOU");
    }
    else
    {
        g.color(0x200818);
        g.alpha(170);
        g.rect(110, kCanvasY + 130, 690, kCanvasY + 250, 10);
        g.alpha(255);
        g.color(0xffffff);
        g.text(400, kCanvasY + 142, kFont14, kCenterX, "Jog wheel or arrows move the chute.  Play drops a medal (hold to keep feeding).");
        g.text(400, kCanvasY + 164, kFont14, kCenterX, "Or press a pad: its column is where the medal drops.");
        g.text(400, kCanvasY + 186, kFont14, kCenterX, "Drop through the moving CHECK gate to spin the reels.");
        g.text(400, kCanvasY + 208, kFont14, kCenterX, "Play or a pad also stops the roulette and the cable car.");
    }
    snprintf(b, sizeof(b), "BANK %ld MEDALS   -   BEST %ld   -   ALL-TIME TICKETS %ld", (long)bank, (long)bestTix,
             (long)lifetimeTix);
    g.color(0xfff070);
    g.text(400, kCanvasY + 276, kFont14B, kCenterX, b);
    if ((frame / 30) & 1)
    {
        g.color(0xffffff);
        g.text(400, kCanvasY + 314, kRom25, kCenterX, "PRESS PLAY OR ANY PAD");
    }
    g.color(0xd0c0e0);
    g.text(400, kCanvasY + 360, kFont12, kCenterX, "Medals and tickets are play tokens only. Run dry and a free MEDAL SHOWER tops you up.");
}

void MedalGame::drawForeground(Gfx& g)
{
    drawRail(g);
    drawHud(g);
    drawSlot(g);
    drawRoulette(g);

    for (int i = 0; i < 3; i++)
    {
        const Pop& p = pops[i];
        if (p.life <= 0) continue;
        int16_t rise = (int16_t)(p.t * 14 / (p.life ? p.life : 1));
        g.color(0x200818);
        g.text((int16_t)(X(p.x) + 1), (int16_t)(Y(p.y - rise) + 1), kFont17B, kCenterX, p.txt);
        g.color((p.t >> 2) & 1 ? p.col : 0xffffff);
        g.text(X(p.x), Y(p.y - rise), kFont17B, kCenterX, p.txt);
    }

    if (bannerLife > 0)
    {
        g.color(0x200818);
        g.alpha(190);
        g.rect(X(96), Y(86), X(288), Y(130), 8);
        g.alpha(255);
        g.color(bannerCol);
        g.text(X(192), Y(90), kRom36, kCenterX, bannerTxt);
        g.color(0xffffff);
        g.text(X(192), Y(116), kFont14B, kCenterX, bannerSub);
    }
    if (ph == phReady)
    {
        g.color(0x200818);
        g.text(X(192) + 2, Y(96) + 2, kRom49, kCenterX, ready > 50 ? "READY..." : "DROP!");
        g.color(0xffd040);
        g.text(X(192), Y(96), kRom49, kCenterX, ready > 50 ? "READY..." : "DROP!");
    }

    // ticker along the bottom of the cabinet
    g.color(0x14080c);
    g.rect(X(2), Y(214), X(382), Y(223), 2);
    g.color(0xffe0a0);
    g.text(400, (int16_t)(Y(214) + 1), kFont12, kCenterX, tickerTxt);

    if (flash > 0)
    {
        g.alpha((uint8_t)(flash * 120 > 40 ? 40 : flash * 120));
        g.color(0xffffff);
        g.rect(kCanvasX, kCanvasY, kCanvasX + kCanvasW, kCanvasY + kCanvasH);
        g.alpha(255);
    }
}

} // namespace i2arc
