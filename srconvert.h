/*
* Copyright (C) 2016 - 2023 Judd Niemann - All Rights Reserved.
* You may use, distribute and modify this code under the
* terms of the GNU Lesser General Public License, version 2.1
*
* You should have received a copy of GNU Lesser General Public License v2.1
* with this file. If not, please refer to: https://github.com/jniemann66/ReSampler
*/

// srconvert.h : core sample rate conversion code.

#ifndef SRCONVERT_H
#define SRCONVERT_H 1

//#define USE_LAZYGET_ON_INTERPOLATE
#define USE_LAZYGET_ON_INTERPOLATE_DECIMATE

#include "FIRFilter.h"
#include "conversioninfo.h"
#include "fraction.h"
#include "ReSampler.h"

namespace ReSampler {

static_assert(std::is_copy_constructible<ConversionInfo>::value, "ConversionInfo needs to be copy Constructible");
static_assert(std::is_copy_assignable<ConversionInfo>::value, "ConversionInfo needs to be copy Assignable");

template<typename FloatType>
std::vector<FloatType> makeFilterCoefficients(const ConversionInfo& ci, Fraction fraction) {

	// determine cutoff frequency and steepness
	double targetNyquist = std::min(ci.inputSampleRate, ci.outputSampleRate) / 2.0;
	double ft = (ci.lpfCutoff / 100.0) * targetNyquist;
	double steepness = 0.090909091 / (ci.lpfTransitionWidth / 100.0);

	// determine filtersize
	int filterSize = static_cast<int>(
		std::min<int>(FILTERSIZE_BASE * ci.overSamplingFactor * std::max(fraction.denominator, fraction.numerator) * steepness, FILTERSIZE_LIMIT)
		| 1 // ensure that filter length is always odd
	);

	// determine sidelobe attenuation
	int sidelobeAtten = ((fraction.numerator == 1) || (fraction.denominator == 1)) ?
		195 :
		160;

	// Make some filter coefficients:
	int sampFreq = ci.overSamplingFactor * ci.inputSampleRate * fraction.numerator;
	std::vector<FloatType> filterTaps(filterSize, 0);
	FloatType* pFilterTaps = &filterTaps[0];
	makeLPF<FloatType>(pFilterTaps, filterSize, ft, sampFreq);
	applyKaiserWindow<FloatType>(pFilterTaps, filterSize, calcKaiserBeta(sidelobeAtten));

	// conditionally convert filter coefficients to minimum-phase:
	if (ci.bMinPhase) {
		makeMinPhase<FloatType>(pFilterTaps, filterSize);
		//return makeMinPhase2<FloatType>(pFilterTaps, filterSize);
	}

	return filterTaps;
}

template<typename FloatType>
class ResamplingStage
{
public:
	ResamplingStage(int L, int M, const FIRFilter<FloatType>& filter, bool bypassMode = false)
		: L(L), M(M),  m(0), filter(filter), bypassMode(bypassMode)
	{
		SetConvertFunction();
	}

	void convert(FloatType* outBuffer, size_t& outBufferSize, const FloatType* inBuffer, const size_t& inBufferSize) {
		(this->*convertFn)(outBuffer, outBufferSize, inBuffer, inBufferSize);
	}

	void setBypassMode(bool bypassMode) {
		ResamplingStage::bypassMode = bypassMode;
		SetConvertFunction();
	}

	void reset() {
		filter.reset();
		m = 0;
	}

private:
	int L;	// interpoLation factor
	int M;	// deciMation factor
	int m;	// decimation index
	FIRFilter<FloatType> filter;
	bool bypassMode;

	// The following typedef defines the type 'ConvertFunction' which is a pointer to any of the member functions which
	// take the arguments (FloatType* outBuffer, size_t& outBufferSize, const FloatType* inBuffer, const size_t& inBufferSize) ...
	typedef void (ResamplingStage::*ConvertFunction) (FloatType* outBuffer, size_t& outBufferSize, const FloatType* inBuffer, const size_t& inBufferSize); // see https://isocpp.org/wiki/faq/pointers-to-members
	ConvertFunction convertFn;

	// passThrough() - just copies input straight to output (used in bypassMode mode)
	void passThrough(FloatType* outBuffer, size_t& outBufferSize, const FloatType* inBuffer, const size_t& inBufferSize) {
		memcpy(outBuffer, inBuffer, inBufferSize * sizeof(FloatType));
		outBufferSize = inBufferSize;
	}

	// filterOnly() - keeps 1:1 conversion ratio, but applies filter
	void filterOnly(FloatType* outBuffer, size_t& outBufferSize, const FloatType* inBuffer, const size_t& inBufferSize) {
		size_t o = 0;
		for(size_t i = 0; i < inBufferSize; ++i) {
			filter.put(inBuffer[i]);
			outBuffer[o++] = filter.get();
		}
		outBufferSize = inBufferSize;
	}

	// interpolate() - interpolate (zero-stuffing) and apply filter:
	void interpolate(FloatType* outBuffer, size_t& outBufferSize, const FloatType* inBuffer, const size_t& inBufferSize) {
		size_t o = 0;
		for (size_t i = 0; i < inBufferSize; ++i) {
			for(int l = 0; l < L; ++l) {

#ifdef	USE_LAZYGET_ON_INTERPOLATE
				((l == 0) ? filter.put(inBuffer[i]) : filter.putZero());
				outBuffer[o++] = filter.lazyGet(L);
#else
				filter.put((l == 0) ? inBuffer[i] : 0);
				outBuffer[o++] = filter.get();
#endif
			}
		}
		outBufferSize = o;
	}

	// decimate() - decimate and apply filter
	void decimate(FloatType* outBuffer, size_t& outBufferSize, const FloatType* inBuffer, const size_t& inBufferSize) {
		size_t o = 0;
		int localm = m;
		for (size_t i = 0; i < inBufferSize; ++i) {
			filter.put(inBuffer[i]);
			if (localm == 0) {
				outBuffer[o++] = filter.get();
			}
			if(++localm == M) {
				localm = 0;
			}
		}
		outBufferSize = o;
		m = localm;
	}

	// interpolateAndDecimate()
	void interpolateAndDecimate(FloatType* outBuffer, size_t& outBufferSize, const FloatType* inBuffer, const size_t& inBufferSize) {
		size_t o = 0;
		int localm = m;
		for (size_t i = 0; i < inBufferSize; ++i) {
			for(int l = 0; l < L; ++l) {

#ifdef	USE_LAZYGET_ON_INTERPOLATE_DECIMATE
				((l == 0) ? filter.put(inBuffer[i]) : filter.putZero());
				if (localm == 0) {
					outBuffer[o++] = filter.lazyGet(L);
				}
#else
				filter.put((l == 0) ? inBuffer[i] : 0);
				if (localm == 0) {
					outBuffer[o++] = filter.get();
				}
#endif
				if (++localm == M) {
					localm = 0;
				}
			}
		}
		outBufferSize = o;
		m = localm;
	}

	void SetConvertFunction() {
		if (bypassMode) {
			convertFn = &ResamplingStage::passThrough;
		}
		else if (L == 1 && M == 1) {
			convertFn = &ResamplingStage::filterOnly;
		}
		else if (L != 1 && M == 1) {
			convertFn = &ResamplingStage::interpolate;
		}
		else if (L == 1 && M != 1) {
			convertFn = &ResamplingStage::decimate;
		}
		else {
			convertFn = &ResamplingStage::interpolateAndDecimate;
		}
	}
};

template <typename FloatType>
class Converter
{
public:
	explicit Converter(const ConversionInfo& ci) : ci(ci), groupDelay(0.0), isBypassMode(false), gain(1.0) {
		if (ci.outputSampleRate == ci.inputSampleRate) {
			isBypassMode = true;
			Converter::ci.bSingleStage = true;
		}

		if (Converter::ci.bSingleStage) {
			isMultistage = false;
			initSinglestage();
		} else {
			isMultistage = true;
			initMultistage();
		}
	}

	void convert(FloatType* outBuffer, size_t& outBufferSize, const FloatType* inBuffer, const size_t& inBufferSize) {
		if (isMultistage) {
			const FloatType* in = inBuffer; // first stage reads directly from inBuffer. Subsequent stages read from output of previous stage
			size_t inSize = inBufferSize;
			size_t outSize = 0;
			for (int i = 0; i < numStages; i++) {
				FloatType* out = (i == indexOfLastStage) ? outBuffer : intermediateOutputBuffers[i].data(); // last stage writes straight to outBuffer;
				convertStages[i].convert(out, outSize, in, inSize);
				in = out; // input of next stage is the output of this stage
				inSize = outSize;
			}
			outBufferSize = outSize;
		}
		else {
			convertStages[0].convert(outBuffer, outBufferSize, inBuffer, inBufferSize);
		}
	}

	double getGroupDelay() {
		return groupDelay;
	}

	double getGain() {
		return gain;
	}

	void reset() {
		for (int i = 0; i < numStages; i++) {
			convertStages[i].reset();
			if (i != indexOfLastStage) {
				std::fill(intermediateOutputBuffers[i].begin(), intermediateOutputBuffers[i].end(), 0.0);
			}
		}
	}

private:
	void initSinglestage() {
		numStages = 1;
		indexOfLastStage = 0; // numStages - 1
		Fraction f = getFractionFromSamplerates(ci.inputSampleRate, ci.outputSampleRate);
		ci.overSamplingFactor = ci.bMinPhase && (f.numerator != f.denominator) && (f.numerator <= 4 || f.denominator <= 4) ? 8 : 1;
		if (ci.overSamplingFactor != 1)
			gain *= ci.overSamplingFactor;

		std::vector<FloatType> filterTaps = makeFilterCoefficients<FloatType>(ci, f);
		f.numerator *= ci.overSamplingFactor;
		f.denominator *= ci.overSamplingFactor;

		FIRFilter<FloatType> firFilter(filterTaps.data(), static_cast<int>(filterTaps.size()));
		convertStages.emplace_back(f.numerator, f.denominator, firFilter, isBypassMode);
		groupDelay = (ci.bMinPhase || !ci.bDelayTrim) ? 0 : (filterTaps.size() - 1) / 2 / f.denominator;
		if (isBypassMode)
			groupDelay = 0;
	}

	void initMultistage() {
		Fraction masterConversionRatio = getFractionFromSamplerates(ci.inputSampleRate, ci.outputSampleRate);
		auto fractions = getConversionStages(masterConversionRatio, ci.maxStages);
		numStages = static_cast<int>(fractions.size());
		indexOfLastStage = numStages - 1;
		unsigned int inputRate = ci.inputSampleRate;
		double stretch = (ci.lpfCutoff + ci.lpfTransitionWidth) / 100.0;
		double lastStopFreq = stretch * inputRate / 2.0;
		std::string stageInputName(ci.inputFilename);
		double ft = ci.lpfCutoff / 100 * std::min(ci.inputSampleRate, ci.outputSampleRate) / 2.0;

		for (int i = 0; i < numStages; i++) {

			// copy ConversionInfo for this stage from master:
			ConversionInfo stageCi = ci;

			// set input & output rates of this stage:
			stageCi.inputSampleRate = inputRate;
			stageCi.outputSampleRate = inputRate * fractions[i].numerator / fractions[i].denominator;

			// decide whether to oversample this stage:
			stageCi.overSamplingFactor = stageCi.bMinPhase ? 2 : 1;
			if (stageCi.overSamplingFactor != 1) {
				gain *= stageCi.overSamplingFactor;
			}

			// set minSampleRate and minNyquist for this stage:
			decltype(stageCi.inputSampleRate) minSampleRate = std::min(stageCi.inputSampleRate, stageCi.outputSampleRate);
			decltype(stageCi.inputSampleRate) minNyquist = static_cast<unsigned int>(minSampleRate / 2.0);

			// determine transition frequency (cutoff) and stop frequency for this stage:
			double stopFreq = std::max(stretch * minNyquist, minSampleRate - lastStopFreq);
			assert(stopFreq > ft); // should always be the case for a LPF

			// set transition frequency (cutoff) and transition width for this stage (they are stored as percentage values)
			if (i == indexOfLastStage) { // last stage must have the characteristics of the requested parameters:
				stageCi.lpfTransitionWidth = ci.lpfTransitionWidth;
				stageCi.lpfCutoff = ci.lpfCutoff;
			}
			else {
				const double widthReduction = 2.0;
				stageCi.lpfTransitionWidth = 100.0 * (stopFreq - ft) / (stageCi.outputSampleRate * 0.5) / widthReduction;
				stageCi.lpfCutoff = 100 - stageCi.lpfTransitionWidth;
			}

			assert(stageCi.lpfTransitionWidth > 0.0);
			lastStopFreq = stopFreq; // keep this value for calculation of next stage's stopFreq

			// make the filter coefficients
			std::vector<FloatType> filterTaps = makeFilterCoefficients<FloatType>(stageCi, fractions[i]);

			// dumpFilter(filterTaps.data(), filterTaps.size());

			// make the filter
			FIRFilter<FloatType> firFilter(filterTaps.data(), static_cast<int>(filterTaps.size()));

			if (ci.bShowStages) { // dump stage parameters:
				std::cout << "Stage: " << 1 + i << "\n";
				std::cout << "inputRate: " << stageCi.inputSampleRate << "\n";
				std::cout << "outputRate: " << stageCi.outputSampleRate << "\n";
				std::cout << "ft: " << ft << "\n";
				std::cout << "stopFreq: " << stopFreq << "\n";
				std::cout << "transition width: " << stageCi.lpfTransitionWidth << " %\n";
				std::cout << "guarantee: " << lastStopFreq << "\n";
				std::cout << "Generated Filter Size: " << filterTaps.size() << "\n";

				stageCi.maxStages = 1;
				// stageCi.bSingleStage = true; // to-do: use single-stage engine vs. multi w/ maxStages= 1 ??
				stageCi.lpfMode = custom;
				stageCi.inputFilename = stageInputName;

				if (i != indexOfLastStage) {
					size_t lastDotPos = stageCi.outputFilename.find_last_of('.');
					if (lastDotPos != std::string::npos) {
						std::string pathWithoutExt = stageCi.outputFilename.substr(0, lastDotPos);
						std::string ext = stageCi.outputFilename.substr(lastDotPos);
						stageCi.outputFilename = pathWithoutExt + "-stage" + std::to_string(i + 1) + ext;
					}
					else {
						stageCi.outputFilename += "-stage" + std::to_string(i + 1);
					}
					stageInputName = stageCi.outputFilename;
				}
				stageCommandLines.emplace_back(stageCi.appName + " " + stageCi.toCmdLineArgs());
			}

			// make the ConvertStage:
			Fraction f = fractions[i];
			f.numerator *= stageCi.overSamplingFactor;
			f.denominator *= stageCi.overSamplingFactor;
			convertStages.emplace_back(f.numerator, f.denominator, firFilter, false);

			// add Group Delay:
			groupDelay *= (static_cast<double>(f.numerator) / f.denominator); // scale previous delay according to conversion ratio
			groupDelay += (ci.bMinPhase || !ci.bDelayTrim) ? 0 : (filterTaps.size() - 1) / 2 / f.denominator; // add delay introduced by this stage

			// calculate size of output buffer for this stage:
			double cumulativeNumerator = 1.0;
			double cumulativeDenominator = 1.0;
			for (int j = 0; j <= i; j++) {
				cumulativeNumerator *= fractions[j].numerator;
				cumulativeDenominator *= fractions[j].denominator;
			}
			size_t outBufferSize = static_cast<size_t>(std::ceil(BUFFERSIZE * cumulativeNumerator / cumulativeDenominator));

			// conditionally show output buffer size
			if (ci.bShowStages) {
				//std::cout << cumulativeNumerator << " / " << cumulativeDenominator << "\n";
				std::cout << "Output Buffer Size: " << outBufferSize << "\n\n" << std::endl;
			}

			// make output buffer for this stage (last stage doesn't need one)
			if (i != indexOfLastStage) {
				intermediateOutputBuffers.emplace_back(std::vector<FloatType>(outBufferSize, 0.0));
			}

			// set input rate of next stage
			inputRate = stageCi.outputSampleRate;

		} // ends loop over i

		if (ci.bShowStages) {
			std::cout << "Command lines to do this conversion in discreet steps:\n";
			for (auto& cmdline : stageCommandLines) {
				std::cout << cmdline << "\n";
			}
			std::cout << std::endl;
		}
	} // initMultistage()

private:
	ConversionInfo ci;
	double groupDelay;
	std::vector<ResamplingStage<FloatType>> convertStages;
	int numStages{};
	int indexOfLastStage{};
	std::vector<std::vector<FloatType>> intermediateOutputBuffers;	// intermediate output buffer for each ConvertStage;
	std::vector<std::string> stageCommandLines;
	bool isMultistage;
	bool isBypassMode;
	double gain;
};

} // namespace ReSampler

#endif // SRCONVERT_H
