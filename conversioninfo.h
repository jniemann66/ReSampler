/*
* Copyright (C) 2016 - 2024 Judd Niemann - All Rights Reserved.
* You may use, distribute and modify this code under the
* terms of the GNU Lesser General Public License, version 2.1
*
* You should have received a copy of GNU Lesser General Public License v2.1
* with this file. If not, please refer to: https://github.com/jniemann66/ReSampler
*/

#ifndef CONVERSIONINFO_H
#define CONVERSIONINFO_H

// conversioninfo.h:
// defines the ConversionInfo struct,
// for holding conversion parameters.

#include <list>
#include <string>
#include "csv.h"
#include "iqdemodulator.h"

namespace ReSampler {

typedef enum {
	relaxed,
	normal,
	steep,
	custom
} LPFMode;

// struct ConversionInfo : structure for holding all the parameters required for a conversion job
struct ConversionInfo
{
	std::string inputFilename;
	std::string outputFilename;
	int inputSampleRate;
	int outputSampleRate;
	double gain;
	double limit;
	bool bUseDoublePrecision;
	bool bNormalize;
	double normalizeAmount;
	int outputFormat;
	std::string outBitFormat;
	bool bDither;
	double ditherAmount;
	int ditherProfileID;
	bool bAutoBlankingEnabled;
	bool bDelayTrim;
	bool bMinPhase;
	bool bSetFlacCompression;
	int flacCompressionLevel;
	bool bSetVorbisQuality;
	double vorbisQuality;
	bool disableClippingProtection;
	LPFMode lpfMode;
	double lpfCutoff;
	double lpfTransitionWidth;
	bool bUseSeed;
	int seed;
	bool dsfInput;
	bool dffInput;
	bool csvOutput;
	bool bEnablePeakDetection;
	bool bMultiThreaded;
	bool bRf64;
	bool bNoPeakChunk;
	bool bWriteMetaData;
	int maxStages;
	bool bSingleStage;
	bool bMultiStage;
	bool bShowStages;
	int progressUpdates;
	int overSamplingFactor;
	bool bBadParams;

	std::string appName;

#if defined (_WIN32) || defined (_WIN64)
	std::string tmpDir;
#endif

	bool bTmpFile;
	bool bShowTempFile;
	bool quantize;
	int quantizeBits;
	IntegerWriteScalingStyle integerWriteScalingStyle;
	bool bRawInput;
	int rawInputChannels;
	int rawInputSampleRate;
	std::string rawInputBitFormat;
	bool bDemodulateIQ;
	ModulationType IQModulationType;
    DeEmphasisType IQDeEmphasisType;
	bool bAdjustStereoWidth;
	double stereoWidth;
	bool bFadeIn;
    bool bFadeOut;
	double fadeInTime;
    double fadeOutTime;

	// functions
	bool fromCmdLineArgs(int argc, char **argv); // populate ConversionInfo from args
	std::string toCmdLineArgs(); // format ConversionInfo into a space-separated string of args
};

std::string sanitize(const std::string& str);
template<typename T>
bool getCmdlineParam(char** begin, char** end, const std::string& option, T& parameter); // fetch a numeric parameter of type T
bool getCmdlineParam(char** begin, char** end, const std::string& option, std::string& parameter); // fetch a string parameter
bool getCmdlineParam(char** begin, char** end, const std::string& option, std::vector<std::string>& parameters); // fetch a vector of strings
bool getCmdlineParam(char** begin, char** end, const std::string& option); // detect presence of command-line switch only
int getDefaultNoiseShape(int sampleRate);

static_assert(std::is_copy_constructible<ConversionInfo>::value,
	"ConversionInfo must be copy constructible");

static_assert(std::is_copy_assignable<ConversionInfo>::value,
	"ConversionInfo must be copy assignable");

} // namespace ReSampler

#endif // CONVERSIONINFO_H


