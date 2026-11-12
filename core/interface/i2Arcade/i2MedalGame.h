#ifndef INTERFACE_I2ARCADE_I2MEDALGAME_H_
#define INTERFACE_I2ARCADE_I2MEDALGAME_H_

// MEDAL MOUNTAIN, Tracker edition.
//
// Port of i2 OS 2000's Medal Mountain (kmedal.js): a two-tier medal pusher.
// Steer the chute with the jog wheel (or press a pad: its column is where the
// medal drops), time drops against the gliding shelves, and pass the moving
// CHECK gate to spin the mountain reels: medal rain, bonus balls, the SKY
// ROULETTE, the CABLE CAR BONUS and THE AVALANCHE. Five 40-second legs with a
// rising ticket multiplier. Medals and tickets are play tokens only.
//
// Same tuning as the original (60 Hz steps, shelf stroke, gate speeds, reel
// odds, payouts). The physics is a single layer of sliding medals (the
// original also stacks them) so it fits the original Tracker's 180 MHz chip.

#include "i2ArcadeApi.h"

namespace i2arc
{

class MedalGame final : public Game
{
  public:
    virtual const char* name() const override { return "MEDAL MOUNTAIN"; }
    virtual void begin(Services& sv) override;
    virtual void end() override;
    virtual void tick(uint32_t nowMs) override;
    virtual void draw(Gfx& g, uint8_t layer) override;
    virtual bool dirty(uint8_t layer) const override { return dirtyMask & (1u << layer); }
    virtual void pad(uint8_t pad, bool down) override;
    virtual void button(Button b, bool down) override;
    virtual void jog(int16_t steps) override;
    virtual const char* buttonLabel(uint8_t i) const override;

    void debugPrint();
    bool debugNoBonus = false;
    int8_t debugForceOut = -1; // next reel result (kmedal.js outcome index)
    void debugQueueSpin() { if (stockN < 4) stock[stockN++] = 1; }

  private:
    static constexpr int kMaxCoins = 320;

    enum Phase : uint8_t
    {
        phTitle,
        phReady,
        phGo,
        phFinal,
        phResults,
    };

    struct Coin
    {
        float x, y, r;
        float vx;
        uint8_t tier;
        uint8_t look; // 0-2 medal colours, 10+ = bonus ball (10 pink, 11 blue, 12 yellow)
        uint8_t air;  // frames left falling (not touching anything)
        uint8_t airMax;
        bool dead;
        bool counts; // false while the bed settles (no payouts)
    };

    struct Shelf
    {
        float lo, hi, front, ph, spd, dp;
    };

    struct Slot
    {
        uint8_t st; // 0 idle, 1 spin, 2 show
        int16_t t;
        uint8_t outKind;
        uint8_t sym[3];
        float pos[3];
        bool done[3];
        uint8_t bump[3];
        bool win;
    };

    struct Feature
    {
        bool active;
        uint8_t kind; // 0 roulette, 1 cable car
        int16_t t;
        bool stop;
        int16_t done; // frame it finished, 0 = running
        // roulette
        uint8_t i;
        float iv;
        int16_t nt;
        uint8_t left;
        // cable car
        float pos, v;
        int8_t dir;
        int8_t tg;
    };

    struct Pop
    {
        int16_t x, y, t, life;
        uint32_t col;
        char txt[24];
    };

    // flow
    void newSession();
    void endSession();
    void loadSave();
    void persist();

    // physics
    Coin* addCoin(uint8_t tier, float x, float y, uint8_t look, uint8_t air);
    void seedBed(uint8_t tier);
    void stepShelf(uint8_t tier, bool quiet);
    void relax(uint8_t tier);
    void fallOff(Coin& c, bool quiet);
    void compact();

    // play
    void stepPlay();
    void steer(bool left, bool right);
    bool tryDrop();
    void gateIn();
    void onWin(Coin& c);
    void onLost(Coin& c);
    void addTix(int32_t n);
    int mult() const;
    void legUp(int leg);
    void waterfall();
    void stepSlot();
    void pickOutcome(uint8_t& kind, uint8_t sym[3]);
    void slotPay();
    void stepRain();
    void dropBall(uint8_t look, int16_t delay);
    void stepFeat();
    void startFeat(uint8_t kind);
    void featPress();
    void rouPay();
    void carPay();
    void avalanche();
    void stepAva();
    void pop(int16_t x, int16_t y, const char* txt, uint32_t col, int16_t life = 60);
    void banner(const char* txt, const char* sub, uint32_t col, int16_t life);
    void ticker(const char* txt);
    void refreshPadLights();

    // drawing
    void drawBackground(Gfx& g);
    void drawForeground(Gfx& g);
    void drawDiorama(Gfx& g);
    void drawField(Gfx& g);
    void drawRail(Gfx& g);
    void drawHud(Gfx& g);
    void drawSlot(Gfx& g);
    void drawRoulette(Gfx& g);
    void drawSymbol(Gfx& g, uint8_t sym, int16_t cx, int16_t cy, int16_t s);
    void drawTitle(Gfx& g);

    Services* sv = nullptr;
    uint32_t lastMs = 0;
    uint32_t accMs = 0;
    uint32_t frame = 0;
    uint8_t dirtyMask = 7;
    uint8_t bgLeg = 255;
    Phase bgPhase = phTitle;

    // input
    int16_t jogAcc = 0;
    bool leftHeld = false, rightHeld = false;
    bool dropHeld = false;
    bool dropPressed = false;
    int16_t holdT = 0;
    int8_t padTarget = -1;
    uint8_t padsDown = 0;
    bool startPressed = false;

    // session
    Phase ph = phTitle;
    int32_t t = 0, left = 0, total = 0;
    int16_t ready = 0;
    int16_t endT = 0;
    uint8_t leg = 0;
    Coin coins[kMaxCoins];
    int coinCount = 0;
    Shelf P[2];
    float gateX = 110, gateW = 18, gatePh = 0;
    int16_t gateLit = 0;
    float cx = 110, cvx = 0;
    int16_t cd = 0;
    int32_t med = 0, med0 = 0, tix = 0, won = 0, drops = 0;
    int16_t showerT = 0, medFlash = 0, tixFlash = 0;
    int32_t jp = 50;
    int16_t heapT = 0;
    uint8_t stock[4];
    uint8_t stockN = 0;
    Slot slot;
    Feature F;
    uint8_t fq[6];
    uint8_t fqN = 0;
    float carPos = 0;
    struct Rain
    {
        int16_t n, t;
    } rain[6];
    uint8_t rainN = 0;
    struct Ball
    {
        uint8_t look;
        int16_t t;
    } balls[4];
    uint8_t ballN = 0;
    struct Ava
    {
        bool on;
        int16_t t, n, left;
        int32_t b;
    } ava;
    int32_t wins[24];
    uint8_t winN = 0;
    int16_t wfT = 0;
    Pop pops[3];
    char bannerTxt[32];
    char bannerSub[64];
    uint32_t bannerCol = 0;
    int16_t bannerT = 0, bannerLife = 0;
    char tickerTxt[96];
    int16_t tickerT = 0;
    float flash = 0;
    int16_t shake = 0;

    // saved
    int32_t bank = 100;
    int32_t lifetimeTix = 0;
    int32_t bestTix = 0;
    int32_t sessions = 0;
    int32_t lastTix = 0;
    int32_t lastMedals = 0;
};

} // namespace i2arc

#endif /* INTERFACE_I2ARCADE_I2MEDALGAME_H_ */
