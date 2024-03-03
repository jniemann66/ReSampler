/*
* Copyright (C) 2024 Judd Niemann - All Rights Reserved.
* You may use, distribute and modify this code under the
* terms of the GNU Lesser General Public License, version 2.1
*
* You should have received a copy of GNU Lesser General Public License v2.1
* with this file. If not, please refer to: https://github.com/jniemann66/ReSampler
*/

#ifndef MPXDECODE_H
#define MPXDECODE_H

// FM Broadcast Multiplex Decoder

#include <vector>
#include <sndfile.h>
#include <sndfile.hh>

#include "FIRFilter.h"
#include "biquad.h"

static const double M_TWOPI = 2.0 * M_PI;

// debugging / diagnostic switches (not enabled for normal use)
// #define MPXDECODER_TUNE_PILOT_AGC
// #define MPXDECODER_DEBUG_PLL_SYNC
// #define MPXDECODER_SAVE_PILOT
// ---

enum PilotPresence
{
	PilotPresenceUnknown,
	PilotPresent,
	PilotNotPresent
};

// NCO : numerically - controlled oscillator
class NCO
{
public:
	NCO(int sampleRate, double centerFrequencyHz = 19000.0) : sampleRate(sampleRate), centerFrequencyHz(centerFrequencyHz) 
	{ 
		hzToAngularFactor = M_TWOPI / NCO::sampleRate; // for converting Hz to radians / sample
		angularToHzFactor = NCO::sampleRate / M_TWOPI; // for converting radians / sample to Hz
		setFrequency(centerFrequencyHz);
		biquad1.setCoeffs(0.002206408204233198, 0.004412816408466396, 0.002206408204233198, -1.8043019281465769, 0.814646474444927);
		biquad2.setCoeffs(0.00390625, 0.0078125, 0.00390625, -1.8486208186651036, 0.8619515640441029);        
	}

	void sync(double input)
	{
		// advance position
		theta += angularFreq;
		if(theta > M_PI) {
			theta -= M_TWOPI;
		}

		// detect phase
		double phase = biquad2.filter(biquad1.filter(cosOut * input));

#ifdef MPXDECODER_DEBUG_PLL_SYNC
		std::cout << ph << ", f=" << getFrequency() << "\n";
#endif

		// some constant that I don't know how to set properly (controls how aggressively the frequency is altered)
        constexpr double someStoopidConstant = 10.0;

		// adjust frequency
        setFrequency(centerFrequencyHz + someStoopidConstant * phase);
	}

    // getDoubled() : get frequency-doubled oscillator output
    // (call this function first, because it evaluates sinOut and cosOut)
	double getDoubled() {
		sinOut = std::sin(theta);
		cosOut = std::cos(theta);
		return 2.0 * cosOut * sinOut;
	}

	// getTripled() : get frequency-tripled oscillator output
	double getTripled() {
		// Chebyshev polynomial of the SECOND kind multipled by sinϴ
		// sin(3ϴ) = (4cos^2(ϴ) - 1) * sin(ϴ)
		return (4.0 * cosOut * cosOut - 1.0) * sinOut;
	}

	double getFrequency() const
	{
        return angularToHzFactor * angularFreq;
	}

	void setFrequency(double value)
    {
        angularFreq = hzToAngularFactor * value;
	}

	static void saveIIRresponse(const std::string& filename)
	{
		SndfileHandle sndfile(filename, SFM_WRITE, SF_FORMAT_WAV | SF_FORMAT_FLOAT, 1, 300000);
		std::vector<double> impulseResponse(10000, 0.0);

		// 4-pole LPF for filtering the pilot tone phase-detector
		ReSampler::Biquad<double> f1(0.002206408204233198, 0.004412816408466396, 0.002206408204233198, -1.8043019281465769, 0.814646474444927);
		ReSampler::Biquad<double> f2(0.00390625, 0.0078125, 0.00390625, -1.8486208186651036, 0.8619515640441029);

		impulseResponse[100] = f2.filter(f1.filter(1.0));
		for(int i = 101; i < static_cast<int>(impulseResponse.size()); i++)
		{
			impulseResponse[i] = f2.filter(f1.filter(0.0));
		}

		sndfile.writef(impulseResponse.data(), impulseResponse.size());
	}

private:
	ReSampler::Biquad<double> biquad1;
	ReSampler::Biquad<double> biquad2;

	int sampleRate;
	double centerFrequencyHz;
	double angularFreq;
	double hzToAngularFactor;
	double angularToHzFactor;

	double theta{0.0};
    double cosOut{1.0}; // oscillator cosine output
    double sinOut{0.0}; // oscillator sine output
};

class MpxDecoder
{
public:
	MpxDecoder(int sampleRate) : nco(sampleRate)
	{
		// create filters
		auto f0 = make19KhzBandpass<double>(sampleRate);
		auto f1 = make38KhzBandpass<double>(sampleRate);
		auto f2 = make57KhzBandpass<double>(sampleRate);
		auto lpf = make15khzLowpass<double>(sampleRate);

		length = f0.size();
		delayLine.resize(length, 0.0);
		centerTap = (length - 1) / 2;
		currentIndex = length - 1;

		filters.emplace_back(f0.data(), f0.size());
		filters.emplace_back(f1.data(), f1.size());
		filters.emplace_back(f2.data(), f2.size());
		filters.emplace_back(lpf.data(), lpf.size()); // left lowpass
		filters.emplace_back(lpf.data(), lpf.size()); // right lowpass

		// these values determined experimentally:
		// (#define MPXDECODER_TUNE_PILOT_AGC to debug & tweak)
		decreaseRate = std::pow(10.0, /* dB per sec = */ -12.0 / sampleRate / 20.0);
		increaseRate = std::pow(10.0, 64.0 / sampleRate / 20.0);
		setStereoWidth(0.5);

#ifdef MPXDECODER_SAVE_PILOT
        pilotOutFile.reset(new SndfileHandle("/tmp/pilot.wav", SFM_WRITE, SF_FORMAT_WAV | SF_FORMAT_FLOAT, 2, sampleRate));
#endif

	}

#ifdef MPXDECODER_TUNE_PILOT_AGC
	~MpxDecoder()
	{
		int64_t totalCount = plusCount + stableCount + minusCount;
		std::cout << 100.0 * plusCount / totalCount << "%, "
				  << 100.0 * stableCount / totalCount << "%, "
				  << 100.0 * minusCount / totalCount << "%" << std::endl;

		std::cout << "Peak Pilot Gain: " << peakPilotGain << std::endl;
	}
#endif

	template<typename FloatType>
	std::pair<FloatType, FloatType> decode(FloatType input)
	{
		delayLine[currentIndex] = input; // place input into history
		if(currentIndex == 0) {
			currentIndex = length - 1;
		} else {
			currentIndex--;
		}
		int d = currentIndex + centerTap;
		FloatType mono = delayLine[d >= length ? d - length : d];

		filters.at(0).put(input);
		filters.at(1).put(input);

		FloatType pilotRaw = filters.at(0).get();
		FloatType sideRaw = filters.at(1).get();
		FloatType pilot = pilotRaw * pilotGain;
		FloatType pilotAbs = std::fabs(pilot);

		if(pilotAbs > pilotPeak) {
			pilotPeak = pilotAbs;
		}

#ifdef MPXDECODER_TUNE_PILOT_AGC
  //      std::cout << pilotGain << ", " << pilotPeak << "\n";
#endif

		if(pilotGain >= pilotMaxGain) {
			pilotGain = pilotMaxGain;
			pilotPresence = PilotNotPresent;
		} else if(pilotPeak < pilotStableLow) { // pilot too quiet
			pilotGain *= increaseRate;

#ifdef MPXDECODER_TUNE_PILOT_AGC
			plusCount++;
#endif

		} else if(pilotPeak > pilotStableHigh) { // pilot too loud
			pilotGain *= decreaseRate;
			pilotPeak *= decreaseRate;


#ifdef MPXDECODER_TUNE_PILOT_AGC
			minusCount++;
#endif

		} else { // stable pilot tone

#ifdef MPXDECODER_TUNE_PILOT_AGC
			stableCount++;
#endif
			pilotPresence = PilotPresent;
		}

#ifdef MPXDECODER_TUNE_PILOT_AGC
		peakPilotGain = std::max(pilotGain, peakPilotGain);
#endif
		FloatType left;
		FloatType right;

		double local38k;

		if(pilotPresence != PilotPresent) {
			left = mono;
			right = mono;
		} else { // todo: fade from mono to stereo (& back ...)

			// do the spectrum shift
			constexpr double scaling = 2.5 * 2;
			local38k = nco.getDoubled();

#ifdef MPXDECODER_SAVE_PILOT
            double pilotOut[2];
            pilotOut[0] = pilot;
            pilotOut[1] = local38k;
            pilotOutFile->write(pilotOut, 2);
#endif
			FloatType side = scaling * local38k * sideRaw;
			nco.sync(pilot);

			// separate L, R stereo channels
			left = stereoGain * (mono + stereoWidth * side);
			right = stereoGain * (mono - stereoWidth * side);
		}

		if(!lowpassEnabled) {
			return {left, right};
		}

		// filter & return output
		filters.at(3).put(left);
		filters.at(4).put(right);

		return {filters.at(3).get(), filters.at(4).get()};
	}

	template <typename FloatType>
	static std::vector<FloatType> makeBandpass(int sampleRate, double ft1, double ft2)
	{
		// determine cutoff frequency and steepness
		double nyquist = sampleRate / 2.0;
		double steepness = 0.090909091 / (2000.0 / nyquist);

		// determine filtersize
		int filterSize = static_cast<int>(
					std::min<int>(FILTERSIZE_BASE * steepness, FILTERSIZE_LIMIT)
					| 1 // ensure that filter length is always odd
					);

		// lower transition
		std::vector<FloatType> filterTaps1(filterSize, 0);
		FloatType* pFilterTaps1 = &filterTaps1[0];
		ReSampler::makeLPF<FloatType>(pFilterTaps1, filterSize, ft1, sampleRate);

		// upper transition
		std::vector<FloatType> filterTaps2(filterSize, 0);
		FloatType* pFilterTaps2 = &filterTaps2[0];
		ReSampler::makeLPF<FloatType>(pFilterTaps2, filterSize, ft2, sampleRate);

		// make bandpass
		for(int i = 0; i < filterSize; i++) {
			filterTaps2[i] -= filterTaps1.at(i);
		}

		int sidelobeAtten = 60;
		ReSampler::applyKaiserWindow<FloatType>(pFilterTaps2, filterSize, ReSampler::calcKaiserBeta(sidelobeAtten));
		return filterTaps2;
	}

	// 15khz lowpass for audio output
	template<typename FloatType>
	static std::vector<FloatType> make15khzLowpass(int sampleRate)
	{
		// determine filter steepness
		double nyquist = sampleRate / 2.0;
		double steepness = 0.090909091 / (lpfW / nyquist);

		// determine filtersize
		int filterSize = static_cast<int>(
					std::min<int>(FILTERSIZE_BASE * steepness, FILTERSIZE_LIMIT)
					| 1 // ensure that filter length is always odd
					);

		std::vector<FloatType> filterTaps1(filterSize, 0);
		FloatType* pFilterTaps1 = &filterTaps1[0];
		ReSampler::makeLPF<FloatType>(pFilterTaps1, filterSize, lpfT, sampleRate);
		int sidelobeAtten = 160;
		ReSampler::applyKaiserWindow<FloatType>(pFilterTaps1, filterSize, ReSampler::calcKaiserBeta(sidelobeAtten));
		return filterTaps1;
	}

	// 19khz bandpass filter for the Pilot Tone
	template<typename FloatType>
	static std::vector<FloatType> make19KhzBandpass(int sampleRate)
	{
		return makeBandpass<FloatType>(sampleRate, 18900, 19100);
	}

	// 38khz bandpass filter for the Audio Subcarrier
	template<typename FloatType>
	static std::vector<FloatType> make38KhzBandpass(int sampleRate)
	{
		return makeBandpass<FloatType>(sampleRate, 23000, 53000);
	}

	// 57khz bandpass filter for RDS / RBDS
	template<typename FloatType>
	static std::vector<FloatType> make57KhzBandpass(int sampleRate)
	{
		return makeBandpass<FloatType>(sampleRate, 54000, 60000);
	}

	// utility function for testing / tweaking the performance of the bandpass filters
	static void saveFilters1(const std::string& filename)
	{
		std::vector<double> filt1 = make19KhzBandpass<double>(192000);
		std::vector<double> filt2 = make38KhzBandpass<double>(192000);
		SndfileHandle sndfile(filename, SFM_WRITE, SF_FORMAT_WAV | SF_FORMAT_FLOAT, 2, 192000);
		std::cout << "filter size " << filt1.size() << std::endl;
		std::vector<double> interleaved;
		interleaved.reserve(2 * filt1.size());
		for(int i = 0; i < static_cast<int>(filt1.size()); i++) {
			interleaved.push_back(filt1.at(i));
			interleaved.push_back(filt2.at(i));
		}
		sndfile.writef(interleaved.data(), filt1.size());
	}

	static double getLpfT()
	{
		return lpfT;
	}

	static double getLpfW()
	{
		return lpfW;
	}

	bool getLowpassEnabled() const
	{
		return lowpassEnabled;
	}

	void setLowpassEnabled(bool value)
	{
		lowpassEnabled = value;
	}

	double getStereoWidth() const
	{
		return stereoWidth;
	}

	void setStereoWidth(double value)
	{
		stereoWidth = value;
		stereoGain = 1.0 / (1.0 + value);
	}

private:
	std::vector<ReSampler::FIRFilter<double>> filters;
	NCO nco; // PLL local oscillator

	static constexpr double lpfT = 15500.0;	// LPF transition freq (Hz)
	static constexpr double lpfW = 3500.0;	// LPF transition width (Hz)
	static constexpr double pilotStableLow = 0.95;
	static constexpr double pilotStableHigh = 1.05;

	// if more gain than this is needed, then something is wrong with the Pilot Tone:
	static constexpr double pilotMaxGain = 15.0;

	std::vector<double> delayLine;
	int length;
	int currentIndex;
	int centerTap;

	PilotPresence pilotPresence{PilotPresenceUnknown};
	bool lowpassEnabled{true}; // do final stereo 15khz LPF or not ?
    double pilotPeak{0.0};
	double pilotGain{1.0};
	double increaseRate;
	double decreaseRate;
	double stereoWidth;
	double stereoGain;

#ifdef MPXDECODER_TUNE_PILOT_AGC
	int64_t plusCount{0};
	int64_t minusCount{0};
	int64_t stableCount{0};
	double peakPilotGain{0.0};
#endif

#ifdef MPXDECODER_SAVE_PILOT
    std::unique_ptr<SndfileHandle> pilotOutFile;
#endif

};

#endif // MPXDECODE_H
