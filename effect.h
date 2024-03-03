/*
* Copyright (C) 2024 Judd Niemann - All Rights Reserved.
* You may use, distribute and modify this code under the
* terms of the GNU Lesser General Public License, version 2.1
*
* You should have received a copy of GNU Lesser General Public License v2.1
* with this file. If not, please refer to: https://github.com/jniemann66/ReSampler
*/

// effect.h : defines Effect Base Class

#ifndef EFFECT_H
#define EFFECT_H

#include <vector>
#include <memory>

namespace ReSampler {

template <typename FloatType>
class Effect
{
public:
	virtual const FloatType* process(const FloatType* inputBuffer, int sampleCount) = 0;

	// getters
	int getChannelCount() const
	{
		return channelCount;
	}

	int getBufferSize() const
	{
		return outputBufferSize;
	}

	int getSampleRate() const
	{
		return sampleRate;
	}

	// setters
	void setChannelCount(int value)
	{
		channelCount = value;
	}

	void setBufferSize(int value)
	{
		outputBufferSize = value;
		outputBuffer.reset(new FloatType[outputBufferSize]);
	}

    void setSampleRate(int value)
    {
        sampleRate = value;
    }

protected:
    // each effect owns an outputBuffer
    std::unique_ptr<FloatType> outputBuffer;
	int outputBufferSize;
	int channelCount;
    int sampleRate;
};

} // namespace ReSampler

#endif // EFFECT_H
