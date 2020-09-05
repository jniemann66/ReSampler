#ifndef FADEEFFECT_H
#define FADEEFFECT_H

#include "effect.h"

namespace ReSampler {

enum FadeType {
    FadeTypeNone = 0,
    FadeTypeFadeIn = 1,
    FadeTypeFadeOut = 2
};

template <typename FloatType>
class StereoImager : public Effect<FloatType>
{
public:
    virtual const FloatType* process(const FloatType* inputBuffer, int sampleCount)
    {
        FloatType* p = Effect<FloatType>::outputBuffer.get();
        for(int i = 0; i < sampleCount; i += Effect<FloatType>::channelCount) {
            double inL = inputBuffer[i];
            double inR = inputBuffer[i + 1];
            (void)inL; // todo: do something
            (void)inR;
        }
        return p;
    }

    int getFadeType() const
    {
        return fadeType;
    }

    void setFadeType(int value)
    {
        fadeType = value;
    }

private:
    FloatType gain{1.0};
    int fadeType{0};
    int64_t position{0};
    int64_t totalSamples{0};
};

}

#endif // FADEEFFECT_H
