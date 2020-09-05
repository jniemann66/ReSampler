#ifndef FADEEFFECT_H
#define FADEEFFECT_H

#include "effect.h"
#include <cmath>

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
            p[i] = inL * gain;
            p[i + 1] = inR * gain;
            gain *= gainChangeRate;
            position++;
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

    void setFadeIn(double seconds)
    {
        (void) seconds;
        startPostion = 0;
        stopPosition = seconds * Effect<FloatType>::sampleRate;
        double initialDb = -100.0;
        gain = pow(10, initialDb / 20.0);
        gainChangeRate = -initialDb / seconds / Effect<FloatType>::sampleRate;
    }

    void setFadeOut(double seconds)
    {
        (void) seconds;
    }
private:
    FloatType gain{1.0};
    FloatType gainChangeRate{1.0};
    int fadeType{0};
    int64_t position{0};
    int64_t totalFrames{0};
    int startPostion{0};
    int stopPosition{0};

};

}

#endif // FADEEFFECT_H
