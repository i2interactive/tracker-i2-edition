#include "i2Glitch.h"

namespace i2glitch
{
namespace
{
constexpr int32_t kFadeStep = 512; // 64 samples from silence to full
constexpr uint32_t kEdge = 64;     // loop edges fade over this many samples

inline int32_t clip16(int32_t v)
{
    return v > 32767 ? 32767 : v < -32768 ? -32768 : v;
}
} // namespace

void Core::init(int16_t* history, uint32_t size)
{
    buf = history;
    mask = size - 1;
    for (uint32_t i = 0; i < size; i++) buf[i] = 0;
    wpos = 0;
    cur = Params();
    next = Params();
    fade = kFadeTop;
}

void Core::feed(const int16_t* l, const int16_t* r, int n)
{
    if (buf == nullptr) return;
    for (int i = 0; i < n; i++)
    {
        buf[wpos & mask] = (int16_t)(((int32_t)l[i] + r[i]) >> 1);
        wpos++;
    }
}

// Called once per block: picks up new parameters and handles the moments an
// effect is engaged or let go.
void Core::begin()
{
    const uint8_t was = cur.fx;
    const uint32_t oldLen = (was & kFreeze) ? cur.freezeLen : cur.stutterLen;
    cur = next;
    const uint8_t now = cur.fx;
    const uint32_t maxLen = (mask + 1) / 2;
    if (cur.stutterLen < kEdge * 2) cur.stutterLen = kEdge * 2;
    if (cur.stutterLen > maxLen) cur.stutterLen = maxLen;
    if (cur.freezeLen < kEdge * 2) cur.freezeLen = kEdge * 2;
    if (cur.freezeLen > maxLen) cur.freezeLen = maxLen;
    if (cur.crushHold < 1) cur.crushHold = 1;
    if (cur.crushBits < 1) cur.crushBits = 1;
    if (cur.crushBits > 16) cur.crushBits = 16;

    const bool loopWas = was & (kStutter | kFreeze);
    const bool loopNow = now & (kStutter | kFreeze);
    const uint32_t len = (now & kFreeze) ? cur.freezeLen : cur.stutterLen;
    if (loopNow && (!loopWas || ((now ^ was) & kFreeze)))
    {
        // grab the last `len` samples and loop them
        loopStart = wpos - len;
        loopPos = 0;
        fade = 0;
    }
    else if (loopNow && len != oldLen && loopPos >= len)
    {
        loopPos = 0;
    }
    if (!loopNow && loopWas)
    {
        fade = 0; // back to the live signal
    }
    if ((now & kStop) && !(was & kStop))
    {
        rpInt = wpos - 1;
        rpFrac = 0;
        rate = 1u << 24;
    }
    if (!(now & kStop) && (was & kStop) && !loopNow)
    {
        fade = 0;
    }
    if ((now & kFilter) && !(was & kFilter))
    {
        low = 0;
        band = 0;
    }
}

void Core::process(const int16_t* l, const int16_t* r, int16_t* out, int n)
{
    if (buf == nullptr) return;
    begin();
    const uint8_t fx = cur.fx;
    const bool looping = fx & (kStutter | kFreeze);
    const uint32_t len = (fx & kFreeze) ? cur.freezeLen : cur.stutterLen;
    const int32_t crushMask = (int32_t)(~((1u << (16 - cur.crushBits)) - 1u));
    const float f = cur.cutoff * (1.0f / 32768.0f);
    const float q = cur.damp * (1.0f / 16384.0f);

    for (int i = 0; i < n; i++)
    {
        const int32_t x = ((int32_t)l[i] + r[i]) >> 1;
        int32_t y;
        if (!looping)
        {
            buf[wpos & mask] = (int16_t)x;
            wpos++;
        }

        if (looping)
        {
            y = buf[(loopStart + loopPos) & mask];
            uint32_t edge = loopPos < len - loopPos ? loopPos : len - loopPos;
            if (edge < kEdge) y = y * (int32_t)edge / (int32_t)kEdge;
            if (++loopPos >= len) loopPos = 0;
        }
        else if (fx & kStop)
        {
            const int32_t a = buf[rpInt & mask];
            const int32_t b = buf[(rpInt + 1) & mask];
            y = a + (((b - a) * (int32_t)(rpFrac >> 8)) >> 16);
            // fade out over the last quarter of the stop
            if (rate < (1u << 22)) y = y * (int32_t)(rate >> 8) >> 14;
            uint32_t acc = rpFrac + rate;
            rpInt += acc >> 24;
            rpFrac = acc & 0xFFFFFFu;
            rate = rate > cur.stopStep ? rate - cur.stopStep : 0;
        }
        else
        {
            y = x;
        }

        if (fx & kCrush)
        {
            if (holdLeft == 0)
            {
                held = y & crushMask;
                holdLeft = cur.crushHold;
            }
            holdLeft--;
            y = held;
        }

        if (fx & kNoise)
        {
            rng ^= rng << 13;
            rng ^= rng >> 17;
            rng ^= rng << 5;
            y += ((int32_t)(int16_t)(rng >> 16) * cur.noise) >> 15;
        }

        if (fx & kFilter)
        {
            low += f * band;
            float high = (float)y - low - q * band;
            band += f * high;
            if (low > 1.0e6f || low < -1.0e6f || band > 1.0e6f || band < -1.0e6f)
            {
                low = 0;
                band = 0;
            }
            y = (int32_t)low;
        }

        y = clip16(y);
        if (fade < kFadeTop)
        {
            y = y * fade >> 15;
            fade += kFadeStep;
            if (fade > kFadeTop) fade = kFadeTop;
        }
        out[i] = (int16_t)y;
    }
}

} // namespace i2glitch
