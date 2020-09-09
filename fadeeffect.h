// fadeeffect.h : defines FadeEffect Class
// provides a basic fade-in at the start of the audio, or fade-out at the end of the audio

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
			for(int ch = 0; ch < Effect<FloatType>::channelCount; ch++) {
				p[i + ch] = inputBuffer[i + ch] * gain;
			}

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

	int64_t getTotalFrames() const
	{
		return totalFrames;
	}

	double getQuietDb() const
	{
		return quietDb;
	}

    void setFadeType(int value)
    {
        fadeType = value;
    }

    void setFadeIn(double seconds)
    {
		fadeInStopPosition = std::min(std::max((int64_t)0, static_cast<int64_t>(seconds * Effect<FloatType>::sampleRate)), totalFrames - 1); // 0 <= fadeInStopPosition < totalFrames
		if(fadeInStopPosition > 0) {
			gain = pow(10, quietDb / 20.0);
			gainIncreaseRate = pow(10, -quietDb / fadeInStopPosition / 20);
			fadeType |= FadeTypeFadeIn;
		}
    }

    void setFadeOut(double seconds)
    {
		fadeOutStartPosition = std::max(fadeInStopPosition, static_cast<int64_t>(totalFrames - 1 - seconds * Effect<FloatType>::sampleRate));
		if(fadeOutStartPosition < totalFrames - 1) {
			gainDecreaseRate = pow(10, quietDb / (totalFrames - fadeOutStartPosition) / 20);
			fadeType |= FadeTypeFadeOut;
		}
	}

	void setTotalFrames(const int64_t &value)
	{
		totalFrames = value;
	}


	void setQuietDb(double value)
	{
		quietDb = value;
	}

private:
	FloatType gain{1.0};
	FloatType gainIncreaseRate{1.0};
	FloatType gainDecreaseRate{1.0};
	double quietDb{-80.0};
    int fadeType{0};
    int64_t position{0};
	int64_t totalFrames{0};
	int64_t fadeInStopPosition{0};
	int64_t fadeOutStartPosition{0};
};


}

#endif // FADEEFFECT_H
