#ifndef EFFECTCHAIN_H
#define EFFECTCHAIN_H

#include "effect.h"
#include <vector>

namespace ReSampler {

template <typename FloatType>
class EffectChain
{
public:
	void add(Effect<FloatType>* effect)
	{
		effect->setBufferSize(outputBufferSize);
		effect->setChannelCount(channelCount);
		effects.push_back(effect);
	}

	const FloatType* process(const FloatType* inputBuffer, int sampleCount)
	{
		const FloatType* lastBuffer = inputBuffer;
		for(auto& effect : effects) {
			lastBuffer = effect->process(lastBuffer, sampleCount);
		}
		return lastBuffer;
	}

	bool empty() const
	{
		return effects.empty();
	}

	int getOutputBufferSize() const
	{
		return outputBufferSize;
	}
	void setOutputBufferSize(int value)
	{
		outputBufferSize = value;
	}

	int getChannelCount() const
	{
		return channelCount;
	}
	void setChannelCount(int value)
	{
		channelCount = value;
	}

private:
	std::vector<Effect<FloatType>*> effects;
	int outputBufferSize;
	int channelCount;
};









} // namespace ReSampler

#endif // EFFECTCHAIN_H
