/*
* Copyright (C) 2024 Judd Niemann - All Rights Reserved.
* You may use, distribute and modify this code under the
* terms of the GNU Lesser General Public License, version 2.1
*
* You should have received a copy of GNU Lesser General Public License v2.1
* with this file. If not, please refer to: https://github.com/jniemann66/ReSampler
*/

// effectchain.h : defines container for effects
// if takeOwnership is set to true, effectchain will delete the effects upon destruction

#ifndef EFFECTCHAIN_H
#define EFFECTCHAIN_H


#include "effect.h"
#include <vector>

namespace ReSampler {

template <typename FloatType>
class EffectChain
{
public:

	virtual ~EffectChain()
	{
		if(takeOwnership) {
			for(auto effect : effects) {
				delete effect;
			}
		}
	}

	// add() : adds an effect to the chain
	void add(Effect<FloatType>* effect)
	{
		effect->setBufferSize(outputBufferSize);
		effect->setChannelCount(channelCount);
		effects.push_back(effect);
	}

	// process() : runs input through chain of effects
	const FloatType* process(const FloatType* inputBuffer, int sampleCount)
	{
		const FloatType* lastBuffer = inputBuffer;
		for(auto& effect : effects) {
			lastBuffer = effect->process(lastBuffer, sampleCount);
		}
		return lastBuffer;
	}

	// getters
	bool empty() const
	{
		return effects.empty();
	}

	int getOutputBufferSize() const
	{
		return outputBufferSize;
	}

	int getChannelCount() const
	{
		return channelCount;
	}

	bool getTakeOwnership() const
	{
		return takeOwnership;
	}

	// setters
	void setOutputBufferSize(int value)
	{
		outputBufferSize = value;
	}

	void setChannelCount(int value)
	{
		channelCount = value;
	}

	void setTakeOwnership(bool value)
	{
		takeOwnership = value;
	}

private:
	std::vector<Effect<FloatType>*> effects;
	int outputBufferSize;
	int channelCount;
	bool takeOwnership{true};
};

} // namespace ReSampler

#endif // EFFECTCHAIN_H
