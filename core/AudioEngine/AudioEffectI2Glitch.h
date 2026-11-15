#ifndef AUDIOENGINE_AUDIOEFFECTI2GLITCH_H_
#define AUDIOENGINE_AUDIOEFFECTI2GLITCH_H_

// i2 edition: puts i2glitch::Core (STATIC's GLITCH page) in the line-in
// stream: 2 inputs (left, right), 2 outputs. Untouched pass-through unless
// i2 DECK switches an effect on. Original Tracker only.

#include "AudioStream.h"
#include "i2Deck/i2Glitch.h"

class AudioEffectI2Glitch : public AudioStream
{
  public:
    AudioEffectI2Glitch();
    virtual void update(void) override;
    // From the main loop.
    void set(const i2glitch::Params& p);

  private:
    audio_block_t* inputQueueArray[2];
    i2glitch::Core core;
    volatile bool ready = false;
};

#endif /* AUDIOENGINE_AUDIOEFFECTI2GLITCH_H_ */
