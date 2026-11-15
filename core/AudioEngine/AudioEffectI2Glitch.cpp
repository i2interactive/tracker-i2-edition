#include "AudioEffectI2Glitch.h"

#include "Audio.h"

#if !defined(PLATFORM_IMX)

// 3 s of mono history (256 KB) in the external SDRAM, not loaded from flash.
static constexpr uint32_t kHistory = 131072;
__attribute__((section(".noinit.$EXTERNAL_RAM.i2glitch"), aligned(4), used)) static int16_t s_history[kHistory];

AudioEffectI2Glitch::AudioEffectI2Glitch()
  : AudioStream(2, inputQueueArray)
{
}

void AudioEffectI2Glitch::set(const i2glitch::Params& p)
{
    if (!ready)
    {
        // first use: clear the history (it is not zeroed at start-up)
        // (the audio interrupt passes audio straight through until ready)
        core.init(s_history, kHistory);
        ready = true;
    }
    AudioNoInterrupts();
    core.set(p);
    AudioInterrupts();
}

AUDIO_CORE_FLASH void AudioEffectI2Glitch::update(void)
{
    audio_block_t* inL = receiveReadOnly(0);
    audio_block_t* inR = receiveReadOnly(1);
    if (!inL || !inR)
    {
        if (inL) release(inL);
        if (inR) release(inR);
        return;
    }

    if (!ready || !core.on() || core.idle())
    {
        if (ready && core.on())
        {
            core.feed(inL->data, inR->data, AUDIO_BLOCK_SAMPLES);
        }
        transmit(inL, 0);
        transmit(inR, 1);
        release(inL);
        release(inR);
        return;
    }

    audio_block_t* out = allocate();
    if (out == nullptr)
    {
        transmit(inL, 0);
        transmit(inR, 1);
        release(inL);
        release(inR);
        return;
    }
    core.process(inL->data, inR->data, out->data, AUDIO_BLOCK_SAMPLES);
    transmit(out, 0);
    transmit(out, 1);
    release(out);
    release(inL);
    release(inR);
}

#endif
