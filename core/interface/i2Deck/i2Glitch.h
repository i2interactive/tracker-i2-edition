#ifndef INTERFACE_I2DECK_I2GLITCH_H_
#define INTERFACE_I2DECK_I2GLITCH_H_

// i2 edition: the glitch processor behind STATIC's GLITCH page.
//
// The FM tuner's audio reaches the codec's line input through an analog
// switch, and the codec streams it into the audio engine over I2S like any
// line-in signal (AudioInput: i2sStream -> stereoToMono -> output mixer).
// On the original Tracker that stream passes through this processor on its
// way to the mixer. When nothing is engaged it passes the audio through
// untouched and only keeps a 1.5 s history; when an effect is engaged it
// works on a mono mix (the tuner runs in mono anyway).
//
// No Tracker dependencies: the audio object (AudioEngine/AudioEffectI2Glitch)
// wraps it, and the PC harness tests it directly.

#include <stdint.h>

namespace i2glitch
{
enum : uint8_t
{
    kStutter = 1 << 0, // repeat the last beat division
    kCrush = 1 << 1,   // bit and sample-rate reduction
    kStop = 1 << 2,    // tape stop: the pitch drops to nothing
    kNoise = 1 << 3,   // noise bursts (the deck also hops the tuner)
    kFilter = 1 << 4,  // resonant low-pass
    kFreeze = 1 << 5,  // hold a short grain
};

constexpr uint32_t kSampleRate = 44118;

struct Params
{
    uint8_t on = 0; // 0 = straight through, untouched
    uint8_t fx = 0; // the bits above
    uint8_t crushBits = 16;
    uint8_t crushHold = 1;      // keep every Nth sample
    uint32_t stutterLen = 5512; // samples
    uint32_t freezeLen = 2940;  // samples
    uint32_t stopStep = 0;      // playback-rate drop per sample (1.0 = 1 << 24)
    uint16_t noise = 0;         // 0-32767
    uint16_t cutoff = 32767;    // filter frequency coefficient (Q15, 0-1)
    uint16_t damp = 16384;      // filter damping (Q15, 1/Q; lower = sharper)
};

class Core
{
  public:
    // history: power-of-two number of mono samples (stutter and freeze can
    // loop up to half of it)
    void init(int16_t* history, uint32_t size);
    void set(const Params& p) { next = p; }
    // True while the deck has the processor switched in.
    bool on() const { return next.on != 0; }
    // True when nothing is engaged and no fade is running: pass the input
    // through untouched, but call feed() so the history stays current.
    bool idle() const { return cur.fx == 0 && next.fx == 0 && fade >= kFadeTop; }
    void feed(const int16_t* l, const int16_t* r, int n);
    void process(const int16_t* l, const int16_t* r, int16_t* out, int n);

  private:
    static constexpr int32_t kFadeTop = 32767;
    void begin();

    int16_t* buf = nullptr;
    uint32_t mask = 0;
    Params cur, next;
    uint32_t wpos = 0;
    uint32_t loopStart = 0, loopPos = 0;
    uint32_t rpInt = 0, rate = 1u << 24;
    uint32_t rpFrac = 0; // 24-bit fraction
    uint8_t holdLeft = 0;
    int32_t held = 0;
    float low = 0, band = 0;
    uint32_t rng = 0x9E3779B9u;
    int32_t fade = kFadeTop;
};

} // namespace i2glitch

#endif /* INTERFACE_I2DECK_I2GLITCH_H_ */
