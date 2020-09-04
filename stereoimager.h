#ifndef STEREOIMAGER_H
#define STEREOIMAGER_H

#include "effect.h"

namespace ReSampler {

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
			double mid = inL + inR;
			double side = inL - inR;
			p[i] = gain * (mid + stereoWidth * side);
			p[i + 1] = gain * (mid - stereoWidth * side);
		}
		return p;
	}

	FloatType getStereoWidth() const
	{
		return stereoWidth;
	}

	void setStereoWidth(const FloatType &value)
	{
		stereoWidth = value;
		gain = 1.0 / (1.0 + stereoWidth);
	}

private:
	FloatType gain{0.5};
	FloatType stereoWidth{1.0};
};

} // namespace ReSampler

#endif // STEREOIMAGER_H
