/*
* Copyright (C) 2016 - 2023 Judd Niemann - All Rights Reserved.
* You may use, distribute and modify this code under the
* terms of the GNU Lesser General Public License, version 2.1
*
* You should have received a copy of GNU Lesser General Public License v2.1
* with this file. If not, please refer to: https://github.com/jniemann66/ReSampler
*/

#ifndef RESAMPLER_H
#define RESAMPLER_H 1

#ifdef __APPLE__
#include <unistd.h>
#include <libproc.h>
#endif

#if defined (__MINGW64__) || defined (__MINGW32__) || defined (__GNUC__)
#ifdef USE_QUADMATH
#include <quadmath.h>
#endif
#endif

#define _USE_MATH_DEFINES
#include <cmath>

#include "sndfile.h"
#include "sndfile.hh"
#include "conversioninfo.h"

#include <type_traits>
#include <vector>
#include <string>
#include <iostream>
#include <map>
#include <functional>

namespace ReSampler {

const std::string strVersion("2.1.0");
const std::string strUsage("usage: ReSampler -i <inputfile> [-o <outputfile>] -r <samplerate> [-b <bitformat>] [-n [<normalization factor>]]\n");
const std::string strExtraOptions(
		"--help\n"
		"--version\n"
		"--compiler\n"
		"--sndfile-version\n"
		"--listsubformats <ext>\n"
		"--showDitherProfiles\n"
		"--gain [<amount>]\n"
		"--doubleprecision\n"
		"--dither [<amount>] [--autoblank] [--ns [<ID>]] [--flat-tpdf] [--seed [<num>]] [--quantize-bits <number of bits>]\n"
		"--noDelayTrim\n"
		"--minphase\n"
		"--flacCompression <compressionlevel>\n"
		"--vorbisQuality <quality>\n"
		"--noClippingProtection\n"
		"--relaxedLPF\n"
		"--steepLPF\n"
		"--lpf-cutoff <percentage> [--lpf-transition <percentage>]\n"
		"--mt\n"
		"--rf64\n"
		"--noPeakChunk\n"
		"--noMetadata\n"
		"--singleStage\n"
		"--multiStage\n"
		"--maxStages\n"
		"--showStages\n"
		"--rawInput <samplerate> <bitformat> [numChannels]\n"
		"--progress-updates <0..100>\n"

		#if defined (_WIN32) || defined (_WIN64)
		"--tempDir <path>\n"
		#endif

		"--showTempFile\n"
		"--noTempFile\n"
        "--demodulateIQ [<AM|LSB|USB|NFM|WFM>]\n"
        "--stereoWidth <amount>\n"
		"--fade-in <time in seconds>\n"
		"--fade-out <time in seconds>\n"
		);

const double clippingTrim = 1.0 - (1.0 / (1 << 23));
const int maxClippingProtectionAttempts = 3;

#define BUFFERSIZE 32768 // buffer size for file reads

// map of commandline subformats to libsndfile subformats:
const std::map<std::string, int> subFormats = {
	{ "s8", SF_FORMAT_PCM_S8 },
	{ "u8", SF_FORMAT_PCM_U8 },
	{ "8", SF_FORMAT_PCM_U8 },	// signed or unsigned depends on major format of output file eg. wav files unsigned
	{ "16", SF_FORMAT_PCM_16 },
	{ "24", SF_FORMAT_PCM_24 },
	{ "32", SF_FORMAT_PCM_32 },
	{ "32f",SF_FORMAT_FLOAT },
	{ "64f", SF_FORMAT_DOUBLE },
	{ "ulaw", SF_FORMAT_ULAW },
	{ "alaw", SF_FORMAT_ALAW },
	{ "ima-adpcm", SF_FORMAT_IMA_ADPCM },
	{ "ms-adpcm", SF_FORMAT_MS_ADPCM },
	{ "gsm610", SF_FORMAT_GSM610 },
	{ "vox-adpcm", SF_FORMAT_VOX_ADPCM },
	{ "g721-32", SF_FORMAT_G721_32 },
	{ "g723-24", SF_FORMAT_G723_24 },
	{ "g723-40", SF_FORMAT_G723_40 },
	{ "dwvw12", SF_FORMAT_DWVW_12 },
	{ "dwvw16", SF_FORMAT_DWVW_16 },
	{ "dwvw24", SF_FORMAT_DWVW_24 },
	{ "dwvwn", SF_FORMAT_DWVW_N },
	{ "dpcm8", SF_FORMAT_DPCM_8 },
	{ "dpcm16", SF_FORMAT_DPCM_16 },
	{ "vorbis", SF_FORMAT_VORBIS },
	{ "alac16", SF_FORMAT_ALAC_16 },
	{ "alac20", SF_FORMAT_ALAC_20 },
	{ "alac24", SF_FORMAT_ALAC_24 },
	{ "alac32", SF_FORMAT_ALAC_32 },
	{ "layer1", SF_FORMAT_MPEG_LAYER_I },
	{ "layer2", SF_FORMAT_MPEG_LAYER_II },
	{ "layer3", SF_FORMAT_MPEG_LAYER_III }
};

// map of default (ie sensible) subformats (usually 16-bit PCM)
const std::map<std::string, std::string> defaultSubFormats = {
	{"aiff", "16"},
	{"au", "16"},
	{"avr", "16"},
	{"caf", "16"},
	{"flac", "16"},
	{"htk", "16"},
	{"iff", "16"},
	{"mat", "16"},
	{"mpc", "16"},
	{"oga", "vorbis"},
	{"paf", "16"},
	{"pvf", "16"},
	{"raw", "16"},
	{"rf64", "16"},
	{"sd2", "16"},
	{"sds", "16"},
	{"sf", "16"},
	{"voc", "16"},
	{"w64", "16"},
	{"wav", "16"},
	{"wve", "alaw"},
	{"xi", "dpcm16"},
	{"mp3", "layer3"}
};

#define MAX_CART_TAG_TEXT_SIZE 16384
typedef SF_CART_INFO_VAR(MAX_CART_TAG_TEXT_SIZE) LargeSFCartInfo;

struct MetaData
{
	std::string title;
	std::string copyright;
	std::string software;
	std::string artist;
	std::string comment;
	std::string date;
	std::string album;
	std::string license;
	std::string trackNumber;
	std::string genre;

	// The following is only relevant for bext chunks in Broadcast Wave files:
	bool has_bext_fields{};
	SF_BROADCAST_INFO broadcastInfo{};

	// The following is only relevant for cart chunks:
	bool has_cart_chunk{};
	LargeSFCartInfo cartInfo{};

};

class OutputManager {
	static std::function<void(int)> progressFunc;

public:
	static std::function<void (int)> getProgressFunc();
	static void setProgressFunc(const std::function<void (int)> &value);
	static void callProgressFunc(int percentComplete);
};

bool getMajorFormatFromFileExt(SF_FORMAT_INFO *info, const std::string& ext);
bool checkSSE2();
bool checkAVX();
bool showBuildVersion();
bool parseGlobalOptions(int argc, char * argv[]);
bool determineBestBitFormat(std::string& bitFormat, const ConversionInfo& ci);
int determineOutputFormat(const std::string & outFileExt, const std::string & bitFormat);
void listSubFormats(const std::string & f);
// explicit instantiations - generate all required flavors of convert()
bool convert_DffFile_Float(ConversionInfo & ci);
bool convert_DffFile_Double(ConversionInfo & ci);
bool convert_DsfFile_Float(ConversionInfo & ci);
bool convert_DsfFile_Double(ConversionInfo & ci);
bool convert_SndfileHandle_Float(ConversionInfo & ci);
bool convert_SndfileHandle_Double(ConversionInfo & ci);
bool convert_IQFile_Float(ConversionInfo & ci);
bool convert_IQFile_Double(ConversionInfo & ci);
template<typename FileReader, typename FloatType> bool convert(ConversionInfo & ci);
template<typename FloatType>
SndfileHandle* getTempFile(int inputFileFormat, int nChannels, const ConversionInfo& ci, std::string& tmpFilename);
void showDitherProfiles();
int getSfBytesPerSample(int format);
bool checkWarnOutputSize(sf_count_t inputSamples, int bytesPerSample, int numerator, int denominator);
template<typename IntType> std::string fmtNumberWithCommas(IntType n);
void printSamplePosAsTime(sf_count_t samplePos, unsigned int sampleRate);

void generateExpSweep(const std::string & filename,
					  int sampleRate = 96000, // samplerate of generated file
					  int format = SF_FORMAT_WAV | SF_FORMAT_FLOAT, // format of generated file
					  double duration = 10.0, // approximate duration in seconds
					  int octaves = 12, // number of octaves below Nyquist for lowest frequency
					  double amplitude_dB = -3.0 // amplitude in dB relative to FS
		);

bool getMetaData(MetaData& metadata, SndfileHandle &infile);
bool setMetaData(const MetaData& metadata, SndfileHandle& outfile);
void showCompiler();
int runCommand(int argc, char** argv);

template <typename InputIterator>
int runCommand(InputIterator first, InputIterator last)
{
	std::vector<const char*> argv;
	for (auto it = first; it != last; ++it) {
		argv.push_back(it->c_str());
	}

	return runCommand(argv.size(), const_cast<char**>(argv.data()));
}

template <typename ContainerType>
int runCommand(const ContainerType& args)
{
	return runCommand(args.cbegin(), args.cend());
}

} // namespace ReSampler

#endif // RESAMPLER_H
