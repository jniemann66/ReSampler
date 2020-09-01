#ifndef EFFECT_H
#define EFFECT_H

#include <vector>

namespace ReSampler {

template <typename FloatType>
class Effect
{
public:
	virtual FloatType* process(const FloatType* inputBuffer, int sampleCount) = 0;

	// getters
	int getChannelCount() const
	{
		return channelCount;
	}

	int getBufferSize() const
	{
		return outputBuffer.size();
	}

	// setters
	void setChannelCount(int value)
	{
		channelCount = value;
	}

	void setBufferSize(int value)
	{
		outputBuffer.resize(value, 0.0);
	}

private:
	std::vector<FloatType> outputBuffer;
	int channelCount;
};





} // namespace ReSampler

#endif // EFFECT_H
