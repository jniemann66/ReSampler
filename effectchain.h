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

	// setters
	void setOutputBufferSize(int value)
	{
		outputBufferSize = value;
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
