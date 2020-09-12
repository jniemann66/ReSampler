/*
* Copyright (C) 2016 - 2020 Judd Niemann - All Rights Reserved.
* You may use, distribute and modify this code under the
* terms of the GNU Lesser General Public License, version 2.1
*
* You should have received a copy of GNU Lesser General Public License v2.1
* with this file. If not, please refer to: https://github.com/jniemann66/ReSampler
*/

// FIRFilter.h : simple FIR filter implementation

#ifndef FIRFFILTER_H_
#define FIRFFILTER_H_

#include "alignedmalloc.h"
#include "factorial.h"

#include <typeinfo>
#include <algorithm>
#include <iostream>
#include <iomanip>
#include <complex>
#include <cstring>
#include <cstdint>
#include <cassert>
#include <vector>
#include <immintrin.h>

#if defined(__ANDROID__)
#ifndef COMPILING_ON_ANDROID
#define COMPILING_ON_ANDROID
#endif
#endif

#include <fftw3.h>

#define WRAP_WITH_MEMCPY
#define FILTERSIZE_LIMIT 131071
#define FILTERSIZE_BASE 103

#ifdef USE_AVX
#define ALIGNMENT_SIZE 32
#else

#define ALIGNMENT_SIZE 16

#if (defined(_M_X64) || defined(__x86_64__) || defined(USE_SSE2)) // All x64 CPUs have SSE2 instructions, but some older 32-bit CPUs do not.
	#define USE_SIMD 1 // Vectorise main loop in FIRFilter::get() by using SSE2 SIMD instrinsics
	#define USE_SIMD_FOR_DOUBLES
#endif

#if defined (__MINGW64__) || defined (__MINGW32__) || defined (__GNUC__)
#ifdef USE_QUADMATH
#include <quadmath.h>
#ifndef FIR_QUAD_PRECISION
#define FIR_QUAD_PRECISION
#endif
#endif
#endif
#endif

#ifndef M_PI
#define M_PI (3.14159265358979323846)
#endif

namespace ReSampler {

	template <typename FloatType>
	class FIRFilter {

	public:

		// constructor:
		FIRFilter(const FloatType* taps, int length) :
			length(length), signal(nullptr), currentIndex(length-1), lastPut(0)

		{
			calcPaddedLength();

			for(int i = 0; i < numVecElements; i++) {
				kernelphases[i] = nullptr;
			}

			allocateBuffers();
			assertAlignment();
			clearBuffers();

			// initialize filter kernel and signal buffers
			for (int i = 0; i < length; ++i) {
				kernelphases[0][i] = taps[i];
				signal[i] = 0.0;
				signal[i + length] = 0.0;
			}

			// Populate additional kernel Phases:
			for(int n = 1; n < numVecElements; n++) {
				memcpy(1 + kernelphases[n], kernelphases[n - 1], (length - 1) * sizeof(FloatType));
			}
		}

		// deconstructor:
		~FIRFilter() {
			freeBuffers();
		}

		// copy constructor:
		FIRFilter(const FIRFilter& other) : length(other.length), currentIndex(other.currentIndex), lastPut(other.lastPut)
		{
			calcPaddedLength();
			allocateBuffers();
			assertAlignment();
			copyBuffers(other);
		}

		// move constructor:
		FIRFilter(FIRFilter&& other) noexcept :
			length(other.length), signal(other.signal), currentIndex(other.currentIndex), lastPut(other.lastPut)
		{
			calcPaddedLength();

			for(int i = 0; i < numVecElements; i++) {
				kernelphases[i] = other.kernelphases[i];
				other.kernelphases[i] = nullptr;
			}

			other.signal = nullptr;
			assertAlignment();
		}

		// copy assignment:
		FIRFilter& operator= (const FIRFilter& other)
		{
			length = other.length;
			calcPaddedLength();
			currentIndex = other.currentIndex;
			lastPut = other.lastPut;
			freeBuffers();
			allocateBuffers();
            assertAlignment();
			copyBuffers(other);
			return *this;
		}

		// move assignment:
		FIRFilter& operator= (FIRFilter&& other) noexcept
		{
			if(this != &other) // prevent self-assignment
			{
				length = other.length;
				calcPaddedLength();
				currentIndex = other.currentIndex;
				lastPut = other.lastPut;

				freeBuffers();

				signal = other.signal;
				for(int i = 0; i < numVecElements; i++) {
					kernelphases[i] = other.kernelphases[i];
					other.kernelphases[i] = nullptr;
				}
				other.signal = nullptr;
                assertAlignment();
			}
			return *this;
		}

		bool operator== (const FIRFilter& other) const
		{
			if (length != other.length)
				return false;

			for (int i = 0; i < paddedLength; i++) {
				if (kernelphases[0][i] != other.kernelphases[0][i])
					return false;
			}

			return true;
		}

		void reset() {
			// reset indexes:
			currentIndex = length - 1;
			lastPut = 0;

			// clear signal buffer
			for (int i = 0; i < paddedLength; ++i) {
				signal[i] = 0.0;
				signal[i + length] = 0.0;
			}
		}

		void put(FloatType value) { // Put signal in reverse order.
			signal[currentIndex] = value;

	#ifndef WRAP_WITH_MEMCPY
			signal[currentIndex + length] = value;
	#endif

			lastPut = currentIndex;
			if (currentIndex == 0) {
				currentIndex = length - 1; // Wrap

	#ifdef WRAP_WITH_MEMCPY
				memcpy(signal + length, signal, length * sizeof(FloatType)); // copy history to upper half of buffer
	#endif

			}
			else
				--currentIndex;
		}

		void putZero() {
			signal[currentIndex] = 0.0;

	#ifndef WRAP_WITH_MEMCPY
			signal[currentIndex + length] = 0.0;
	#endif

			if (currentIndex == 0) {
				currentIndex = length - 1; // Wrap

	#ifdef WRAP_WITH_MEMCPY
				memcpy(signal + length, signal, length * sizeof(FloatType)); // copy history to upper half of buffer
	#endif

			}
			else
				--currentIndex;
		}

		FloatType get() {

	#ifdef FIR_QUAD_PRECISION

			// scalar processing of quad-precision types
			__float128 output = 0.0Q;
			int index = currentIndex;
			for (int i = 0; i < length; ++i) {
				output += (__float128)signal[index] * (__float128)kernelphases[0][i];
				index++;
			}

	#elif defined(USE_AVX)

			// AVX processing of float types

			FloatType output = 0.0;
			int index = currentIndex & -8; // make multiple-of-eight
			int phase = currentIndex & 7;
			FloatType* kernel = kernelphases[phase];

			alignas(ALIGNMENT_SIZE) __m256 s;	// AVX Vector Registers for calculation
			alignas(ALIGNMENT_SIZE) __m256 k;
			alignas(ALIGNMENT_SIZE) __m256 product;
			alignas(ALIGNMENT_SIZE) __m256 accumulator = _mm256_setzero_ps();

			for (int i = 0; i < paddedLength; i += 8) {
				s = _mm256_load_ps(signal + index + i);
				k = _mm256_load_ps(kernel + i);

	#ifdef USE_FMA
							accumulator = _mm256_fmadd_ps(s, k, accumulator);
	#else
				product = _mm256_mul_ps(s, k);
				accumulator = _mm256_add_ps(product, accumulator);
	#endif

			}

			output += sum8floats(accumulator);

	#elif defined(USE_SIMD)

			// vector processing of float types (doubles require separate specialisation)

			FloatType output = 0.0;
			int index = currentIndex & -4; // make multiple-of-four
			int phase = currentIndex & 3;
			FloatType* kernel = kernelphases[phase];

			alignas(ALIGNMENT_SIZE) __m128 s;	// SIMD Vector Registers for calculation
			alignas(ALIGNMENT_SIZE) __m128 k;
			alignas(ALIGNMENT_SIZE) __m128 product;
			alignas(ALIGNMENT_SIZE) __m128 accumulator = _mm_setzero_ps();

			for (int i = 0; i < paddedLength; i += 4) {
				s = _mm_load_ps(signal + index + i);
				k = _mm_load_ps(kernel + i);
				product = _mm_mul_ps(s, k);
				accumulator = _mm_add_ps(product, accumulator);
			}

			// http://stackoverflow.com/questions/6996764/fastest-way-to-do-horizontal-float-vector-sum-on-x86
			__m128 a   = _mm_shuffle_ps(
				accumulator,
				accumulator,                 // accumulator = [D     C     | B     A    ]
				_MM_SHUFFLE(2, 3, 0, 1));                  // [C     D     | A     B    ]
			__m128 b   = _mm_add_ps(accumulator, a);       // [D+C   C+D   | B+A   A+B  ]
			a          = _mm_movehl_ps(a, b);              // [C     D     | D+C   C+D  ]
			b          = _mm_add_ss(a, b);                 // [C     D     | D+C A+B+C+D]
			output    += _mm_cvtss_f32(b);                 // A+B+C+D

	#else
			// scalar processing of float or double types
			FloatType output = 0.0;
			int index = currentIndex;
			for (int i = 0; i < length; ++i) {
				output += signal[index] * kernelphases[0][i];
				index++;
			}

	#endif

			return static_cast<FloatType>(output);

		}

		FloatType lazyGet(int L) {	// Skips stuffed-zeros introduced by interpolation, by only calculating every Lth sample from lastPut
			FloatType output = 0.0;
			int offset = lastPut - currentIndex;
			if (offset < 0) { // Wrap condition
				offset += length;
			}

			for (int i = offset; i < length; i+=L) {
				output += signal[i + currentIndex] * kernelphases[0][i];
			}
			return output;
		}

	private:
		int length;
		int paddedLength{};

		FloatType* signal; // Double-length signal buffer, to facilitate fast emulation of a circular buffe
		int currentIndex;
		int lastPut;
		int numVecElements{};
		uintptr_t alignMask{};

		// Polyphase Filter Kernel table:

	#if defined(USE_AVX)
		FloatType* kernelphases[8]; // note:  will only use half of these if FloatType = double
	#elif defined(USE_SIMD)
		FloatType* kernelphases[4]; // note: will only use half of these if FloatType = double
	#else
		FloatType* kernelphases[1];
	#endif

		void calcPaddedLength()
		{

	#if defined(USE_AVX) || defined(USE_SIMD)
			numVecElements = ALIGNMENT_SIZE / sizeof(FloatType);
	#else
			numVecElements = 1; // Scalar mode
	#endif

			alignMask = static_cast<uintptr_t>(-numVecElements);
			paddedLength = (length & alignMask) + numVecElements;
		}

		void allocateBuffers()
		{
			signal = static_cast<FloatType*>(aligned_malloc((paddedLength + length) * sizeof(FloatType), ALIGNMENT_SIZE));
			for(int i = 0; i < numVecElements; i++) {
				kernelphases[i] = static_cast<FloatType*>(aligned_malloc(paddedLength * sizeof(FloatType), ALIGNMENT_SIZE));
			}
		}

		void clearBuffers()
		{
			memset(signal, 0.0, (paddedLength + length) * sizeof(FloatType));
			for(int i = 0; i < numVecElements; i++) {
				memset(kernelphases[i], 0.0, paddedLength * sizeof(FloatType));
			}
		}

		void copyBuffers(const FIRFilter& other)
		{
			memcpy(signal, other.signal, (paddedLength + length) * sizeof(FloatType));
			for(int i = 0; i < numVecElements; i++) {
				memcpy(kernelphases[i], other.kernelphases[i], paddedLength * sizeof(FloatType));
			}
		}

		void freeBuffers()
		{
			aligned_free(signal);
			for(int i = 0; i < numVecElements; i++) {
				aligned_free(kernelphases[i]);
			}
		}

        // assertAlignment() : asserts that all private data buffers are aligned on expected boundaries
        void assertAlignment()
		{
	#ifdef COMPILING_ON_ANDROID
			// TODO: support 32-byte alignment for android?
	#warning  32-byte alignment is not yet supported when compiling for android
	#else
			const std::uintptr_t alignment = ALIGNMENT_SIZE;
            assert(reinterpret_cast<std::uintptr_t>(signal) % alignment == 0);
			for(int i = 0; i < numVecElements; i++) {
                assert(reinterpret_cast<std::uintptr_t>(kernelphases[i]) % alignment == 0);
			}
	#endif
		}

	#if defined(USE_AVX)
		// Horizontal add function (sums 8 floats into single float) http://stackoverflow.com/questions/23189488/horizontal-sum-of-32-bit-floats-in-256-bit-avx-vector
	static inline float sum8floats(__m256 x) {
		const __m128 x128 = _mm_add_ps(
			_mm256_extractf128_ps(x, 1),
			_mm256_castps256_ps128(x));																// ( x3+x7, x2+x6, x1+x5, x0+x4 )
		const __m128 x64 = _mm_add_ps(x128, _mm_movehl_ps(x128, x128));								// ( -, -, x1+x3+x5+x7, x0+x2+x4+x6 )
		const __m128 x32 = _mm_add_ss(x64, _mm_shuffle_ps(x64, x64, 0x55));							// ( -, -, -, x0+x1+x2+x3+x4+x5+x6+x7 )
		return _mm_cvtss_f32(x32);
	}

	// Horizontal add function (sums 4 doubles into single double)
	static inline double sum4doubles(__m256d x) {
		const __m128d x128 = _mm_add_pd(
			_mm256_extractf128_pd(x, 1),
			_mm256_castpd256_pd128(x));
		const __m128d x64 = _mm_add_pd(_mm_permute_pd(x128, 1), x128);
		return _mm_cvtsd_f64(x64);
	}

	#endif // defined(USE_AVX)

	};

	// Specializations for doubles:

	#if defined(USE_AVX)

	template <>
    inline double FIRFilter<double>::get() {

		// AVX implementation: Processes four doubles at a time.

		double output = 0.0;
		int index = currentIndex & -4; // make multiple-of-four
		int phase = currentIndex & 3;
		double* kernel = kernelphases[phase];

		alignas(ALIGNMENT_SIZE) __m256d s;	// AVX Vector Registers for calculation
		alignas(ALIGNMENT_SIZE) __m256d k;
		alignas(ALIGNMENT_SIZE) __m256d product;
		alignas(ALIGNMENT_SIZE) __m256d accumulator = _mm256_setzero_pd();

		for (int i = 0; i < paddedLength; i += 4) {
			s = _mm256_load_pd(signal + index);
			k = _mm256_load_pd(kernel + i);

	#ifdef USE_FMA
					accumulator = _mm256_fmadd_pd(s, k, accumulator);
	#else
			product = _mm256_mul_pd(s, k);
			accumulator = _mm256_add_pd(product, accumulator);
	#endif

			index += 4;
		}

		output += sum4doubles(accumulator);
		return output;
	}

	#elif defined(USE_SIMD) && defined(USE_SIMD_FOR_DOUBLES) && !defined(FIR_QUAD_PRECISION)

	template <>
	inline double FIRFilter<double>::get() {

		// SSE Implementation: Processes two doubles at a time.

		double output = 0.0;
		double* kernel;
		int index = currentIndex & -2; // make multiple-of-two
		int phase = currentIndex & 1;
		kernel = kernelphases[phase];

		alignas(ALIGNMENT_SIZE) __m128d s;	// SIMD Vector Registers for calculation
		alignas(ALIGNMENT_SIZE) __m128d k;
		alignas(ALIGNMENT_SIZE) __m128d product;
		alignas(ALIGNMENT_SIZE) __m128d accumulator = _mm_setzero_pd();

		for (int i = 0; i < paddedLength; i += 2) {
			s = _mm_load_pd(signal + index + i);
			k = _mm_load_pd(kernel + i);
			product = _mm_mul_pd(s, k);
			accumulator = _mm_add_pd(product, accumulator);
		}

		// horizontal add of two doubles
		__m128 undef  = _mm_undefined_ps();
		__m128 shuftmp= _mm_movehl_ps(undef, _mm_castpd_ps(accumulator));
		__m128d shuf  = _mm_castps_pd(shuftmp);
		output +=  _mm_cvtsd_f64(_mm_add_sd(accumulator, shuf));

		return output;
	}

	#endif // double specialisation


	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// -- Functions beyond this point are for manipulating filter taps, and not for actually performing filtering -- //
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////


	// makeLPF() : generate low pass filter coefficients, using sinc function
	template<typename FloatType> bool makeLPF(FloatType* filter, int Length, FloatType transitionFreq, FloatType sampleRate)
	{
	#ifdef FIR_QUAD_PRECISION

		// use quads internally, regardless of FloatType
		__float128 ft = transitionFreq / sampleRate; // normalised transition frequency
        // assert(ft < 0.5Q);
		int halfLength = Length / 2;
		__float128 halfM = 0.5Q * (Length - 1);
		__float128 M_TWOPIq = 2.0Q * M_PIq;

		if (Length & 1)
			filter[halfLength] = 2.0Q * ft; // if length is odd, avoid divide-by-zero at centre-tap

		for (int n = 0; n<halfLength; ++n) {
			__float128 sinc = sinq(fmodq(M_TWOPIq * ft * (n - halfM), M_TWOPIq)) / (M_PIq * (n - halfM));	// sinc function
			filter[Length - n - 1] = filter[n] = sinc;	// exploit symmetry
		}

	#else

		// use doubles internally, regardless of FloatType
		double ft = transitionFreq / sampleRate; // normalised transition frequency
        // assert(ft < 0.5);
		int halfLength = Length / 2;
		double halfM = 0.5 * (Length - 1);
		double M_TWOPI = 2.0 * M_PI;

		if (Length & 1)
			filter[halfLength] = 2.0 * ft; // if length is odd, avoid divide-by-zero at centre-tap

		for (int n = 0; n < halfLength; ++n) {
			// sinc function
			double sinc = sin(fmod(M_TWOPI * ft * (n - halfM), M_TWOPI)) / (M_PI * (n - halfM));
			filter[Length - n - 1] = filter[n] = sinc;	// exploit symmetry
		}
	#endif

		return true;
	}

	// This function converts a requested sidelobe height (in dB) to a value for the Beta parameter used in a Kaiser window:
	template<typename FloatType> FloatType calcKaiserBeta(FloatType dB)
    {
		if(dB < 21.0)
		{
			return 0;
		}
		if ((dB >= 21.0) && (dB <= 50.0)) {
			return 0.5842 * pow((dB - 21), 0.4) + 0.07886 * (dB - 21);
		}
		if (dB > 50.0) {
			return 0.1102 * (dB - 8.7);
		}

		return 0;
	}

	// I0() : 0th-order Modified Bessel function of the first kind:
	inline double I0(double z)
	{
		double result = 0.0;
		for (int k = 0; k < 34; ++k) {
			double kfact = factorial[k];
			double x = pow(z * z / 4.0, k) / (kfact * kfact);
			result += x;
		}
		return result;
	}

	#ifdef FIR_QUAD_PRECISION
	inline __float128 I0q(__float128 x)
	{
		__float128 result = 0.0Q;
		__float128 kfact = 1.0Q;
		__float128 xx_4 = x * x / 4.0Q;
		for (int k = 0; k < 60; ++k){
			result += powq(xx_4, k) / factorialSquaredq[k];
		}
		return result;
	}
	#endif

	// applyKaiserWindow() - This function applies a Kaiser Window to an array of filter coefficients ("textbook" version):
	template<typename FloatType> bool applyKaiserWindow(FloatType* filter, int Length, double Beta)
	{
		// Note: sometimes, the Kaiser Window formula is defined in terms of Alpha (instead of Beta),
		// in which case, Alpha def= Beta / pi

		if (Length < 1)
			return false;

	#ifdef FIR_QUAD_PRECISION

		for (int n = 0; n < Length; ++n) {
			filter[n] *= I0q(Beta * sqrtq(1.0Q - powq((2.0Q * n / (Length - 1) - 1), 2.0Q)))
						 / I0q(Beta);
		}

	#else

		for (int n = 0; n < Length; ++n) {
			filter[n] *= I0(Beta * sqrt(1.0 - pow((2.0 * n / (Length - 1) - 1), 2.0)))
				/ I0(Beta);
		}

	#endif

		return true;
	}

	// applyKaiserWindow2() - applies a Kaiser Window to an array of filter coefficients (alternative formula):
	template<typename FloatType> bool applyKaiserWindow2(FloatType* filter, int Length, double Beta)
	 {
		 double A;	// use double internally, regardless of FloatType (speed not an issue here; no reason not to)
		 double maxA = 0; // for diagnostics
		 for (int n = 0; n < Length; ++n) {

			 // simplified Kaiser Window Equation:
			 A = (2.0 * Beta / Length) * sqrt(n*(Length - n - 1));
			 maxA = std::max(maxA, A);
			 filter[n] *= I0(A) / I0(Beta);
		 }

		return true;
	}

    inline std::vector<double> makeHilbert(int length)
    {
        std::vector<double> coeffs;
        int length_ = std::max(length, 3) | 1;
        coeffs.resize(length_);
        int c = length_ / 2;
        coeffs[c] = 0.0;
        for(int n = 0; n < c; n++) {
            double s = std::sin((n - c) * M_PI / 2.0);
            double s2 = 2.0 * s * s;
            coeffs[n] = s2 / (M_PI * (n - c));
            coeffs[length_ - n - 1] = s2 / (M_PI * (c - n));
        }

        return coeffs;
    }

	// the following is a set of Complex-In, Complex-Out transforms used for constructing a minimum-Phase FIR:

	// logV() : logarithm of a vector of Complex doubles
	inline std::vector<std::complex<double>>
	logV(const std::vector<std::complex<double>>& input) {
		std::vector<std::complex<double>> output(input.size(), 0);
		std::transform(input.begin(), input.end(), output.begin(),
			[](std::complex<double> x) -> std::complex<double> {return std::log(x); });
		return output;
	}

	// limitDynRangeV() : set a limit (-dB) on how quiet signal is allowed to be below the peak.
	// Guaranteed to never return zero.
	inline std::vector<std::complex<double>>
	limitDynRangeV(const std::vector<std::complex<double>>& input, double dynRangeDB) {
		double dynRangeLinear = pow(10, std::abs(dynRangeDB) / 20.0); // will give same result for positive or negative dB values.

		// find peak:
		double peak=0.0;
		for (auto &c : input) {
			peak = std::max(peak, std::abs(c));
		}

		// determine low threshold
		double lowThresh = peak / dynRangeLinear;	// a level which is dynRangeDB dB below peak
		std::complex<double> lastX = lowThresh;		// variable for storing last output value

		std::vector<std::complex<double>> output(input.size(), 0);

		std::transform(input.begin(), input.end(), output.begin(),
			[lowThresh, &lastX](std::complex<double> x) -> std::complex<double> {

			double level = std::abs(x);
			if (level < lowThresh) {
				if (level == 0.0) {		// when input is zero, we must somehow make the modulus of the output equal to lowThresh
					x = lastX;			// sticky output; use last output value instead of zero
				}
				else {
					x = (x / level) * lowThresh; // scale x such that |x| == lowThresh
					lastX = x;
				}
			}
			return x; } // ends lambda
		); // ends call to std::transform()

		return output;
	}

	// realV() : real parts of a vector of Complex doubles
	inline std::vector<std::complex<double>>
	realV(const std::vector<std::complex<double>>& input) {
		std::vector<std::complex<double>> output(input.size(), 0);
		std::transform(input.begin(), input.end(), output.begin(),
			[](std::complex<double> x) -> std::complex<double> {return x.real(); });
		return output;
	}

	// imagV() : imaginary parts of a vector of Complex doubles (answer placed in imaginary part of output):
	inline std::vector<std::complex<double>>
	imagV(const std::vector<std::complex<double>>& input) {
		std::vector<std::complex<double>> output(input.size(), 0);
		std::transform(input.begin(), input.end(), output.begin(),
			[](std::complex<double> x) -> std::complex<double> {return{ 0,x.imag() }; });
		return output;
	}

	// expV() : exp of a vector of Complex doubles
	inline std::vector<std::complex<double>>
	expV(const std::vector<std::complex<double>>& input) {
		std::vector<std::complex<double>> output(input.size(), 0);
		std::transform(input.begin(), input.end(), output.begin(),
			[](std::complex<double> x) -> std::complex<double> {return exp(x); });
		return output;
	}

	// fftV() : FFT of vector of Complex doubles
	inline std::vector<std::complex<double>>
	fftV(std::vector<std::complex<double>> input) {

		std::vector<std::complex<double>> output(input.size(), 0); // output vector

		// create, execute, destroy plan:
		fftw_plan p = fftw_plan_dft_1d(static_cast<int>(input.size()),
			reinterpret_cast<fftw_complex*>(&input[0]),
			reinterpret_cast<fftw_complex*>(&output[0]),
			FFTW_FORWARD,
			FFTW_ESTIMATE);

		fftw_execute(p);
		fftw_destroy_plan(p);

		return output;
	}

	// ifftV() : Inverse FFT of vector of Complex doubles
	inline std::vector<std::complex<double>>
	ifftV(std::vector<std::complex<double>> input) {

		std::vector<std::complex<double>> output(input.size(), 0); // output vector

		// create, execute, destroy plan:
		fftw_plan p = fftw_plan_dft_1d(static_cast<int>(input.size()),
			reinterpret_cast<fftw_complex*>(&input[0]),
			reinterpret_cast<fftw_complex*>(&output[0]),
			FFTW_BACKWARD,
			FFTW_ESTIMATE);

		fftw_execute(p);
		fftw_destroy_plan(p);

		// scale output:
		double reciprocalSize = 1.0 / input.size();
		for (auto &c : output){
			c *= reciprocalSize;
		}

		return output;
	}

	// AnalyticSignalV() : Analytic signal of vector of Complex doubles
	// (Note: This function is referred to as "hilbert()" in Matlab / Octave, but it is not exactly a hilbert transform.
	// The hilbert Transform is placed in the imaginary part, and the original input is in the real part.)
	// See Footnote* below for more information on algorithm ...

	inline std::vector<std::complex<double>>
	AnalyticSignalV(const std::vector<std::complex<double>>& input) {

		std::vector<std::complex<double>> U = fftV(input);

		size_t N = input.size();
		size_t halfN = N / 2;

		// Note: U[0], U[halfN] unchanged:
		for (size_t n = 1; n < N; ++n) {
			if (n > halfN)
				U[n] = 0;
			if (n < halfN)
				U[n] *= 2.0;
		}

		std::vector<std::complex<double>> output = ifftV(U);
		return output;
	}

	// makeMinPhase() : transform linear-phase FIR filter coefficients into minimum-phase (in-place version)
	template<typename FloatType>
	void makeMinPhase(FloatType* pFIRcoeffs, size_t length)
	{
		auto fftLength = static_cast<size_t>(pow(2, 2.0 + ceil(log2(length)))); // use FFT 4x larger than (length rounded-up to power-of-2)

		std::vector <std::complex<double>> complexInput;
		std::vector <std::complex<double>> complexOutput;

		// Pad zeros on either side of FIR:

		size_t frontPaddingLength = (fftLength - length) / 2;
		size_t backPaddingLength = fftLength - frontPaddingLength - length;

		for (size_t n = 0; n < frontPaddingLength; ++n) {
			complexInput.emplace_back(0, 0);
		}

		for (size_t n = 0; n < length; ++n) {
			complexInput.push_back({ pFIRcoeffs[n], 0 });
		}

		for (size_t n = 0; n < backPaddingLength; ++n) {
			complexInput.emplace_back(0, 0);
		}

		assert(complexInput.size() == fftLength); // make sure padding worked properly.

		// Formula is as follows:

		// take the reversed array of
		// the real parts of
		// the ifft of
		// e to the power of
		// the Analytic Signal of
		// the real parts of
		// the log of
		// the dynamic-ranged limited version of
		// the fft of
		// the original filter

		complexOutput = realV(ifftV(expV(AnalyticSignalV(realV(logV(limitDynRangeV(fftV(complexInput),-190)))))));
		std::reverse(complexOutput.begin(), complexOutput.end());

		// write all the real parts back to coeff array:
		size_t n = 0;
		for (auto &c : complexOutput) {
			if (n < length)
				pFIRcoeffs[n] = c.real();
			else
				break;
			++n;
		}
	}

	///////////////////////////////////////////////////////////////////////
	// utility functions:

	// dumpKaiserWindow() - utility function for displaying Kaiser Window:
	inline void dumpKaiserWindow(size_t length, double Beta) {
		std::vector<double> f(length, 1);
		applyKaiserWindow<double>(f.data(), static_cast<int>(length), Beta);
		for (size_t i = 0; i < length; ++i) {
			std::cout << i << ": " << f[i] << std::endl;
		}

		std::vector<double> g(length, 1);
		applyKaiserWindow<double>(g.data(), static_cast<int>(length), Beta);
		for (size_t i = 0; i < length; ++i) {
			std::cout << i << ": " << g[i] << std::endl;
		}
	}

	// asserts that the two Kaiser Window formulas agree with each other (within a specified tolerance)
	inline void assertKaiserWindow(size_t length, double Beta) {

		const double tolerance = 0.001;
		const double upper = 1.0 + tolerance;
		const double lower = 1.0 - tolerance;

		std::vector<double> f(length, 1);
		applyKaiserWindow2<double>(f.data(), static_cast<int>(length), Beta);

		std::vector<double> g(length, 1);
		applyKaiserWindow<double>(g.data(), static_cast<int>(length), Beta);

		for (size_t i = 0; i < length; ++i) {
			double ratio = f[i] / g[i];
			assert(ratio < upper && ratio > lower);
		}
	}

	// dumpFilter() - utility function for displaying filter coefficients:
	template<typename FloatType> void dumpFilter(const FloatType* Filter, int Length) {
		for (int i = 0; i < Length; ++i) {
			std::cout << Filter[i] << std::endl;
		}
	}

	inline void dumpComplexVector(const std::vector<std::complex<double>>& v)
	{
		for (auto &c : v) {
			std::cout << c.real() << "+" << c.imag() << "i" << std::endl;
		}
	}

	template<typename FloatType>
	void dumpFFT(FloatType* data, size_t length)
	{
		auto pow2length = static_cast<size_t>(pow(2, 1.0 + floor(log2(length))));

		std::vector <std::complex<double>> complexInput;
		std::vector <std::complex<double>> complexOutput;

		for (int n = 0; n < pow2length; ++n) {
			if (n<length)
				complexInput.push_back({ data[n], 0 });
			else
				complexInput.emplace_back(0, 0); // pad remainder with zeros (to-do: does it mattter where the zeros are put ?)
		}

		complexOutput = fftV(complexInput);

		std::setprecision(17);
		std::cout << "real,imag,mag,phase" << std::endl;
		for (auto &c : complexOutput) {
			std::cout << c.real() << "," << c.imag() << "," << std::abs(c) << "," << arg(c) << std::endl;
		}
	}

	inline void testSinAccuracy() {

		const int numSteps = 10000000;
		const double inc = M_PI / numSteps;
		double t = M_PI / -2.0;
		double maxError = 0.0;
		double worstT = 0.0;

		for (int i = 0; i < numSteps; ++i ) {
			// calc relative error of
			// |(sin 2t - 2 * sint * cost) / sin 2t|
			// (double-angle identity)

			double e = std::abs((std::sin(2.0 * t) - 2.0 * std::sin(t) * std::cos(t)) / std::sin(2.0 * t));
			//double e = std::abs((sin(2.0 * t) - 2.0 * sin(t) * cos(t)) / sin(2.0 * t));
			if (e > maxError) {
				worstT = t;
				maxError = e;
			}
			t += inc;
		}
		std::cout << "maxError: " << std::setprecision(33) << maxError << std::endl;
		std::cout << "worstT: " << worstT << std::endl;
	}

} // namespace ReSampler

// *Marple, S. L. "Computing the Discrete-Time Analytic Signal via FFT." IEEE Transactions on Signal Processing. Vol. 47, 1999, pp. 2600�2603

#endif // FIRFFILTER_H_
