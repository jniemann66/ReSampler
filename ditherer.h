/*
* Copyright (C) 2016 - 2021 Judd Niemann - All Rights Reserved.
* You may use, distribute and modify this code under the
* terms of the GNU Lesser General Public License, version 2.1
*
* You should have received a copy of GNU Lesser General Public License v2.1
* with this file. If not, please refer to: https://github.com/jniemann66/ReSampler
*/

#ifndef DITHERER_H
#define DITHERER_H 1

// Ditherer.h
// defines Ditherer class, for adding dither and noise-shaping to input samples

// configuration:
#define MAX_FIR_FILTER_SIZE 41

#include <cmath>
#include <random>
#include <cstring>

#include "biquad.h"
#include "noiseshape.h"

namespace ReSampler {

enum FilterType {
	bypass,
	cascadedBiquad,
	fir
};

enum NoiseGeneratorType {
	flatTPDF,
	slopedTPDF,
	RPDF,
	GPDF,
	impulse,
	legacyTPDF
};

enum DitherProfileID {
	flat,
	legacy,
	flat_f,
	ModEWeighted44k,
	Wannamaker3tap,
	Lipshitz44k,
	standard,
	Wannamaker24tap,
	Wannamaker9tap,
	High28,
	ImpEWeighted44k,
	High30,
	High32,
	Blue,
	Rpdf,
	Rpdf_f,
	end
};

struct DitherProfile {
	DitherProfileID id;
	const char* name;
	NoiseGeneratorType noiseGeneratorType;
	FilterType filterType;
	int intendedSampleRate;
	int N;
	const double* coeffs;
	bool bUseFeedback;
};

const DitherProfile ditherProfileList[] = {

	// id, name, noiseGeneratorType, filterType, intendedSampleRate, N, coeffs, bUseFeedback

	{ flat, "flat tpdf", flatTPDF, bypass, 44100, 1, noiseShaperPassThrough, false },
	{ legacy, "classic", legacyTPDF, bypass, 44100, 10, noiseShaperPassThrough, true },
	{ flat_f, "flat tpdf (with error-correction feedback)", flatTPDF, fir, 44100, 1, noiseShaperPassThrough, true },
	{ ModEWeighted44k, "Modified E-Weighted",flatTPDF, fir, 44100, 9, modew44, true },
	{ Wannamaker3tap, "Wannamaker 3-tap", slopedTPDF, fir, 44100, 3, wan3, true },
	{ Lipshitz44k, "Lipshitz",flatTPDF, fir, 44100, 5, lips44, true },
	{ standard, "standard", slopedTPDF, fir, 44100, 10, std_44, true },
	{ Wannamaker24tap, "Wannamaker 24-tap",flatTPDF, fir, 44100, 24, wan24, true },
	{ Wannamaker9tap, "Wannamaker 9-tap",flatTPDF, fir, 44100, 9, wan9, true },
	{ High28, "High28", slopedTPDF, fir, 44100, 13, high28, true },
	{ ImpEWeighted44k, "Improved E-Weighted",flatTPDF, fir, 44100, 9, impew44, true },
	{ High30, "High30", slopedTPDF, fir, 44100, 10, high30, true },
	{ High32, "High32",slopedTPDF, fir, 44100, 12, high32, true },
	{ Blue, "blue noise", flatTPDF, fir, 44100, 23, blue, true },
	{ Rpdf, "flat rpdf", RPDF, bypass, 44100, 1, noiseShaperPassThrough, false },
	{ Rpdf_f, "flat rpdf (with error-correction feedback)", RPDF, bypass, 44100, 1, noiseShaperPassThrough, true }
};

template<typename FloatType>
class Ditherer
{
public:
	// Constructor:
	// signalBits: number of bits of the target bitformat
	// ditherBits: number of bits of dither to add, and doesn't have to be an integer
	// bAutoBlankingEnabled: if true, enable auto-blanking of dither (on Silence)
	// seed: seed for PRNG
	// filterID: noise-shaping filter to use

	Ditherer(unsigned int signalBits, FloatType ditherBits, bool bAutoBlankingEnabled, int seed, DitherProfileID ditherProfileID = standard) :
		seed(seed),
		Z1(0),
		masterVolume(1.0),
		randGenerator(static_cast<unsigned int>(seed)),		// initialize (seed) RNG
		dist(0, randMax),		// set the range of the random number distribution
		signalBits(signalBits),
		ditherBits(ditherBits),
		selectedDitherProfile(ditherProfileList[ditherProfileID]),
		gain(1.0),
		bUseErrorFeedback(ditherProfileList[ditherProfileID].bUseFeedback),
		bPulseEmitted(false),
		bAutoBlankingEnabled(bAutoBlankingEnabled)

	{
		// general parameters:
		maxSignalMagnitude = static_cast<FloatType>((1 << (signalBits - 1)) - 1); // note the -1 : match 32767 scaling factor for 16 bit !
		reciprocalSignalMagnitude = static_cast<FloatType>(1.0 / maxSignalMagnitude); // value of LSB in target format
		maxDitherScaleFactor = static_cast<FloatType>(pow(2, ditherBits - 1)) / maxSignalMagnitude / static_cast<FloatType>(randMax);
		oldRandom = 0;

		// set-up noise generator:
		switch (selectedDitherProfile.noiseGeneratorType) {
		case flatTPDF:
			noiseGenerator = &Ditherer::noiseGeneratorFlatTPDF;
			break;
		case RPDF:
			noiseGenerator = &Ditherer::noiseGeneratorRPDF;
			break;
		case GPDF:
			noiseGenerator = &Ditherer::noiseGeneratorGPDF;
			break;
		case impulse:
			noiseGenerator = &Ditherer::noiseGeneratorImpulse;
			break;
		case legacyTPDF:
			noiseGenerator = &Ditherer::noiseGeneratorLegacy;
			break;
		case slopedTPDF:
		default:
			noiseGenerator = &Ditherer::noiseGeneratorSlopedTPDF;
		}

		// set-up filter type:
		switch (selectedDitherProfile.filterType) {
		case bypass:
			noiseShapingFilter = &Ditherer::noiseShaperPassThrough;
			break;
		case fir:
			noiseShapingFilter = &Ditherer::noiseShaperFIR;
			break;
		case cascadedBiquad:
		default:
			noiseShapingFilter = &Ditherer::noiseShaperCascadedBiquad;
		}

		//// IIR-specific stuff:
		if (ditherBits < 1.5)
		{
			// IIR noise-shaping filter (2 biquads) - flatter response
			f1.setCoeffs(0.798141839881378,
				-0.7040563852194521,
				0.15341541599754416,
				0.3060312586301247,
				0.02511886431509577);

			f2.setCoeffs(0.5,
				-0.7215722413008345,
				0.23235922079486643,
				-1.5531272249269004,
				0.7943282347242815);
		}
		else
		{
			// IIR noise-shaping filter (2 biquads)
			f1.setCoeffs(0.1872346691747817,
				-0.1651633303505913,
				0.03598944852318585,
				1.2861600144545022,
				0.49000000000000016);

			f2.setCoeffs(0.5,
				-0.7215722413008345,
				0.23235922079486643,
				-1.2511963408503206,
				0.5328999999999999);
		}

		// FIR-specific stuff:
		const FloatType scale = 1.0;
		FIRLength = selectedDitherProfile.N;
		for (int n = 0; n < FIRLength; ++n) {
			FIRCoeffs[n] = static_cast<FloatType>(scale * selectedDitherProfile.coeffs[n]);
		}

		memset(FIRHistory, 0, MAX_FIR_FILTER_SIZE * sizeof(FloatType));

		// set-up Auto-blanking:
		if (bAutoBlankingEnabled) {	// initial state: silence
			ditherScaleFactor = 0.0;
		}
		else {	// initial state: dithering
			ditherScaleFactor = maxDitherScaleFactor;
		}

		autoBlankLevelThreshold = static_cast<FloatType>(1.0 / pow(2, 32)); // 1 LSB of 32-bit digital
		autoBlankTimeThreshold = 30000; // number of zero samples before activating autoblank
		autoBlankDecayCutoff = static_cast<FloatType>(0.25 * reciprocalSignalMagnitude / randMax);
		zeroCount = 0;

	} // Ends Constructor

	void adjustGain(FloatType factor) {
		gain *= factor;
		maxDitherScaleFactor = static_cast<FloatType>(gain * pow(2, ditherBits - 1) / maxSignalMagnitude / randMax);
	}

	void reset() {
		// reset filters
		f1.reset();
		f2.reset();
		f3.reset();

		memset(FIRHistory, 0, MAX_FIR_FILTER_SIZE * sizeof(FloatType));

		// re-seed PRNG
		randGenerator.seed(static_cast<unsigned int>(seed));
		oldRandom = 0;
		Z1 = 0;
		zeroCount = 0;
		masterVolume = 1.0;
		if (bAutoBlankingEnabled) {	// initial state: silence
			ditherScaleFactor = 0.0;
		}
		else {	// initial state: dithering
			ditherScaleFactor = maxDitherScaleFactor;
		}
	}

// The dither function ///////////////////////////////////////////////////////
//
// Ditherer Topology:
//                              Noise
//                               |
//                               v
//                    preDither [G1]
//                         ^     |   +----------> preQuantize
//                         |     v   |
//   inSample ----->+( )---+--->(+)--+--[G2]-->[Q]-->--+------> postQuantize
//                    -    |                           |
//                    ^    +---------->-( )+<----------+
//                    |                  |
//                 [filter]              |
//                    |                  v
//                    +---<---[z^-1]-----+
//
//  Gain Stages:
//	G1 = ditherScaleFactor
//  G2 = masterVolume
//

FloatType dither(FloatType inSample) {

	// Auto-Blanking
	if (bAutoBlankingEnabled) {
		if (std::abs(inSample) < autoBlankLevelThreshold) {
			++zeroCount;
			if (zeroCount > autoBlankTimeThreshold) {
				ditherScaleFactor *= autoBlankDecayFactor; // decay
				if (ditherScaleFactor < autoBlankDecayCutoff) {
					ditherScaleFactor = 0.0; // decay cutoff
					masterVolume = 0.0; // mute
				}
			}
		}
		else {
			zeroCount = 0; // reset
			ditherScaleFactor = maxDitherScaleFactor; // restore
			masterVolume = 1.0;
		}
	} // ends auto-blanking

	FloatType noise = (this->*noiseGenerator)() * ditherScaleFactor;
	FloatType preDither = bUseErrorFeedback ? inSample - (this->*noiseShapingFilter)(Z1) : inSample;
	FloatType preQuantize, postQuantize;
	preQuantize = masterVolume * (preDither + noise);
	postQuantize = reciprocalSignalMagnitude * round(maxSignalMagnitude * preQuantize); // quantize
	Z1 = (postQuantize - preDither);
	return postQuantize;
} // ends function: dither()

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

private:
	int oldRandom;
	int seed;
	FloatType Z1;				// last Quantization error
	FloatType maxSignalMagnitude;	// maximum integral value for signal target bit depth (for quantizing)
	FloatType reciprocalSignalMagnitude; // for normalizing quantized signal back to +/- 1.0
	FloatType maxDitherScaleFactor, ditherScaleFactor;	// maximum integral value for dither target bit depth
	FloatType masterVolume;
	int64_t zeroCount; // number of consecutive zeroes in input;
	FloatType autoBlankDecayCutoff;	// threshold at which ditherScaleFactor is set to zero during active blanking
	std::mt19937 randGenerator; // Mersenne Twister - one of the best random number algorithms available
	std::uniform_int_distribution<int> dist; // random number distribution
	static const int randMax = 16777215; // 2^24 - 1 */
	unsigned int signalBits;
	FloatType ditherBits;
	DitherProfile selectedDitherProfile;
	FloatType gain;
	bool bUseErrorFeedback;
	FloatType outputLimit;
	FloatType(Ditherer::*noiseShapingFilter)(FloatType); // function pointer to noise-shaping filter
	FloatType(Ditherer::*noiseGenerator)(); // function pointer to noise-generator
	bool bPulseEmitted;

	// Auto-Blanking parameters:
	bool bAutoBlankingEnabled;
	FloatType autoBlankLevelThreshold;				// input signals below this threshold are considered zero
	FloatType autoBlankTimeThreshold;				// number of zero samples before activating blanking
	const FloatType autoBlankDecayFactor = static_cast<FloatType>(0.9995);	// dither level will decrease by this factor for each sample when blanking is active

	// IIR Filter-related stuff:
	Biquad<double> f1;
	Biquad<double> f2;
	Biquad<double> f3;
	Biquad<double> f4;

	// FIR Filter-related stuff:
	int FIRLength;
	FloatType FIRCoeffs[MAX_FIR_FILTER_SIZE];
	FloatType FIRHistory[MAX_FIR_FILTER_SIZE]; // (circular) buffer for noise history

	// --- Noise-generating functions ---

	// pure flat tpdf generator
	// calculate two random numbers and subtracts them, yielding a triangular distribution (which is 'fattest' at zero).
	FloatType noiseGeneratorFlatTPDF() {
		int a = dist(randGenerator);
		int b = dist(randGenerator);
		return static_cast<FloatType>(a - b);
	}

	// The sloped TPDF generator remembers and subtracts the previous random number from the new random number,
	// which is equivalent to applying a [1,-1] 2-tap FIR (also known as the first-difference operator),
	// This yields a first-order 6dB/octave (20dB/decade) highpass magnitude response.
	// Thus, the resulting noise is violet noise instead of white, which is quite effective for dithering purposes.
	// It also has the advantage of only calcluating one random number on each iteration, instead of two.
	FloatType noiseGeneratorSlopedTPDF() {
		int newRandom = dist(randGenerator);
		auto tpdfNoise = static_cast<FloatType>(newRandom - oldRandom);
		oldRandom = newRandom;
		return tpdfNoise;
	}

	FloatType noiseGeneratorRPDF() { // rectangular PDF (single PRNG)
		static constexpr int halfRand = (randMax + 1) >> 1;
		return static_cast<FloatType>(halfRand - dist(randGenerator));
	}

	FloatType noiseGeneratorGPDF() { // Gaussian PDF (n PRNGs)
		// calculate n random numbers and average them
		static constexpr int halfRand = (randMax + 1) >> 1;
		const int n = 5;
		FloatType r = 0;
		for (int i = 0; i < n; ++i) {
			r += dist(randGenerator);
		}
		return static_cast<FloatType>(halfRand - r/n);
	}

	FloatType noiseGeneratorImpulse() { // impulse - emits a single pulse at the begininng, followed by zeroes (for testing only)
		if (!bPulseEmitted) {
			bPulseEmitted = true;
			return static_cast<FloatType>(randMax);
		}
		return 0.0;
	}

	FloatType noiseGeneratorLegacy() { // legacy noise generator (from previous version of ReSampler) - applies filter to noise _before_ injection into dither engine
		int newRandom = dist(randGenerator);
		auto tpdfNoise = static_cast<FloatType>(newRandom - oldRandom); // sloped TDPF
		oldRandom = newRandom;
		return static_cast<FloatType>(f2.filter(f1.filter(tpdfNoise)));
	}

	// --- Noise-shaping functions ---

	FloatType noiseShaperPassThrough(FloatType x) {
		return x;
	}

	FloatType noiseShaperCascadedBiquad(FloatType x) {
		return static_cast<FloatType>(f3.filter(f2.filter(f1.filter(x))));
	}

	FloatType noiseShaperFIR(FloatType x) { // very simple FIR ...

		// put sample at end of buffer:
		FloatType* historyPtr = &FIRHistory[FIRLength - 1];
		*historyPtr = x;

		FloatType filterOutput = 0.0;

		// macc with coefficients:
		for (int k = 0; k < FIRLength; k++) {
			filterOutput += *historyPtr-- * FIRCoeffs[k];
		}

		// shift buffer backwards for next time:
		memmove(FIRHistory, &FIRHistory[1],
			(FIRLength - 1) * sizeof(FloatType));

		return filterOutput;
	}
};

} // namespace ReSampler

#endif // DITHERER_H
