/*
* Copyright (C) 2023 Judd Niemann - All Rights Reserved.
* You may use, distribute and modify this code under the
* terms of the GNU Lesser General Public License, version 2.1
*
* You should have received a copy of GNU Lesser General Public License v2.1
* with this file. If not, please refer to: https://github.com/jniemann66/ReSampler
*
*/

#ifndef IQDEMODULATOR_H
#define IQDEMODULATOR_H

#include <string>
#include <cstdint>
#include <memory>
#include <vector>
#include <iostream>
#include <iomanip>
#include <cmath>
#include <map>
#include <iostream>

#include <sndfile.h>
#include <sndfile.hh>

#include "biquad.h"
#include "mpxdecode.h"

// #define COLLECT_IQ_STATS
// #define ALTERNATIVE_FM_FUNCTIONS

#define ERROR_IQFILE_WFM_SAMPLERATE_TOO_LOW (0xff01)
#define ERROR_IQFILE_TWO_CHANNELS_EXPECTED (0xff02)

namespace  ReSampler {

enum ModulationType
{
	ModulationTypeNone = 0,
	NFM,
	AM,
	LSB,
	USB,
	WFM,
	WFM_NO_LOWPASS,
	DSB,
	CW
};

enum DeEmphasisType
{
	NoDeEmphasis = 0,
	DeEmphasis50 = 0x10,
	DeEmphasis75 = 0x20
};

static const std::map<std::string, ModulationType> modulationTypeMap
{
	{"NONE", ModulationType::ModulationTypeNone},
	{"NFM", ModulationType::NFM},
	{"AM", ModulationType::AM},
	{"LSB", ModulationType::LSB},
	{"USB", ModulationType::USB},
	{"WFM", ModulationType::WFM},
	{"DSB", ModulationType::DSB},
	{"CW", ModulationType::CW}
};

static const std::map<std::string, DeEmphasisType> deEmphasisTypeMap
{
	{"NONE", NoDeEmphasis},
	{"50", DeEmphasis50},
	{"75", DeEmphasis75}
};

class IQFile
{
public:

	IQFile(const std::string& fileName) : sndfileHandle(new SndfileHandle(fileName))
	{
	}

	IQFile(const std::string& fileName, int infileMode, int infileFormat, int infileChannels, int infileRate) :
		sndfileHandle(new SndfileHandle(fileName, infileMode, infileFormat & 0xFFFF00FF, infileChannels, infileRate))
	{
		// Extract modulation type from 2nd-last byte of file format code.
		// (Note: libsndfile has this for the subformat mask:
		// SF_FORMAT_SUBMASK = 0x0000FFFF
		// So far, only the least-significant byte has been used. ie: 0x000000FF.
		// If they ever add more formats in the future which use the upper byte,
		// then this strategy may need reevaluation ...)

		int format = (infileFormat & 0x0000FF00) >>  8;
		modulationType = static_cast<ModulationType>(format & 0x0f);
		deEmphasisType = static_cast<DeEmphasisType>(format & 0x30);

		bool enableLowpass = true;
		if(modulationType == WFM_NO_LOWPASS) {
			enableLowpass = false;
			modulationType = WFM;
		}

		if(modulationType == NFM || modulationType == WFM) { // initialize FM parameters

			// initialize differentiators
			differentiatorType = samplerate() < 248000 ? 5 : 8;
			differentiatorCoeffs = differentiators.at(differentiatorType);
			differentiatorLength = differentiatorCoeffs.size();
			differentiatorDelay = differentiatorLength / 2;

#ifdef ALTERNATIVE_FM_FUNCTIONS
			historyI.resize(differentiatorLength, 0.0);
            historyQ.resize(differentiatorLength, 0.0);
#endif

			phaseHistory.resize(differentiatorLength, 0.0);
			differentiatorIndex = differentiatorLength - 1;

			// for high sample rates, a smaller portion of the differentiator slope is used,
			// resulting in lower output level. So, compensate gain ...
			if(samplerate() > 300000) {
				gainTrim *= (samplerate() / 300000.0); // todo: scale the differentiator coeffs instead ? (might save a multiply on every sample)
			}

			// set de-emphasis
			if(modulationType == WFM) {
				if(samplerate() != 0) {
                    // to-do: more deemphasis settings ??
					switch (deEmphasisType) {
					case NoDeEmphasis:
						break;
					case DeEmphasis50:
						setDeEmphasisTc(2, samplerate(), 50);
						break;
					case DeEmphasis75:
						setDeEmphasisTc(2, samplerate(), 75);
						break;
					}

					mpxDecoder = std::unique_ptr<MpxDecoder>(new MpxDecoder(samplerate()));
					mpxDecoder->setLowpassEnabled(enableLowpass);
				}
			}
		}
	}

#ifdef COLLECT_IQ_STATS
	~IQFile()
	{
		double iAvg = sumI / framesRead;
		double qAvg = sumQ / framesRead;
		double iRMS = std::sqrt(sumISquared / framesRead);
		double qRMS = std::sqrt(sumQSquared / framesRead);
		std::cout << "I: peak=" << peakI << ", avg=" << iAvg << ", RMS=" << iRMS << std::endl;
		std::cout << "Q: peak=" << peakQ << ", avg=" << qAvg << ", RMS=" << qRMS << std::endl;
	}
#endif

	int error() {

		if(sndfileHandle == nullptr) {
			return SF_ERR_UNRECOGNISED_FORMAT;
		}

		if(sndfileHandle->samplerate() == 0) {
			return SF_ERR_UNRECOGNISED_FORMAT;
		}

		if(modulationType == WFM && sndfileHandle->samplerate() < 116000) {
			return ERROR_IQFILE_WFM_SAMPLERATE_TOO_LOW;
		}

		if(sndfileHandle->channels() != 2) {
			return ERROR_IQFILE_TWO_CHANNELS_EXPECTED;
		}

		return sndfileHandle->error();
	}

	int channels() {
		if(modulationType == WFM) {
			return 2; // FM stereo
		} else {
			return 1;
		}
	}

	int samplerate() {
		return sndfileHandle == nullptr ? 0 : sndfileHandle->samplerate();
	}

	int64_t frames() {
		return sndfileHandle == nullptr ? 0LL : sndfileHandle->frames();
	}

	int format() {
		return sndfileHandle == nullptr ? 0 : sndfileHandle->format();
	}

	template<typename FloatType>
	int64_t read(FloatType* inbuffer, int64_t count) {

		if(sndfileHandle == nullptr) {
			return 0LL;
		}

		if(wavBuffer.size() < count) {
			wavBuffer.resize(count);
		}

		int64_t samplesRead = sndfileHandle->read(wavBuffer.data(), count);
		int64_t j = 0;

		switch(modulationType) {
		case AM:
			// Amplitude Modulation
			for(int64_t i = 0; i < samplesRead; i += 2) {
				inbuffer [j++] = demodulateAM(wavBuffer.at(i), wavBuffer.at(i + 1));
			}
			break;
		case LSB:
		case USB:
			// Single Side Band
			for(int64_t i = 0; i < samplesRead; i += 2) {
				// just copy I-component
				inbuffer[j++] = wavBuffer.at(i);
			}
			break;
		case WFM:
			// Wideband FM:
			if(deEmphasisType == NoDeEmphasis) {
				for(int64_t i = 0; i < samplesRead; i += 2) {
					// demodulate, decode
					std::pair<FloatType, FloatType> decoded = mpxDecoder->decode(demodulateFM(wavBuffer.at(i), wavBuffer.at(i + 1)));
					inbuffer[j++] = decoded.first;
					inbuffer[j++] = decoded.second;
				}
			} else {
				for(int64_t i = 0; i < samplesRead; i += 2) {
					// demodulate, decode, deemphasize
					double iVal = wavBuffer.at(i);
					double qVal = wavBuffer.at(i + 1);

#ifdef COLLECT_IQ_STATS
					peakI = std::max(peakI, std::abs(iVal));
					peakQ = std::max(peakQ, std::abs(qVal));
					sumI += iVal;
					sumQ += qVal;
					sumISquared += (iVal * iVal);
					sumQSquared += (qVal * qVal);
					framesRead++;
#endif

					std::pair<FloatType, FloatType> decoded = mpxDecoder->decode(demodulateFM(iVal, qVal));
					inbuffer[j++] = deEmphasisFilters[0].filter(decoded.first);
					inbuffer[j++] = deEmphasisFilters[1].filter(decoded.second);
				}
			}
			break;
		default:
			// Narrowband FM
			for(int64_t i = 0; i < samplesRead; i += 2) {
				inbuffer[j++] = demodulateFM(wavBuffer.at(i), wavBuffer.at(i + 1));
			}
		}

		return j;
	}

	sf_count_t seek(int64_t frames, int whence) {
		if(sndfileHandle == nullptr) {
			return 0LL;
		}

		return sndfileHandle->seek(frames, whence);
	}

	// getters
	ModulationType getModulationType() const
	{
		return modulationType;
	}

	// setters
	void setModulationType(const ModulationType &value)
	{
		modulationType = value;
	}

private:

	// demodulateFM() : atan2, arbitrary FIR length
	template<typename FloatType>
	FloatType demodulateFM(FloatType i, FloatType q)
	{
		// place input into history
		z0.real(i);
		z0.imag(q);

		// determine angle between previous and latest complex z
		auto dz = z0 * std::conj(z1);
		phase += std::arg(dz);
		z1 = z0;

        // place input into history
		phaseHistory[differentiatorIndex] = phase;

        // perform the convolution
		FloatType dP{0.0}; // differentiator result
        int p = differentiatorIndex;
        for(int j = 0 ; j < differentiatorLength; j++) {
			FloatType vP = phaseHistory.at(p);
            if(++p == differentiatorLength) {
                p = 0; // wrap
            }
            dP += differentiatorCoeffs[j] * vP;
        }

        // update the current index
        if(differentiatorIndex == 0) {
            differentiatorIndex = differentiatorLength - 1; // wrap
        } else {
            differentiatorIndex--;
        }

        return gainTrim * dP;
	}

#ifdef ALTERNATIVE_FM_FUNCTIONS
	// demodulateFM2() : atan2-free, arbitrary FIR length
    template<typename FloatType>
	FloatType demodulateFM2(FloatType i, FloatType q)
    {
        static constexpr double threshold = -45.0;
        static const double c = std::pow(10.0, threshold / 20.0);
        FloatType dI{0.0}; // differentiated I
        FloatType dQ{0.0}; // differentiated Q

        // place input into history
        historyI[differentiatorIndex] = i;
        historyQ[differentiatorIndex] = q;

        // get position of delay tap
        FloatType delayedI;
        FloatType delayedQ;
        int delayIndex = differentiatorIndex + differentiatorDelay;
        if(delayIndex >= differentiatorLength) {
            delayIndex -= differentiatorLength;
        }

        // get delayed values from history
        delayedI = historyI.at(delayIndex);
        delayedQ = historyQ.at(delayIndex);

        // perform the convolution
        int p = differentiatorIndex;
        for(int j = 0 ; j < differentiatorLength; j++) {
            FloatType vI = historyI.at(p);
            FloatType vQ = historyQ.at(p);
            if(++p == differentiatorLength) {
                p = 0; // wrap
            }
            dI += differentiatorCoeffs[j] * vI;
            dQ += differentiatorCoeffs[j] * vQ;
        }

        // update the current index
        if(differentiatorIndex == 0) {
            differentiatorIndex = differentiatorLength - 1; // wrap
        } else {
            differentiatorIndex--;
        }

        double gain = 2.0 / (c + delayedI * delayedI + delayedQ * delayedQ);
        return gain * (dQ * delayedI - dI  * delayedQ);
    }

	// demodulateFM3() : atan2, 2nd-order FIR
	template<typename FloatType>
	FloatType demodulateFM3(FloatType i, FloatType q)
	{
		static constexpr double threshold = -40.0; // dB
		static const double c = std::pow(10.0, threshold / 20.0);
		static const double gainTrim = 2.75494098472591;

		// determine magnitude and gain
		double iSquared = i * i;
		double qSquared = q * q;
		double a = std::sqrt(iSquared + qSquared);
		double g = 2.0 / std::max(a, c);

		// place input into history
		z0.real(i * g);
		z0.imag(q * g);

		auto dz = z0 * std::conj(z1);
		z1 = z0;
		return gainTrim * std::arg(dz);
	}

	// demodulateFM4() : atan2-free, 2nd order FIR
	template<typename FloatType>
	FloatType demodulateFM4(FloatType i, FloatType q)
	{
		static constexpr double threshold = -45.0;
		static const double c = std::pow(10.0, threshold / 20.0);

		// this is actually quite simple, thanks to some clever calculus tricks.
		// see https://www.embedded.com/dsp-tricks-frequency-demodulation-algorithms/

		i2 = i1;
		i1 = i0;
		i0 = i;
		q2 = q1;
		q1 = q0;
		q0 = q;

		double gain = 2.0 / (c + i1 * i1 + q1 * q1);
		return gain * (((q0 - q2) * i1) - ((i0 - i2) * q1));
	}
#endif // ALTERNATIVE_FM_FUNCTIONS

	template<typename FloatType>
	FloatType demodulateAM(FloatType i, FloatType q)
	{
		static constexpr FloatType scale = 0.7071; // << 1/sqrt(2)
		return scale * std::sqrt(i * i + q * q);
	}

	// setDeEmphasisHz() : set up deemphasis filter, given a frequency in Hz
    //	double tau = 1 / (2 * pi * f); // Hz to time constant
	//	double f = 2122.1; // 75 us
	//  double f = 3183.1; // 50 us

	void setDeEmphasisHz(int channels, int sampleRate, double freqHz)
	{
		setDeEmphasisTc(channels, sampleRate, (1.0 / (M_TWOPI * freqHz)));
	}

	// setDeEmphasisTc() : set up deemphasis filter, given a time constant in microseconds
	void setDeEmphasisTc(int channels, int sampleRate, double tc = 50.0 /* microseconds */)
	{
		deEmphasisFilters.resize(channels);
		double p1 = -exp(-1.0 / (sampleRate * tc * 0.000001));
		double z1 = (1 + p1) / 5.0;
		for(auto& biquad : deEmphasisFilters) {
			biquad.setCoeffs(z1, z1, 0.0, p1, 0.0);
			biquad.reset();
		}
	}

private:
	// resources
	std::unique_ptr<SndfileHandle> sndfileHandle;
	std::unique_ptr<MpxDecoder> mpxDecoder;
	std::vector<double> wavBuffer;
    std::vector<Biquad<double>> deEmphasisFilters;
	std::vector<double> differentiatorCoeffs;

	// properties
	double gainTrim{1.1};
	ModulationType modulationType{ModulationType::NFM};
	DeEmphasisType deEmphasisType{DeEmphasis50};

#ifdef ALTERNATIVE_FM_FUNCTIONS
	// registers used for demodulating FM (atan2-less)
	double i0{0.0};
	double i1{0.0};
	double i2{0.0};
	double q0{0.0};
	double q1{0.0};
	double q2{0.0};
#endif

	// registers for demodulating FM (atan2 version)
	std::complex<double> z0{0.0};
	std::complex<double> z1{0.0};
    double phase{0.0};

	// default differentiator type
	int differentiatorType{8};

	// collection of differentiator coefficients
	const std::vector<std::vector<double>> differentiators
	{
/*00*/	{0.0},
/*01*/	{
			1.0,
			-1.0
		},
/*02*/	{
			1.0,
			0.0,
			-1.0
		},
/*03*/	{
			0.0209,
			-0.1128,
			1.2411,
			-1.2411,
			0.1128,
			-0.0209
		},
/*04*/	{
			-0.0081,
			0.0341,
			-0.1266,
			1.2620,
			-1.2620,
			0.1266,
			-0.0341,
			0.0081
		},
/*05*/	{
			0.0035,
			-0.0140,
			0.0401,
			-0.1321,
			1.2639,
			-1.2639,
			0.1321,
			-0.0401,
			0.0140,
			-0.0035
		},
/*06*/	{
			0.0209,
			0.0,
			-0.1128,
			0.0,
			1.2411,
			0.0,
			-1.2411,
			0.0,
			0.1128,
			0.0,
			-0.0209
		},
/*07*/	{
			-0.0081,
			0.0,
			0.0341,
			0.0,
			-0.1266,
			0.0,
			1.2620,
			0.0,
			-1.2620,
			0.0,
			0.1266,
			0.0,
			-0.0341,
			0.0,
			0.0081
		},
/*08*/	{
			0.0035,
			0.0,
			-0.0140,
			0.0,
			0.0401,
			0.0,
			-0.1321,
			0.0,
			1.2639,
			0.0,
			-1.2639,
			0.0,
			0.1321,
			0.0,
			-0.0401,
			0.0,
			0.0140,
			0.0,
			-0.0035
		}
	};

    int differentiatorLength;
    int differentiatorDelay;
    int differentiatorIndex;

#ifdef ALTERNATIVE_FM_FUNCTIONS
    std::vector<double> historyI;
    std::vector<double> historyQ;
#endif

	std::vector<double> phaseHistory;

#ifdef COLLECT_IQ_STATS
	int64_t framesRead{0ll};
	double peakI{0.0};
	double peakQ{0.0};
	double sumI{0.0};
	double sumQ{0.0};
	double sumISquared{0.0};
	double sumQSquared{0.0};
#endif

public:

	// writeDifferentiators() : utility function do dump differentiator coeffs to wav files,
	// for testing and analysis.

	void writeDifferentiators(const std::string& name)
	{
		for(int c = 0; c < differentiators.size(); c++) {

			std::string filename = name + std::to_string(c) + ".wav";
			SndfileHandle sndfile(filename, SFM_WRITE, SF_FORMAT_WAV | SF_FORMAT_FLOAT, 1, 300000);
			std::vector<double> waveform(10000, 0.0);

			auto impulseResponse = differentiators.at(c);
			int iLength = impulseResponse.size();
			int offset = (waveform.size() - iLength) / 2;

			for(int i = 0; i < iLength; i++ ) {
				waveform[offset + i] = impulseResponse.at(i);
			}

			sndfile.writef(waveform.data(), waveform.size());
		}
	}

	// generateFMTestTone() : utility function for testing.
	// Writes an FM-modulated tone to a file in IQ format.

	static void generateFMTestTone(const std::string& filename, double toneFreq = 1000)
	{
		constexpr int sampleRate = 256000;
		constexpr double maxModulationHz = 53000.0;
		constexpr double duration = 2.0;
		constexpr int length = sampleRate * duration;
		const double omegaS = M_TWOPI * toneFreq / sampleRate;

		SndfileHandle sndfile(filename, SFM_WRITE, SF_FORMAT_WAV | SF_FORMAT_FLOAT, 2, sampleRate);

		double thetaS = 0.0;
		double theta = 0.0;

		for(int s = 0; s < length; s++) {
			double signal = sin(thetaS);
			thetaS += omegaS;
			if(thetaS > M_TWOPI) {
				thetaS -= M_TWOPI;
			} else if (thetaS < -M_TWOPI) {
				thetaS += M_TWOPI;
			}

			std::vector<double> iq
			{
				cos(theta),
				sin(theta)
			};

			sndfile.writef(iq.data(), 1);

			double omega = M_TWOPI * maxModulationHz * signal / sampleRate;
			theta += omega;
			if(theta > M_TWOPI) {
				theta -= M_TWOPI;
			} else if (theta < -M_TWOPI) {
				theta += M_TWOPI;
			}
		}
	}
};

} // namespace  ReSampler

#endif // IQDEMODULATOR_H
