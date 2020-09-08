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
