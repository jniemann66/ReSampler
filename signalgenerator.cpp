#include "signalgenerator.h"

#define _USE_MATH_DEFINES
#include <cmath>
#include <vector>
#include "sndfile.hh"

#if defined (__MINGW64__) || defined (__MINGW32__) || defined (__GNUC__)
#ifdef USE_QUADMATH
#include <quadmath.h>
#endif
#endif

#ifndef USE_QUADMATH

void SignalGenerator::generateExpSweep(const std::string& filename, int sampleRate, int format, double duration, int nOctaves, double amplitude_dB)
{
	int pow2P = 1 << nOctaves;
	int pow2P1 = 1 << (nOctaves + 1);
	double amplitude = pow(10.0, (amplitude_dB / 20.0));
	double M = pow2P1 * nOctaves * M_LN2;
	int N = lround((duration * sampleRate) / M) * M; // N must be integer multiple of M
	double y = log(pow2P);
	double C = (N * M_PI / pow2P) / y;
	double TWOPI = 2.0 * M_PI;

	SndfileHandle outFile(filename, SFM_WRITE, format, 1, sampleRate);
	std::vector<double> signal(N, 0.0);

	for (int n = 0; n < N; n++) {
		signal[n] = amplitude * sin(fmod(C * exp(y * n / N), TWOPI));
	}

	outFile.write(signal.data(), N);
}

#else // QUAD PRECISION VERSION

void SignalGenerator::generateExpSweep(const std::string& filename, int sampleRate, int format, double duration, int nOctaves, double amplitude_dB)
{
	int pow2P = 1 << nOctaves;
	int pow2P1 = 1 << (nOctaves + 1);
	__float128 amplitude = pow(10.0Q, (amplitude_dB / 20.0Q));
	__float128 M = pow2P1 * nOctaves * M_LN2q;
	int N = lroundq((duration * sampleRate) / M) * M; // N must be integer multiple of M
	__float128 y = logq(pow2P);
	__float128 C = (N * M_PIq / pow2P) / y;
	__float128 TWOPI = 2.0Q * M_PIq;

	SndfileHandle outFile(filename, SFM_WRITE, format, 1, sampleRate);
	std::vector<double> signal(N, 0.0);

	for (int n = 0; n < N; n++) {
		signal[n] = amplitude * sinq(fmodq(C * expq(y * n / N), TWOPI));
	}

	outFile.write(signal.data(), N);
}

#endif

void SignalGenerator::generateImpulse(const std::string& filename, int sampleRate, int format, int N, int nOctaves, double amplitude_dB)
{
	double amplitude = pow(10.0, (amplitude_dB / 20.0));
	SndfileHandle outFile(filename, SFM_WRITE, format, 1, sampleRate);
	std::vector<double> signal(N, 0.0);
	signal[(N / 2)] = amplitude;

	outFile.write(signal.data(), N);
}
