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
class FadeEffect : public Effect<FloatType>
{
public:
    virtual const FloatType* process(const FloatType* inputBuffer, int sampleCount)
    {
        FloatType* p = Effect<FloatType>::outputBuffer.get();

		if(fadeType == FadeTypeNone) {
			// bypass
			return inputBuffer;
		}

        for(int i = 0; i < sampleCount; i += Effect<FloatType>::channelCount) {
            double inL = inputBuffer[i];
            double inR = inputBuffer[i + 1];
            p[i] = inL * gain;
            p[i + 1] = inR * gain;

			if(fadeType & FadeTypeFadeIn) {
				if(gain < 1.0) {
					gain *= gainIncreaseRate;
					if(gain >= 1.0) {
						gain = 1.0;
						fadeType ^= FadeTypeFadeIn; // fade-in completed; switch it off
					}
				}
			}

			if(fadeType & FadeTypeFadeOut) {
				if(position >= fadeOutStartPosition) {
					gain *= gainDecreaseRate;
				}
			}

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
		constexpr double initialDb = -100.0;
        gain = pow(10, initialDb / 20.0);
		gainIncreaseRate = pow(10, -initialDb / seconds / Effect<FloatType>::sampleRate / 20);
    }

    void setFadeOut(double seconds)
    {
		constexpr double finalDb = -100.0;
		gainDecreaseRate = pow(10, finalDb / seconds / Effect<FloatType>::sampleRate / 20);
		fadeOutStartPosition = std::max(0, totalFrames - seconds * Effect<FloatType>::sampleRate);
	}

private:
    FloatType gain{1.0};
	FloatType gainIncreaseRate{1.0};
	FloatType gainDecreaseRate{1.0};
    int fadeType{0};
    int64_t position{0};
    int64_t totalFrames{0};
	int fadeOutStartPosition{0};
};

}

#endif // FADEEFFECT_H
