#ifndef SIGNALGENERATOR_H
#define SIGNALGENERATOR_H

#include "sndfile.h"

#include <string>

class SignalGenerator{
public:
	static void generateExpSweep(
			const std::string& filename,
			int sampleRate = 96000,
			int format = SF_FORMAT_WAV | SF_FORMAT_FLOAT,
			double duration = 10.0,
			int octaves = 12,
			double amplitude_dB = -3.0
			);

	// generates a signal which is zero everywhere, except for one sample in the middle, which is equal to the value of 'amplitude'
	static void generateImpulse(
			const std::string& filename,
			int sampleRate = 96000,
			int format = SF_FORMAT_WAV | SF_FORMAT_FLOAT,
			int N = 1024,
			int octaves = 12,
			double amplitude_dB = -3.0
			);
};

#endif // SIGNALGENERATOR_H
