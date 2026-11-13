#ifndef INTERFACE_I2ARCADE_I2QUIZGAME_H_
#define INTERFACE_I2ARCADE_I2QUIZGAME_H_

// BRAIN BANZAI! - QUIZ KAGEMORI, Tracker edition.
//
// Port of i2 OS 2000's Quiz Kagemori (kquiz.js) for the original Tracker: the
// host MR. MARU, you against three CPU contestants, a buzzer round, two of
// ODD ONE OUT / TRUE STORY / CATEGORY BLITZ, a bonus buzzer round, FAST FINGER
// and the FINAL WAGER. The picture and music rounds are not in this edition.
//
// Answers sit in four rows that match the Tracker's four pad rows: press any
// pad in a row to pick that answer. The question bank is read from the SD card
// (i2/Arcade/Quiz/*.txt).

#include "i2ArcadeApi.h"

namespace i2arc
{

class QuizGame final : public Game
{
  public:
    virtual const char* name() const override { return "QUIZ KAGEMORI"; }
    virtual void begin(Services& sv) override;
    virtual void tick(uint32_t nowMs) override;
    virtual void draw(Gfx& g, uint8_t layer) override;
    virtual bool dirty(uint8_t layer) const override { return dirtyMask & (1u << layer); }
    virtual void pad(uint8_t pad, bool down) override;
    virtual void button(Button b, bool down) override;
    virtual void jog(int16_t steps) override;
    virtual const char* buttonLabel(uint8_t i) const override;
    virtual bool wantsExit() const override { return exitRequested; }

    // Host harness hooks
    void debugSkipToRound(int round);
    int debugPhase() const { return (int)ph; }

  private:
    enum Phase : uint8_t
    {
        phTitle,
        phMissing,
        phIntro,
        phRIntro,
        phRead,
        phPick,
        phRead2,
        phAnswer,
        phFCat,
        phWager,
        phReveal,
        phEnd,
        phOver,
    };

    enum Kind : uint8_t
    {
        kText,
        kChoose, // everyone answers at once (odd one out, true story, blitz)
        kOrder,
    };

    enum RoundType : uint8_t
    {
        rtBuzz,
        rtBonus,
        rtOdd,
        rtTrue,
        rtBlitz,
        rtFast,
        rtFinal,
    };

    struct TextQ
    {
        const char* cat;
        const char* q;
        const char* a[4];
    };
    struct OrderQ
    {
        const char* q;
        const char* a[4];
    };
    struct OddQ
    {
        const char* a[4];
        const char* why;
    };
    struct TrueQ
    {
        const char* a[4];
    };

    struct Question
    {
        Kind kind;
        const char* cat;
        const char* q;
        const char* ch[4];
        int8_t ans;
        int8_t order[4];
        int16_t oddIndex; // for the "why" line, -1 = none
    };

    struct Round
    {
        RoundType t;
        uint8_t n;
        uint8_t cat; // blitz category index
    };

    struct Player
    {
        const char* name;
        uint32_t col;
        bool cpu;
        float acc, spd;
        int32_t score;
        int16_t streak;
        bool ll, out;
        int8_t lock;
        int32_t lockT;
        int8_t pick;
        bool know;
        float tb, tl, tf, pdelay, wt;
        int8_t prog;
        int32_t fin;
        int16_t frz;
        int32_t bet, cap;
        bool wl;
        bool el[4];
        char ex; // 'n' neutral, 'w' won, 'l' lost, 't' thinking
        char pop[20];
        int16_t popT;
        uint32_t popCol;
    };

    // --- data
    bool loadBank();
    int splitFields(char* line, char** out, int maxFields);
    void shuffle(int8_t* idx, int n);

    // --- show flow (mirrors kquiz.js)
    void newShow();
    void beginRound();
    void nextQ();
    void beginQ();
    void beginFinal();
    void pickCat(int ci);
    void planCpu(Player& p);
    void buzzIn(Player& p);
    void resolvePick(Player& p, int i);
    bool lockAns(Player& p, int i, int32_t t);
    void ordPress(Player& p, int i);
    void useLL(Player& p);
    void award(Player& p, int32_t pts);
    void correct(Player& p, int32_t pts);
    void settle();
    void toReveal();
    void toAnswer();
    void finish();
    void gameOver();
    void say(const char* txt);
    void sayf(const char* fmt, ...);
    void popOn(Player& p, const char* txt, uint32_t col);
    void step();
    bool takeConfirm();
    int takeRow();
    bool skipPressed();

    int pickTextQ(int cat, bool preferVintNow);
    void makeTextQ(int bankIndex);

    void persist();
    void loadSave();
    void refreshPadLights();

    // --- drawing
    void drawBackground(Gfx& g);
    void drawForeground(Gfx& g);
    void drawHeader(Gfx& g);
    void drawPanel(Gfx& g);
    void drawChoices(Gfx& g);
    void drawPodiums(Gfx& g);
    void drawTitle(Gfx& g);
    int wrap(Gfx& g, uint8_t font, const char* s, int16_t width, int maxLines, int* starts, int* lens);
    void para(Gfx& g, uint8_t font, const char* s, int16_t x, int16_t y, int16_t width, int16_t lineH,
              int maxLines, int maxChars, uint8_t align);

    Services* sv = nullptr;
    uint32_t lastMs = 0;
    uint32_t accMs = 0;
    uint32_t frame = 0;
    uint8_t dirtyMask = 3;
    bool exitRequested = false;
    bool bankOk = false;

    // input latched between frames
    int8_t rowPressed = -1;
    bool confirmPressed = false;
    bool llPressed = false;
    int16_t jogSteps = 0;
    bool shiftHeld = false;
    uint8_t rowHeld[4] = {};

    // bank (pointers into the scratch buffer)
    TextQ* bank = nullptr;
    int bankCount = 0;
    OrderQ* ords = nullptr;
    int ordCount = 0;
    OddQ* odds = nullptr;
    int oddCount = 0;
    TrueQ* trues = nullptr;
    int trueCount = 0;
    uint8_t* usedText = nullptr; // per-bank-entry "asked this session" flags
    uint8_t* usedOrd = nullptr;
    uint8_t* usedOdd = nullptr;
    uint8_t* usedTrue = nullptr;

    // show state
    Phase ph = phTitle;
    Player P[4];
    uint8_t np = 4;
    Round plan[8];
    uint8_t planLen = 0;
    int8_t ri = -1;
    Question qs[6];
    uint8_t qn = 0;
    int8_t qi = -1;
    Question Q;
    int32_t pt = 0;      // frames in this phase
    int32_t rt = 0;      // reading frames
    bool cOn = false;    // choices visible
    int32_t cOnT = 0;
    int32_t cd = 0;      // pick countdown
    int32_t lim = 0;     // answer time limit
    bool early = false;
    int8_t buz = -1;
    int8_t winner = -1;
    bool elim[4] = {};
    float skill = 1.0f;
    int8_t chooser = -1;
    uint8_t fc[3] = {};
    int8_t fcur = 1;
    uint8_t fcat = 0;
    int8_t topIdx = -1;
    int32_t bonus = 0;
    int16_t got = 0;
    char sayText[200];
    int32_t sayT = 0;
    char sayNext[96];
    float flash = 0;

    // saved
    int32_t best = 0;
    int32_t wins = 0;
    int32_t plays = 0;
    int32_t lastScore = 0;
    bool lastTop = false;
};

} // namespace i2arc

#endif /* INTERFACE_I2ARCADE_I2QUIZGAME_H_ */
