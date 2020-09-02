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

protected:
	// each effect owns an outputBuffer
	std::unique_ptr<FloatType> outputBuffer;
	int outputBufferSize;
	int channelCount;
};

} // namespace ReSampler

#endif // EFFECT_H
