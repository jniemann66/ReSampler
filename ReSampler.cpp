/*
* Copyright (C) 2016 - 2020 Judd Niemann - All Rights Reserved.
* You may use, distribute and modify this code under the
* terms of the GNU Lesser General Public License, version 2.1
*
* You should have received a copy of GNU Lesser General Public License v2.1
* with this file. If not, please refer to: https://github.com/jniemann66/ReSampler
*/

// ReSampler.cpp : Audio Sample Rate Converter by Judd Niemann.

#include "ReSampler.h"
#include "csv.h" // to-do: check macOS
#include "ctpl/ctpl_stl.h"
#include "raiitimer.h"
#include "fraction.h"
#include "srconvert.h"
#include "ditherer.h"
#include "iqdemodulator.h"
#include "mpxdecode.h"
#include "stereoimager.h"
#include "fadeeffect.h"
#include "effectchain.h"

#include <cstdio>
#include <string>
#include <iostream>
#include <vector>
#include <iomanip>
#include <regex>

////////////////////////////////////////////////////////////////////////////////////////
// This program uses the following libraries:
// 1:
// libsndfile
// available at http://www.mega-nerd.com/libsndfile/
//
// (copy of entire package included in $(ProjectDir)\libsbdfile)
//
// 2:
// fftw
// http://www.fftw.org/
//
//                                                                                    //
////////////////////////////////////////////////////////////////////////////////////////

namespace ReSampler {

// parseGlobalOptions() - result indicates whether to terminate.
bool parseGlobalOptions(int argc, char * argv[]) {

	// help switch:
	if (getCmdlineParam(argv, argv + argc, "--help") || getCmdlineParam(argv, argv + argc, "-h")) {
		std::cout << strUsage << std::endl;
		std::cout << "Additional options:\n\n" << strExtraOptions << std::endl;
		return true;
	}

	// version switch:
	if (getCmdlineParam(argv, argv + argc, "--version")) {
		std::cout << strVersion << std::endl;
		return true;
	}

	// compiler switch:
	if (getCmdlineParam(argv, argv + argc, "--compiler")) {
		showCompiler();
		return true;
	}

	// sndfile-version switch:
	if (getCmdlineParam(argv, argv + argc, "--sndfile-version")) {
		char s[128];
		sf_command(nullptr, SFC_GET_LIB_VERSION, s, sizeof(s));
		std::cout << s << std::endl;
		return true;
	}

	// listsubformats
	if (getCmdlineParam(argv, argv + argc, "--listsubformats")) {
		std::string filetype;
		getCmdlineParam(argv, argv + argc, "--listsubformats", filetype);
		listSubFormats(filetype);
		return true;
	}

	// showDitherProfiles
	if (getCmdlineParam(argv, argv + argc, "--showDitherProfiles")) {
		showDitherProfiles();
		return true;
	}

	// generate
	if (getCmdlineParam(argv, argv + argc, "--generate")) {
		std::string filename;
		getCmdlineParam(argv, argv + argc, "--generate", filename);
		generateExpSweep(filename);
		return true;
	}

	return false;
}

// determineBestBitFormat() : determines the most appropriate bit format for the output file, through the following process:
// 1. Try to use infile's format and if that isn't valid for outfile, then:
// 2. use the default subformat for outfile.
// store best bit format as a string in BitFormat

bool determineBestBitFormat(std::string& bitFormat, const ConversionInfo& ci)
{
	// get infile's extension from filename:
	std::string inFileExt;
	if (ci.inputFilename.find_last_of('.') != std::string::npos) {
		inFileExt = ci.inputFilename.substr(ci.inputFilename.find_last_of('.') + 1);
	}

	bool dsfInput = false;
	bool dffInput = false;

	int inFileFormat = 0;

	if (inFileExt == "dsf") {
		dsfInput = true;
	}
	else if (inFileExt == "dff") {
		dffInput = true;
	}

	else { // libsndfile-openable file

		if (ci.bRawInput)
		{
			inFileFormat = SF_FORMAT_RAW | subFormats.at(ci.rawInputBitFormat);
		}
		else
		{
			// Inspect input file for format:
			SndfileHandle infile(ci.inputFilename, SFM_READ);
			inFileFormat = infile.format();

			if (int e = infile.error())
			{
				std::cout << "Couldn't Open Input File (" << sf_error_number(e) << ")" << std::endl;
				return false;
			}
		}

		// get BitFormat of inFile as a string:
		for (auto& subformat : subFormats) {
			if (subformat.second == (inFileFormat & SF_FORMAT_SUBMASK)) {
				bitFormat = subformat.first;
				break;
			}
		}

		// retrieve infile's TRUE extension (from the file contents), and if retrieval is successful, override extension derived from filename:
		SF_FORMAT_INFO infileFormatInfo;
		infileFormatInfo.format = inFileFormat & SF_FORMAT_TYPEMASK;
		if (sf_command(nullptr, SFC_GET_FORMAT_INFO, &infileFormatInfo, sizeof(infileFormatInfo)) == 0) {
			inFileExt = std::string(infileFormatInfo.extension);
		}
	}

	// get outfile's extension:
	std::string outFileExt;
	if (ci.outputFilename.find_last_of('.') != std::string::npos)
		outFileExt = ci.outputFilename.substr(ci.outputFilename.find_last_of('.') + 1);

	// when the input file is dsf/dff, use default output subformat:
	if (dsfInput || dffInput) { // choose default output subformat for chosen output file format
		bitFormat = defaultSubFormats.find(outFileExt)->second;
		std::cout << "defaulting to " << bitFormat << std::endl;
		return true;
	}

	// get total number of major formats:
	SF_FORMAT_INFO formatinfo;
	int format;
	int major_count;
	memset(&formatinfo, 0, sizeof(formatinfo));
	sf_command(nullptr, SFC_GET_FORMAT_MAJOR_COUNT, &major_count, sizeof(int));

	// determine if inFile's subformat is valid for outFile:
	for (int m = 0; m < major_count; m++)
	{
		formatinfo.format = m;
		sf_command(nullptr, SFC_GET_FORMAT_MAJOR, &formatinfo, sizeof(formatinfo));

		if (stricmp(formatinfo.extension, outFileExt.c_str()) == 0) { // match between format number m and outfile's file extension
			format = formatinfo.format | (inFileFormat & SF_FORMAT_SUBMASK); // combine outfile's major format with infile's subformat

			// Check if format / subformat combination is valid:
			SF_INFO sfinfo;
			memset(&sfinfo, 0, sizeof(sfinfo));
			sfinfo.channels = 1;
			sfinfo.format = format;

			if (sf_format_check(&sfinfo)) { // Match: infile's subformat is valid for outfile's format
				break;
			}

			// infile's subformat is not valid for outfile's format; use outfile's default subformat
			std::cout << "Output file format " << outFileExt << " and subformat " << bitFormat << " combination not valid ... ";
			bitFormat.clear();
			bitFormat = defaultSubFormats.find(outFileExt)->second;
			std::cout << "defaulting to " << bitFormat << std::endl;
			break;

		}
	}
	return true;
}

// determineOutputFormat() : returns an integer representing a libsndfile output format:
int determineOutputFormat(const std::string& outFileExt, const std::string& bitFormat)
{
	SF_FORMAT_INFO info;
	int format = 0;
	int major_count;
	memset(&info, 0, sizeof(info));
	sf_command(nullptr, SFC_GET_FORMAT_MAJOR_COUNT, &major_count, sizeof(int));
	bool bFileExtFound = false;

	// Loop through all major formats to find match for outFileExt:
	for (int m = 0; m < major_count; ++m) {
		info.format = m;
		sf_command(nullptr, SFC_GET_FORMAT_MAJOR, &info, sizeof(info));
		if (stricmp(info.extension, outFileExt.c_str()) == 0) {
			bFileExtFound = true;
			break;
		}
	}

	if (bFileExtFound) {
		// Check if subformat is recognized:
		auto sf = subFormats.find(bitFormat);
		if (sf != subFormats.end()) {
			format = info.format | sf->second;
		} else {
			std::cout << "Warning: bit format " << bitFormat << " not recognised !" << std::endl;
		}
	}

	// Special cases:
	if (bitFormat == "8") {
		// user specified 8-bit. Determine whether it must be unsigned or signed, based on major type:
		// These formats always use unsigned: 8-bit when they use 8-bit: mat rf64 voc w64 wav

		if ((outFileExt == "mat") || (outFileExt == "rf64") || (outFileExt == "voc") || (outFileExt == "w64") || (outFileExt == "wav"))
			format = info.format | SF_FORMAT_PCM_U8;
		else
			format = info.format | SF_FORMAT_PCM_S8;
	}

	return format;
}

// listSubFormats() - lists all valid subformats for a given file extension (without "." or "*."):
void listSubFormats(const std::string& f)
{
	SF_FORMAT_INFO	info;
	int major_count;
	memset(&info, 0, sizeof(info));
	sf_command(nullptr, SFC_GET_FORMAT_MAJOR_COUNT, &major_count, sizeof(int));
	bool bFileExtFound = false;

	// Loop through all major formats to find match for outFileExt:
	for (int m = 0; m < major_count; ++m) {
		info.format = m;
		sf_command(nullptr, SFC_GET_FORMAT_MAJOR, &info, sizeof(info));
		if (stricmp(info.extension, f.c_str()) == 0) {
			bFileExtFound = true;
			break;
		}
	}

	if (bFileExtFound) {
		SF_INFO sfinfo;
		memset(&sfinfo, 0, sizeof(sfinfo));
		sfinfo.channels = 1;

		// loop through all subformats and find which ones are valid for file type:
		for (auto& subformat : subFormats) {
			sfinfo.format = (info.format & SF_FORMAT_TYPEMASK) | subformat.second;
			if (sf_format_check(&sfinfo))
				std::cout << subformat.first << std::endl;
		}
	}
	else {
		std::cout << "File extension " << f << " unknown" << std::endl;
	}
}

// explicit instantiations - generate all required flavors of convert()
bool convert_DffFile_Float(ConversionInfo & ci) {
	return convert<DffFile, float>(ci);
}
bool convert_DffFile_Double(ConversionInfo & ci) {
	return convert<DffFile, double>(ci);
}
bool convert_DsfFile_Float(ConversionInfo & ci) {
	return convert<DsfFile, float>(ci);
}
bool convert_DsfFile_Double(ConversionInfo & ci) {
	return convert<DsfFile, double>(ci);
}
bool convert_SndfileHandle_Float(ConversionInfo & ci) {
	return convert<SndfileHandle, float>(ci);
}
bool convert_SndfileHandle_Double(ConversionInfo & ci) {
	return convert<SndfileHandle, double>(ci);
}
bool convert_IQFile_Float(ConversionInfo& ci) {
	return convert<IQFile, float>(ci);
}
bool convert_IQFile_Double(ConversionInfo& ci) {
	return convert<IQFile, double>(ci);
}

// convert()

/* Note: type 'FileReader' MUST implement the following methods:
constructor(const std::string& fileName)
constructor(const std::string& fileName, int infileMode, int infileFormat, int infileChannels, int infileRate)
bool error() // or int error()
unsigned int channels()
unsigned int samplerate()
uint64_t frames()
int format()
read(inbuffer, count)
seek(position, whence)
*/

template<typename FileReader, typename FloatType>
bool convert(ConversionInfo& ci)
{
	bool multiThreaded = ci.bMultiThreaded;

	// pointer for temp file;
	SndfileHandle* tmpSndfileHandle = nullptr;

	// filename for temp file;
	std::string tmpFilename;

	// Prepare input file opening parameters
	int infileMode;
	int infileFormat = 0; // leave these zero for normal operation. (Only set for RAW input)
	int infileChannels = 0;
	int infileRate = 0;
	if(ci.dsfInput) {
		infileMode = Dsf_read;
	} else if(ci.dffInput) {
		infileMode = Dff_read;
	} else {
		infileMode = SFM_READ;
		if(ci.bRawInput) {
			auto it = subFormats.find(ci.rawInputBitFormat);
			int infileSubFormat = (it != subFormats.end()) ? it->second : SF_FORMAT_PCM_16;
			infileFormat = SF_FORMAT_RAW | infileSubFormat;
			infileChannels = ci.rawInputChannels;
			infileRate = ci.rawInputSampleRate;
			std::cout << "raw input" << std::endl;
		}

		if(ci.bDemodulateIQ) {
            infileFormat |= ((ci.IQModulationType | ci.IQDeEmphasisType) << 8);
            ModulationType modulationType = ci.IQModulationType;
            if (modulationType == WFM_NO_LOWPASS) {
                modulationType = WFM;
            }
            // stuff the modulation type into the second-least-significant byte
            auto k = std::find_if(std::begin(modulationTypeMap), std::end(modulationTypeMap), [modulationType](const std::pair<std::string, ModulationType> pair){
                return pair.second == modulationType;
			});
			std::string s;
			if(k != std::end(modulationTypeMap)) {
				s = k->first;
			}
            std::cout << "IQ demodulation " << s << " ";
            if(ci.IQDeEmphasisType != NoDeEmphasis) {
                auto k1 = std::find_if(std::begin(deEmphasisTypeMap), std::end(deEmphasisTypeMap), [&](const std::pair<std::string, DeEmphasisType> pair){
                    return pair.second == ci.IQDeEmphasisType;
                });
                if(k1 != std::end(deEmphasisTypeMap)) {
                    std::cout << k1->first << "us de-emphasis";
                }
            }
            std::cout << std::endl;
		}
	}

	// Open input file
	FileReader infile(ci.inputFilename, infileMode, infileFormat, infileChannels, infileRate);

    int e = infile.error();
    if (e != SF_ERR_NO_ERROR) {
        if(e == ERROR_IQFILE_WFM_SAMPLERATE_TOO_LOW) {
            std::cout << "Sample Rate not high enough for WFM" << std::endl;
        } else if (e == ERROR_IQFILE_TWO_CHANNELS_EXPECTED) {
            std::cout << "2 channels expected for an I/Q input file !" << std::endl;
        } else {
            std::cout << "Error: Couldn't Open Input File (" << sf_error_number(e) << ")" << std::endl;
        }
		return false;
	}

	// read input file metadata:
	MetaData m;
	getMetaData(m, infile);

	// read input file properties:
	int nChannels = static_cast<int>(infile.channels());
	ci.inputSampleRate = infile.samplerate();
	sf_count_t inputFrames = infile.frames();
	sf_count_t inputSampleCount = inputFrames * nChannels;
	double inputDuration = 1000.0 * inputFrames / ci.inputSampleRate; // ms

	// determine conversion ratio:
	Fraction fraction = getFractionFromSamplerates(ci.inputSampleRate, ci.outputSampleRate);

	// set buffer sizes:
	auto inputChannelBufferSize = static_cast<size_t>(BUFFERSIZE);
	auto inputBlockSize = static_cast<size_t>(BUFFERSIZE * nChannels);
	auto outputChannelBufferSize = static_cast<size_t>(1 + std::ceil(BUFFERSIZE * static_cast<double>(fraction.numerator) / static_cast<double>(fraction.denominator)));
	auto outputBlockSize = static_cast<size_t>(nChannels * (1 + outputChannelBufferSize));

	// allocate buffers:
	std::vector<FloatType> inputBlock(inputBlockSize, 0);		// input buffer for storing interleaved samples from input file
	std::vector<FloatType> outputBlock(outputBlockSize, 0);		// output buffer for storing interleaved samples to be saved to output file
	std::vector<std::vector<FloatType>> inputChannelBuffers;	// input buffer for each channel to store deinterleaved samples
	std::vector<std::vector<FloatType>> outputChannelBuffers;	// output buffer for each channel to store converted deinterleaved samples
	for (int n = 0; n < nChannels; n++) {
		inputChannelBuffers.emplace_back(std::vector<FloatType>(inputChannelBufferSize, 0));
		outputChannelBuffers.emplace_back(std::vector<FloatType>(outputChannelBufferSize, 0));
	}

	int inputFileFormat = infile.format();
	if (inputFileFormat != DFF_FORMAT && inputFileFormat != DSF_FORMAT) { // this block only relevant to libsndfile ...
		// detect if input format is a floating-point format:
		bool bFloat = false;
		bool bDouble = false;
		switch (inputFileFormat & SF_FORMAT_SUBMASK) {
		case SF_FORMAT_FLOAT:
			bFloat = true;
			break;
		case SF_FORMAT_DOUBLE:
			bDouble = true;
			break;
		}

		for (auto& subformat : subFormats) { // scan subformats for a match:
			if (subformat.second == (inputFileFormat & SF_FORMAT_SUBMASK)) {
				std::cout << "input bit format: " << subformat.first;
				break;
			}
		}

		if (bFloat)
			std::cout << " (float)";
		if (bDouble)
			std::cout << " (double precision)";

		std::cout << std::endl;
	}

	std::cout << "source file channels: " << nChannels << std::endl;
	std::cout << "input sample rate: " << ci.inputSampleRate << "\noutput sample rate: " << ci.outputSampleRate << std::endl;

	FloatType peakInputSample;
	sf_count_t peakInputPosition = 0LL;
	sf_count_t samplesRead = 0LL;
	sf_count_t totalSamplesRead = 0LL;

	if (ci.bEnablePeakDetection) {
		peakInputSample = 0.0;
		std::cout << "Scanning input file for peaks ...";

		do {
			samplesRead = infile.read(inputBlock.data(), inputBlockSize);
			for (unsigned int s = 0; s < samplesRead; ++s) { // read all samples, without caring which channel they belong to
				if (std::abs(inputBlock[s]) > peakInputSample) {
					peakInputSample = std::abs(inputBlock[s]);
					peakInputPosition = totalSamplesRead + s;
				}
			}
			totalSamplesRead += samplesRead;
		} while (samplesRead > 0);

		std::cout << "Done\n";
		std::cout << "Peak input sample: " << std::fixed << peakInputSample << " (" << 20 * log10(peakInputSample) << " dBFS) at ";
		printSamplePosAsTime(peakInputPosition, ci.inputSampleRate);
		std::cout << std::endl;
		infile.seek(0, SEEK_SET); // rewind back to start of file
	}

	else { // no peak detection
		peakInputSample = ci.bNormalize ?
					0.5  /* ... a guess, since we haven't actually measured the peak (in the case of DSD, it is a good guess.) */ :
					1.0;
	}

	if (ci.bNormalize) { // echo Normalization settings to user
		auto prec = std::cout.precision();
		std::cout << "Normalizing to " << std::setprecision(2) << ci.limit << std::endl;
		std::cout.precision(prec);
	}

	// echo filter settings to user:
	double targetNyquist = std::min(ci.inputSampleRate, ci.outputSampleRate) / 2.0;
	double ft = (ci.lpfCutoff / 100.0) * targetNyquist;
	auto prec = std::cout.precision();
	std::cout << "LPF transition frequency: " << std::fixed << std::setprecision(2) << ft << " Hz (" << 100 * ft / targetNyquist << " %)" << std::endl;
	std::cout.precision(prec);
	if (ci.bMinPhase) {
		std::cout << "Using Minimum-Phase LPF" << std::endl;
	}

	// echo conversion ratio to user:
	FloatType resamplingFactor = static_cast<FloatType>(ci.outputSampleRate) / ci.inputSampleRate;
	std::cout << "Conversion ratio: " << resamplingFactor
			  << " (" << fraction.numerator << ":" << fraction.denominator << ")" << std::endl;

	// if the outputFormat is zero, it means "No change to file format"
	// if output file format has changed, use outputFormat. Otherwise, use same format as infile:
	int outputFileFormat = ci.outputFormat ? ci.outputFormat : inputFileFormat;

	// if the minor (sub) format of outputFileFormat is not set, attempt to use minor format of input file (as a last resort)
	if ((outputFileFormat & SF_FORMAT_SUBMASK) == 0) {
		outputFileFormat |= (inputFileFormat & SF_FORMAT_SUBMASK); // may not be valid subformat for new file format.
	}

	// for wav files, determine whether to switch to rf64 mode:
	if ((outputFileFormat & SF_FORMAT_TYPEMASK) == SF_FORMAT_WAV || (outputFileFormat & SF_FORMAT_TYPEMASK) == SF_FORMAT_WAVEX) {
		if (ci.bRf64 ||
				checkWarnOutputSize(inputSampleCount, getSfBytesPerSample(outputFileFormat), fraction.numerator, fraction.denominator)) {
			std::cout << "Switching to rf64 format !" << std::endl;
			outputFileFormat &= ~SF_FORMAT_TYPEMASK; // clear file type
			outputFileFormat |= SF_FORMAT_RF64;
		}
	}

	// note: libsndfile has an rf64 auto-downgrade mode:
	// http://www.mega-nerd.com/libsndfile/command.html#SFC_RF64_AUTO_DOWNGRADE
	// However, rf64 auto-downgrade is more appropriate for recording applications
	// (where the final file size cannot be known until the recording has stopped)
	// In the case of sample-rate conversions, the output file size (and therefore the decision to promote to rf64)
	// can be determined at the outset.

	// Determine the value of outputSignalBits, based on outputFileFormat.
	// outputSignalsBits is used to set the level of the LSB for dithering
	int outputSignalBits;
	switch (outputFileFormat & SF_FORMAT_SUBMASK) {
	case SF_FORMAT_PCM_24:
		outputSignalBits = 24;
		break;
	case SF_FORMAT_PCM_S8:
	case SF_FORMAT_PCM_U8:
		outputSignalBits = 8;
		break;
	case SF_FORMAT_DOUBLE:
		outputSignalBits = 53;
		break;
	case SF_FORMAT_FLOAT:
		outputSignalBits = 21;
		break;
	default:
		outputSignalBits = 16;
	}

	if (ci.quantize) {
		outputSignalBits = std::max(1, std::min(ci.quantizeBits, outputSignalBits));
	}

	// confirm dithering options for user:
	if (ci.bDither) {
		auto prec = std::cout.precision();
		std::cout << "Generating " << std::setprecision(2) << ci.ditherAmount << " bits of " << ditherProfileList[ci.ditherProfileID].name << " dither for " << outputSignalBits << "-bit output format";
		std::cout.precision(prec);
		if (ci.bAutoBlankingEnabled)
			std::cout << ", with auto-blanking";
		std::cout << std::endl;
	}

	// make a vector of ditherers (one ditherer for each channel):
	std::vector<Ditherer<FloatType>> ditherers;
	ditherers.reserve(static_cast<size_t>(nChannels));
	auto seed = static_cast<int>(ci.bUseSeed ? ci.seed : time(nullptr));

	for (int n = 0; n < nChannels; n++) {
		// to-do: explore other seed-generation options (remote possibility of overlap)
		// maybe use a single global RNG ?
		// or use discard/jump-ahead ... to ensure parallel streams are sufficiently "far away" from each other ?
		ditherers.emplace_back(outputSignalBits, ci.ditherAmount, ci.bAutoBlankingEnabled, n + seed, static_cast<DitherProfileID>(ci.ditherProfileID));
	}

	// make a vector of Resamplers
	std::vector<Converter<FloatType>> converters;
	converters.reserve(static_cast<size_t>(nChannels));
	for (int n = 0; n < nChannels; n++) {
		converters.emplace_back(ci);
	}

	// Calculate initial gain:
	FloatType gain = static_cast<FloatType>(ci.gain) * static_cast<FloatType>(converters[0].getGain()) *
			static_cast<FloatType>(ci.bNormalize ? fraction.numerator * (ci.limit / static_cast<double>(peakInputSample)) : fraction.numerator * ci.limit);

	// todo: more testing with very low bit depths (eg 4 bits)
	if (ci.bDither) { // allow headroom for dithering:
		FloatType ditherCompensation =
				(pow(2, outputSignalBits - 1) - pow(2, ci.ditherAmount - 1)) / pow(2, outputSignalBits - 1); // eg 32767/32768 = 0.999969 (-0.00027 dB)
		gain *= ditherCompensation;
	}

	int groupDelay = static_cast<int>(converters[0].getGroupDelay());

	FloatType peakOutputSample;
	bool bClippingDetected;
	RaiiTimer timer(inputDuration);

	int clippingProtectionAttempts = 0;

	do { // clipping detection loop (repeats if clipping detected AND not using a temp file)

		infile.seek(0, SEEK_SET);
		peakInputSample = 0.0;
		bClippingDetected = false;
		std::unique_ptr<SndfileHandle> outFile;
		std::unique_ptr<CsvFile> csvFile;

		if (ci.csvOutput) { // csv output
			csvFile.reset(new CsvFile(ci.outputFilename));
			csvFile->setNumChannels(nChannels);

			// defaults
			csvFile->setNumericBase(Decimal);
			csvFile->setIntegerWriteScalingStyle(ci.integerWriteScalingStyle);
			csvFile->setSignedness(Signed);
			csvFile->setNumericFormat(Integer);

			if (ci.outBitFormat.empty()) { // default = 16-bit, unsigned, integer (decimal)
				csvFile->setNumBits(16);
			}
			else {
				std::regex rgx("([us]?)(\\d+)([fiox]?)"); // [u|s]<numBits>[f|i|o|x]
				std::smatch matchResults;
				std::regex_search(ci.outBitFormat, matchResults, rgx);
				int numBits = 16;

				if (matchResults.length() >= 1 && matchResults[1].compare("u") == 0) {
					csvFile->setSignedness(Unsigned);
				}

				if (matchResults.length() >= 2 && std::stoi(matchResults[2]) != 0) {
					numBits = std::min(std::max(1, std::stoi(matchResults[2])), 64); // 1-64 bits
				}

				if (matchResults.length() >= 3 && !matchResults[3].str().empty()) {
					if (matchResults[3].compare("f") == 0) {
						csvFile->setNumericFormat(FloatingPoint);
					}
					else if (matchResults[3].compare("o") == 0) {
						csvFile->setNumericBase(Octal);
					}
					else if (matchResults[3].compare("x") == 0) {
						csvFile->setNumericBase(Hexadecimal);
					}
				}

				csvFile->setNumBits(numBits);

				// todo: precision, other params
			}
		}

		else { // libSndFile output

			try {

				// output file may need to be overwriten on subsequent passes,
				// and the only way to close the file is to destroy the SndfileHandle.

				outFile.reset(new SndfileHandle(ci.outputFilename, SFM_WRITE, outputFileFormat, nChannels, ci.outputSampleRate));

				if (int e = outFile->error()) {
					std::cout << "Error: Couldn't Open Output File (" << sf_error_number(e) << ")" << std::endl;
					return false;
				}

				if (ci.bNoPeakChunk) {
					outFile->command(SFC_SET_ADD_PEAK_CHUNK, nullptr, SF_FALSE);
				}

				if (ci.bWriteMetaData) {
					if (!setMetaData(m, *outFile)) {
						std::cout << "Warning: problem writing metadata to output file ( " << outFile->strError() << " )" << std::endl;
					}
				}

				// if the minor (sub) format of outputFileFormat is flac, and user has requested a specific compression level, set compression level:
				if (((outputFileFormat & SF_FORMAT_FLAC) == SF_FORMAT_FLAC) && ci.bSetFlacCompression) {
					std::cout << "setting flac compression level to " << ci.flacCompressionLevel << std::endl;
					double cl = ci.flacCompressionLevel / 8.0; // there are 9 flac compression levels from 0-8. Normalize to 0-1.0
					outFile->command(SFC_SET_COMPRESSION_LEVEL, &cl, sizeof(cl));
				}

				// if the minor (sub) format of outputFileFormat is vorbis, and user has requested a specific quality level, set quality level:
				if (((outputFileFormat & SF_FORMAT_VORBIS) == SF_FORMAT_VORBIS) && ci.bSetVorbisQuality) {

					auto prec = std::cout.precision();
					std::cout.precision(1);
					std::cout << "setting vorbis quality level to " << ci.vorbisQuality << std::endl;
					std::cout.precision(prec);

					double cl = (1.0 - ci.vorbisQuality) / 11.0; // Normalize from (-1 to 10), to (1.0 to 0) ... why is it backwards ?
					outFile->command(SFC_SET_COMPRESSION_LEVEL, &cl, sizeof(cl));
				}
			}

			catch (std::exception& e) {
				std::cout << "Error: Couldn't Open Output File " << e.what() << std::endl;
				return false;
			}
		}

		// conditionally open a temp file:
		if (ci.bTmpFile) {
			tmpSndfileHandle = getTempFile<FloatType>(inputFileFormat, nChannels, ci, tmpFilename);
			if (tmpSndfileHandle == nullptr) {
				ci.bTmpFile = false;
			}
		} // ends opening of temp file

		// echo conversion mode to user (multi-stage/single-stage, multi-threaded/single-threaded)
		std::string stageness(ci.bMultiStage ? "multi-stage" : "single-stage");
		std::string threadedness(ci.bMultiThreaded ? ", multi-threaded" : "");
		std::cout << "Converting (" << stageness << threadedness << ") ..." << std::endl;

		peakOutputSample = 0.0;
		totalSamplesRead = 0;
		sf_count_t incrementalProgressThreshold = (ci.progressUpdates > 0 ) ? inputSampleCount / ci.progressUpdates : inputSampleCount + 1;
		sf_count_t nextProgressThreshold = incrementalProgressThreshold;

		int outStartOffset = std::min(groupDelay * nChannels, static_cast<int>(outputBlockSize) - nChannels);

		// initialise output (post-conversion) processing
		EffectChain<FloatType> outputChain;
		outputChain.setOutputBufferSize(outputBlockSize);
		outputChain.setChannelCount(nChannels);
		outputChain.setTakeOwnership(true);

		if(ci.bAdjustStereoWidth && nChannels == 2) {
			auto stereoImager = new StereoImager<FloatType>;
			std::cout << "adjusting stereo width" << std::endl;
			stereoImager->setStereoWidth(ci.stereoWidth);
			outputChain.add(stereoImager);
		}

		if(ci.bFadeIn) {
			auto fadeEffect = new FadeEffect<FloatType>;
			fadeEffect->setSampleRate(ci.outputSampleRate);
			fadeEffect->setFadeIn(ci.fadeInTime);
			outputChain.add(fadeEffect);
		}

		bool hasOutputFX = !outputChain.empty();
		// ---

		do { // central conversion loop (the heart of the matter ...)

			// Grab a block of interleaved samples from file:
			samplesRead = infile.read(inputBlock.data(), inputBlockSize);
			totalSamplesRead += samplesRead;

			// de-interleave into channel buffers
			size_t i = 0;
			for (int s = 0; s < samplesRead; s += nChannels) {
				for (int ch = 0; ch < nChannels; ++ch) {
					inputChannelBuffers[ch][i] = inputBlock[s + ch];
				}
				++i;
			}

			struct Result {
				size_t outBlockindex;
				FloatType peak;
			};


			std::vector<std::future<Result>> results(nChannels);
			ctpl::thread_pool threadPool(nChannels);
			size_t outputBlockIndex = 0;

			for (int ch = 0; ch < nChannels; ++ch) { // run convert stage for each channel (concurrently)

				auto kernel = [&, ch](int) {
					FloatType* iBuf = inputChannelBuffers[ch].data();
					FloatType* oBuf = outputChannelBuffers[ch].data();
					size_t o = 0;
					FloatType localPeak = 0.0;
					size_t localOutputBlockIndex = 0;
					converters[ch].convert(oBuf, o, iBuf, i);
					for (size_t f = 0; f < o; ++f) {
						// note: disable dither for temp files (dithering to be done in post)
						FloatType outputSample = (ci.bDither && !ci.bTmpFile) ? ditherers[ch].dither(gain * oBuf[f]) : gain * oBuf[f]; // gain, dither
						localPeak = std::max(localPeak, std::abs(outputSample)); // peak
						outputBlock[localOutputBlockIndex + ch] = outputSample; // interleave
						localOutputBlockIndex += nChannels;
					}
					Result res{};
					res.outBlockindex = localOutputBlockIndex;
					res.peak = localPeak;
					return res;
				};

				if (multiThreaded) {
					results[ch] = threadPool.push(kernel);
				} else {
					Result res = kernel(0);
					peakOutputSample = std::max(peakOutputSample, res.peak);
					outputBlockIndex = res.outBlockindex;
				}
			}

			if (multiThreaded) { // collect results:
				for (int ch = 0; ch < nChannels; ++ch) {
					Result res = results[ch].get();
					peakOutputSample = std::max(peakOutputSample, res.peak);
					outputBlockIndex = res.outBlockindex;
				}
			}

			// process output (with Group Delay Compensation):
			sf_count_t outputSampleCount = outputBlockIndex - outStartOffset;
			const FloatType* outputData = hasOutputFX ?
						outputChain.process(outputBlock.data(), outputSampleCount) + outStartOffset :
						outputBlock.data() + outStartOffset;

			// write out to either temp file or outfile
			if (ci.bTmpFile) {
				tmpSndfileHandle->write(outputData, outputSampleCount);
			}
			else {
				if (ci.csvOutput) {
					csvFile->write(outputData, outputSampleCount);
				}
				else {
					outFile->write(outputData, outputSampleCount);
				}
			}

			outStartOffset = 0; // reset after first use

			// conditionally send progress update:
			if (totalSamplesRead > nextProgressThreshold) {
				int progressPercentage = std::min(static_cast<int>(99), static_cast<int>(100 * totalSamplesRead / inputSampleCount));
				OutputManager::callProgressFunc(progressPercentage);
				nextProgressThreshold += incrementalProgressThreshold;
			}

		} while (samplesRead > 0); // ends central conversion loop

		if (ci.bTmpFile) {
			gain = 1.0; // output file must start with unity gain relative to temp file
		}
		else {
			// notify user:
			std::cout << "Done" << std::endl;
			auto prec = std::cout.precision();
			std::cout << "Peak output sample: " << std::setprecision(6) << peakOutputSample << " (" << 20 * log10(peakOutputSample) << " dBFS)" << std::endl;
			std::cout.precision(prec);
		}

		do {
			// test for clipping:
			if (!ci.disableClippingProtection && peakOutputSample > ci.limit) {

				std::cout << "\nClipping detected !" << std::endl;

				// calculate gain adjustment
				FloatType gainAdjustment = static_cast<FloatType>(clippingTrim) * ci.limit / peakOutputSample;
				gain *= gainAdjustment;

				// echo gain adjustment to user - use slightly differnt message if using temp file:
				if (ci.bTmpFile) {
					std::cout << "Adjusting gain by " << 20 * log10(gainAdjustment) << " dB" << std::endl;
				}
				else {
					std::cout << "Re-doing with " << 20 * log10(gainAdjustment) << " dB gain adjustment" << std::endl;
				}

				// reset the ditherers
				if (ci.bDither) {
					for (auto &ditherer : ditherers) {
						ditherer.adjustGain(gainAdjustment);
						ditherer.reset();
					}
				}

				// reset the converters
				for (auto &converter : converters) {
					converter.reset();
				}

			} // ends test for clipping

			// if using temp file, write to outFile
			if (ci.bTmpFile) {

				std::cout << "Writing to output file ...\n";
				std::vector<FloatType> outBuf(inputBlockSize, 0);
				peakOutputSample = 0.0;
				totalSamplesRead = 0;
				nextProgressThreshold = incrementalProgressThreshold;

				tmpSndfileHandle->seek(0, SEEK_SET);
				if (!ci.csvOutput) {
					outFile->seek(0, SEEK_SET);
				}

				do { // Grab a block of interleaved samples from temp file:
					samplesRead = tmpSndfileHandle->read(inputBlock.data(), inputBlockSize);
					totalSamplesRead += samplesRead;

					// de-interleave into channels, apply gain, add dither, and save to output buffer
					size_t i = 0;
					for (int s = 0; s < samplesRead; s += nChannels) {
						for (int ch = 0; ch < nChannels; ++ch) {
							FloatType smpl = ci.bDither ? ditherers[ch].dither(gain * inputBlock[i]) :
														  gain * inputBlock[i];
							peakOutputSample = std::max(std::abs(smpl), peakOutputSample);
							outBuf[i++] = smpl;
						}
					}

					// write output buffer to outfile
					if (ci.csvOutput) {
						csvFile->write(outBuf.data(), i);
					}
					else {
						outFile->write(outBuf.data(), i);
					}

					// conditionally send progress update:
					if (totalSamplesRead > nextProgressThreshold) {
						int progressPercentage = std::min(static_cast<int>(99),
														  static_cast<int>(100 * totalSamplesRead / inputSampleCount));
						OutputManager::callProgressFunc(progressPercentage);
						nextProgressThreshold += incrementalProgressThreshold;
					}

				} while (samplesRead > 0);

				std::cout << "Done" << std::endl;
				auto prec = std::cout.precision();
				std::cout << "Peak output sample: " << std::setprecision(6) << peakOutputSample << " (" << 20 * log10(peakOutputSample) << " dBFS)" << std::endl;
				std::cout.precision(prec);

			} // ends if (ci.bTmpFile)

			bClippingDetected = peakOutputSample > ci.limit;
			if (bClippingDetected)
				clippingProtectionAttempts++;

			// explanation of 'while' loops:
			// 1. when clipping is detected and temp file is in use, go back to re-adjusting gain, resetting ditherers etc and repeat
			// 2. when clipping is detected and temp file NOT used, go all the way back to reading the input file, and running the whole conversion again
			// (This whole control structure might be better served with good old gotos ...)

		} while (ci.bTmpFile && !ci.disableClippingProtection && bClippingDetected && clippingProtectionAttempts < maxClippingProtectionAttempts); // if using temp file, do another round if clipping detected
	} while (!ci.bTmpFile && !ci.disableClippingProtection && bClippingDetected && clippingProtectionAttempts < maxClippingProtectionAttempts); // if NOT using temp file, do another round if clipping detected

	// clean-up temp file:
	delete tmpSndfileHandle; // dealllocate SndFileHandle

#if defined (TEMPFILE_OPEN_METHOD_STD_TMPNAM) || defined (TEMPFILE_OPEN_METHOD_WINAPI)
	std::remove(tmpFilename.c_str()); // actually remove the temp file from disk
#endif

	return true;
} // ends convert()

// getTempFile() : opens a temp file (wav/rf64 file in floating-point format).
// Double- or single- precision is determined by FloatType.
// Dynamically allocates a SndfileHandle.
// Returns SndfileHandle pointer, which is the caller's responsibility to delete.
// Returns nullptr if unsuccessful.

template<typename FloatType>
SndfileHandle* getTempFile(int inputFileFormat, int nChannels, const ConversionInfo& ci, std::string& tmpFilename) {

	SndfileHandle* tmpSndfileHandle = nullptr;
	bool tmpFileError;
	int outputFileFormat = ci.outputFormat ? ci.outputFormat : inputFileFormat;

	// set major format of temp file (inherit rf64-ness from output file):
	int tmpFileFormat = (outputFileFormat & SF_FORMAT_RF64) ? SF_FORMAT_RF64 : SF_FORMAT_WAV;

	// set appropriate floating-point subformat:
	tmpFileFormat |= (sizeof(FloatType) == 8) ? SF_FORMAT_DOUBLE : SF_FORMAT_FLOAT;

#if defined (TEMPFILE_OPEN_METHOD_WINAPI)
	TCHAR _tmpFilename[MAX_PATH];
	TCHAR _tmpPathname[MAX_PATH];
	tmpFileError = true;
	DWORD pathLen;

	if (!ci.tmpDir.empty()) {
		pathLen = static_cast<DWORD>(ci.tmpDir.length());
		std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>, wchar_t> widener;
		wcscpy_s(_tmpPathname, MAX_PATH, widener.from_bytes(ci.tmpDir).c_str());
	}
	else {
		pathLen = GetTempPath(MAX_PATH, _tmpPathname);
	}

	if (pathLen > MAX_PATH || pathLen == 0)
		std::cerr << "Error: Could not determine temp path for temp file" << std::endl;
	else {
		if (GetTempFileName(_tmpPathname, TEXT("ReS"), 0, _tmpFilename) == 0)
			std::cerr << "Error: Couldn't generate temp file name" << std::endl;
		else {
			tmpFileError = false;
			std::wstring_convert<std::codecvt_utf8<wchar_t>> wchar2utf8;
			tmpFilename = wchar2utf8.to_bytes(_tmpFilename);
			if (ci.bShowTempFile) std::cout << "Temp Filename: " << tmpFilename << std::endl;
			tmpSndfileHandle = new SndfileHandle(tmpFilename, SFM_RDWR, tmpFileFormat, nChannels, ci.outputSampleRate); // open using filename
		}
	}

#elif defined (TEMPFILE_OPEN_METHOD_STD_TMPFILE)
	FILE* f = std::tmpfile();
	tmpFileError = (f == NULL);
	if (!tmpFileError) {
		tmpSndfileHandle = new SndfileHandle(fileno(f), true, SFM_RDWR, tmpFileFormat, nChannels, ci.outputSampleRate); // open using file descriptor
	}
	else {
		std::cerr << "std::tmpfile() failed" << std::endl;
	}

#elif defined (TEMPFILE_OPEN_METHOD_MKSTEMP)
	char templ[] = "ReSamplerXXXXXX";
	int fd = mkstemp(templ);
	tmpFileError = (fd == -1);
	if (!tmpFileError) {
		if (ci.bShowTempFile) printf("temp file: %s\n", templ);
		tmpSndfileHandle = new SndfileHandle(fd, true, SFM_RDWR, tmpFileFormat, nChannels, ci.outputSampleRate); // open using file descriptor
	}
	else {
		std::cerr << "std::mkstemp() failed" << std::endl;
	}

#else
	// tmpnam() method
	tmpFileError = false;
	tmpFilename = std::string(std::string(std::tmpnam(nullptr)) + ".wav");
	if (ci.bShowTempFile) std::cout << "Temp Filename: " << tmpFilename << std::endl;
	tmpSndfileHandle = new SndfileHandle(tmpFilename, SFM_RDWR, tmpFileFormat, nChannels, ci.outputSampleRate); // open using filename

#endif

	int e = 0;
	if (tmpFileError || tmpSndfileHandle == nullptr || (e = tmpSndfileHandle->error())) {
		std::cout << "Error: Couldn't Open Temporary File (" << sf_error_number(e) << ")\n";
		std::cout << "Disabling temp file mode." << std::endl;
		tmpSndfileHandle = nullptr;
	}
	else {
		// disable floating-point normalisation (important - we want to record/recover floating point values exactly)
		if (sizeof(FloatType) == 8) {
			tmpSndfileHandle->command(SFC_SET_NORM_DOUBLE, NULL,
									  SF_FALSE); // http://www.mega-nerd.com/libsndfile/command.html#SFC_SET_NORM_DOUBLE
		}
		else {
			tmpSndfileHandle->command(SFC_SET_NORM_FLOAT, NULL, SF_FALSE);
		}
	}

	return tmpSndfileHandle;
}

// retrieve metadata using libsndfile API :
bool getMetaData(MetaData& metadata, SndfileHandle& infile) {
	const char* empty = "";
	const char* str;

	metadata.title.assign((str = infile.getString(SF_STR_TITLE)) ? str : empty);
	metadata.copyright.assign((str = infile.getString(SF_STR_COPYRIGHT)) ? str : empty);
	metadata.software.assign((str = infile.getString(SF_STR_SOFTWARE)) ? str : empty);
	metadata.artist.assign((str = infile.getString(SF_STR_ARTIST)) ? str : empty);
	metadata.comment.assign((str = infile.getString(SF_STR_COMMENT)) ? str : empty);
	metadata.date.assign((str = infile.getString(SF_STR_DATE)) ? str : empty);
	metadata.album.assign((str = infile.getString(SF_STR_ALBUM)) ? str : empty);
	metadata.license.assign((str = infile.getString(SF_STR_LICENSE)) ? str : empty);
	metadata.trackNumber.assign((str = infile.getString(SF_STR_TRACKNUMBER)) ? str : empty);
	metadata.genre.assign((str = infile.getString(SF_STR_GENRE)) ? str : empty);

	// retrieve Broadcast Extension (bext) chunk, if it exists:
	metadata.has_bext_fields = (infile.command(SFC_GET_BROADCAST_INFO, (void*)&metadata.broadcastInfo, sizeof(SF_BROADCAST_INFO)) == SF_TRUE);

	if (metadata.has_bext_fields) {
		std::cout << "Input file contains a Broadcast Extension (bext) chunk" << std::endl;
	}

	// retrieve cart chunk, if it exists:
	metadata.has_cart_chunk = (infile.command(SFC_GET_CART_INFO, (void*)&metadata.cartInfo, sizeof(LargeSFCartInfo)) == SF_TRUE);

	if (metadata.has_cart_chunk) {
		// Note: size of CART chunk is variable, depending on size of last field (tag_text[])
		if (metadata.cartInfo.tag_text_size > MAX_CART_TAG_TEXT_SIZE) {
			metadata.cartInfo.tag_text_size = MAX_CART_TAG_TEXT_SIZE; // apply hard limit on number of characters (spec says unlimited ...)
		}
		std::cout << "Input file contains a cart chunk" << std::endl;
	}
	return true;
}

// set metadata using libsndfile API :
bool setMetaData(const MetaData& metadata, SndfileHandle& outfile) {

	std::cout << "Writing Metadata" << std::endl;
	if (!metadata.title.empty()) outfile.setString(SF_STR_TITLE, metadata.title.c_str());
	if (!metadata.copyright.empty()) outfile.setString(SF_STR_COPYRIGHT, metadata.copyright.c_str());
	if (!metadata.software.empty()) outfile.setString(SF_STR_SOFTWARE, metadata.software.c_str());
	if (!metadata.artist.empty()) outfile.setString(SF_STR_ARTIST, metadata.artist.c_str());
	if (!metadata.comment.empty()) outfile.setString(SF_STR_COMMENT, metadata.comment.c_str());
	if (!metadata.date.empty()) outfile.setString(SF_STR_DATE, metadata.date.c_str());
	if (!metadata.album.empty()) outfile.setString(SF_STR_ALBUM, metadata.album.c_str());
	if (!metadata.license.empty()) outfile.setString(SF_STR_LICENSE, metadata.license.c_str());
	if (!metadata.trackNumber.empty()) outfile.setString(SF_STR_TRACKNUMBER, metadata.trackNumber.c_str());
	if (!metadata.genre.empty()) outfile.setString(SF_STR_GENRE, metadata.genre.c_str());

	if (((outfile.format() &  SF_FORMAT_TYPEMASK) == SF_FORMAT_WAV) ||
			((outfile.format() &  SF_FORMAT_TYPEMASK) == SF_FORMAT_WAVEX) ||
			((outfile.format() &  SF_FORMAT_TYPEMASK) == SF_FORMAT_RF64)) { /* some sort of wav file */

		// attempt to write bext / cart chunks:
		if (metadata.has_bext_fields) {
			outfile.command(SFC_SET_BROADCAST_INFO, (void*)&metadata.broadcastInfo, sizeof(SF_BROADCAST_INFO));
		}

		if (metadata.has_cart_chunk) {
			outfile.command(SFC_SET_CART_INFO,
							(void*)&metadata.cartInfo,
							sizeof(metadata.cartInfo) - MAX_CART_TAG_TEXT_SIZE + metadata.cartInfo.tag_text_size // (size of cartInfo WITHOUT tag text) + (actual size of tag text)
							);
		}
	}

	return (outfile.error() == 0);
}

bool testSetMetaData(SndfileHandle& outfile) {
	MetaData m;
	m.title.assign("test title");
	m.copyright.assign("test copyright");
	m.software.assign("test software");
	m.artist.assign("test artist");
	m.comment.assign("test comment");
	m.date.assign("test date");
	m.album.assign("test album");
	m.license.assign("test license");
	m.trackNumber.assign("test track number");
	m.genre.assign("test genre");
	return setMetaData(m, outfile);
}

void showDitherProfiles() {
	for (int d = DitherProfileID::flat; d != DitherProfileID::end; ++d) {
		std::cout << ditherProfileList[d].id << " : " << ditherProfileList[d].name << std::endl;
	}
}

int getSfBytesPerSample(int format) {
	int subformat = format & SF_FORMAT_SUBMASK;
	switch (subformat) {
	case SF_FORMAT_PCM_S8:
		return 1;
	case SF_FORMAT_PCM_16:
		return 2;
	case SF_FORMAT_PCM_24:
		return 3;
	case SF_FORMAT_PCM_32:
		return 4;
	case SF_FORMAT_PCM_U8:
		return 1;
	case SF_FORMAT_FLOAT:
		return 4;
	case SF_FORMAT_DOUBLE:
		return 8;
	default:
		return 2; // for safety
	}
}

bool checkWarnOutputSize(sf_count_t inputSamples, int bytesPerSample, int numerator, int denominator)
{
	sf_count_t outputDataSize = inputSamples * bytesPerSample * numerator / denominator;

	const sf_count_t limit4G = 1ULL << 32;
	if (outputDataSize >= limit4G) {
		std::cout << "Warning: output file ( " << fmtNumberWithCommas(outputDataSize) << " bytes of data ) will exceed 4GB limit" << std::endl;
		return true;
	}
	return false;
}

template<typename IntType>
std::string fmtNumberWithCommas(IntType n) {
	std::string s = std::to_string(n);
	int64_t insertPosition = s.length() - 3;
	while (insertPosition > 0) {
		s.insert(static_cast<size_t>(insertPosition), ",");
		insertPosition -= 3;
	}
	return s;
}

void printSamplePosAsTime(sf_count_t samplePos, unsigned int sampleRate) {
	double seconds = static_cast<double>(samplePos) / sampleRate;
	auto h = static_cast<int>(seconds / 3600);
	auto m = static_cast<int>((seconds - (h * 3600)) / 60);
	double s = seconds - (h * 3600) - (m * 60);
	std::ios::fmtflags f(std::cout.flags());
	std::cout << std::setprecision(0) << h << ":" << m << ":" << std::setprecision(6) << s;
	std::cout.flags(f);
}

bool testSetMetaData(DsfFile& outfile) {
	(void)outfile; // unused
	// stub - to-do
	return true;
}

bool testSetMetaData(DffFile& outfile) {
	(void)outfile; // unused
	// stub - to-do
	return true;
}

bool getMetaData(MetaData& metadata, const DffFile& f) {
	(void)metadata; // unused
	(void)f; // unused
	// stub - to-do
	return true;
}

bool getMetaData(MetaData& metadata, const DsfFile& f) {
	(void)metadata; // unused
	(void)f; // unused
	// stub - to-do
	return true;
}

bool getMetaData(MetaData& metadata, const IQFile& f) {
	(void)metadata; // unused
	(void)f; // unused
	// stub - to-do
	return true;
}

#ifndef USE_QUADMATH

void generateExpSweep(const std::string& filename, int sampleRate, int format, double duration, int nOctaves, double amplitude_dB) {
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

void generateExpSweep(const std::string& filename, int sampleRate, int format, double duration, int nOctaves, double amplitude_dB) {

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

bool checkSSE2() {
#if defined (_MSC_VER) || defined (__INTEL_COMPILER)
	bool bSSE2ok = false;
	int CPUInfo[4] = { 0,0,0,0 };
	__cpuid(CPUInfo, 0);
	if (CPUInfo[0] != 0) {
		__cpuid(CPUInfo, 1);
		if (CPUInfo[3] & (1 << 26))
			bSSE2ok = true;
	}
	if (bSSE2ok) {
		std::cout << "CPU supports SSE2 (ok)";
		return true;
	}
	else {
		std::cout << "Your CPU doesn't support SSE2 - please try a non-SSE2 build on this machine" << std::endl;
		return false;
	}
#endif // defined (_MSC_VER) || defined (__INTEL_COMPILER)
	return true; // todo: fix the check on gcc
}

bool checkAVX() {
#if defined (_MSC_VER) || defined (__INTEL_COMPILER)
	// Verify CPU capabilities:
	bool bAVXok = false;
	int cpuInfo[4] = { 0,0,0,0 };
	__cpuid(cpuInfo, 0);
	if (cpuInfo[0] != 0) {
		__cpuid(cpuInfo, 1);
		if (cpuInfo[2] & (1 << 28)) {
			bAVXok = true; // Note: this test only confirms CPU AVX capability, and does not check OS capability.
			// to-do: check for AVX2 ...
		}
	}
	if (bAVXok) {
		std::cout << "CPU supports AVX (ok)";
		return true;
	}
	else {
		std::cout << "Your CPU doesn't support AVX - please try a non-AVX build on this machine" << std::endl;
		return false;
	}
#endif // defined (_MSC_VER) || defined (__INTEL_COMPILER)
	return true; // todo: gcc detection
}

bool showBuildVersion() {
	std::cout << strVersion << " ";
#if defined(_M_X64) || defined(__x86_64__) || defined(__aarch64__)
	std::cout << "64-bit version";
#ifdef USE_AVX
	std::cout << " AVX build ... ";
	if (!checkAVX())
		return false;
#ifdef USE_FMA
	std::cout << "\nusing FMA (Fused Multiply-Add) instruction ... ";
#endif
#endif // USE_AVX
	std::cout << std::endl;
#else
	std::cout << "32-bit version";
#if defined(USE_SSE2)
	std::cout << ", SSE2 build ... ";
	// Verify processor capabilities:
	if (!checkSSE2())
		return false;
#endif // defined(USE_SSE2)
	std::cout << "\n" << std::endl;
#endif
	return true;
}

void showCompiler() {
	// https://sourceforge.net/p/predef/wiki/Compilers/
#if defined (__clang__)
	std::cout << "Clang " << __clang_major__ << "."
			  << __clang_minor__ << "."
			  << __clang_patchlevel__ << std::endl;
#elif defined (__MINGW64__)
	std::cout << "minGW-w64" << std::endl;
#elif defined (__MINGW32__)
	std::cout << "minGW" << std::endl;
#elif defined (__GNUC__)
	std::cout << "gcc " << __GNUC__ << "."
			  << __GNUC_MINOR__ << "."
			  << __GNUC_PATCHLEVEL__ << std::endl;
#elif defined (_MSC_VER)
	std::cout << "Visual C++ " << _MSC_FULL_VER << std::endl;
#elif defined (__INTEL_COMPILER)
	std::cout << "Intel Compiler " << __INTEL_COMPILER << std::endl;
#else
	std::cout << "unknown" << std::endl;
#endif
}


int runCommand(int argc, char** argv) {

	// test for global options
	if (parseGlobalOptions(argc, argv)) {
		return EXIT_SUCCESS;
	}

	// ConversionInfo instance to hold parameters
	ConversionInfo ci;

	// get path/name of this app
#ifdef __APPLE__
	char pathBuf[PROC_PIDPATHINFO_MAXSIZE];
	pid_t pid = getpid();
	if (proc_pidpath(pid, pathBuf, sizeof(pathBuf)) == 0) {
		ci.appName.assign(pathBuf);
	}
#else
	ci.appName = argv[0];
#endif

	ci.overSamplingFactor = 1;

	// get conversion parameters
	ci.fromCmdLineArgs(argc, argv);
	if (ci.bBadParams) {
		std::cout << strUsage << std::endl;
		return EXIT_FAILURE;
	}

	// query build version AND cpu
	if (!showBuildVersion()) {
		return EXIT_FAILURE; // can't continue (CPU / build mismatch)
	}

	// echo filenames to user
	std::cout << "Input file: " << ci.inputFilename << std::endl;
	std::cout << "Output file: " << ci.outputFilename << std::endl;

	if (ci.disableClippingProtection) {
		std::cout << "clipping protection disabled " << std::endl;
	}

	// Isolate the file extensions
	std::string inFileExt;
	std::string outFileExt;

	if (ci.inputFilename.find_last_of('.') != std::string::npos) {
		inFileExt = ci.inputFilename.substr(ci.inputFilename.find_last_of('.') + 1);
	}

	if (ci.outputFilename.find_last_of('.') != std::string::npos) {
		outFileExt = ci.outputFilename.substr(ci.outputFilename.find_last_of('.') + 1);
	}

	// detect dsf or dff format
	ci.dsfInput = (inFileExt == "dsf");
	ci.dffInput = (inFileExt == "dff");

	// detect csv output
	ci.csvOutput = (outFileExt == "csv");

	if (ci.csvOutput) {
		std::cout << "Outputting to csv format" << std::endl;
	}

	else {
		if (!ci.outBitFormat.empty()) {  // new output bit format requested
			ci.outputFormat = determineOutputFormat(outFileExt, ci.outBitFormat);
			if (ci.outputFormat) {
				std::cout << "Changing output bit format to " << ci.outBitFormat << std::endl;
			}
			else { // user-supplied bit format not valid; try choosing appropriate format
				std::string outBitFormat;
				determineBestBitFormat(outBitFormat, ci);
				ci.outputFormat = determineOutputFormat(outFileExt, outBitFormat);
				if (ci.outputFormat) {
					ci.outBitFormat = outBitFormat;
					std::cout << "Changing output bit format to " << ci.outBitFormat << std::endl;
				}
				else {
					std::cout << "Warning: NOT Changing output file bit format !" << std::endl;
					ci.outputFormat = 0; // back where it started
				}
			}
		}

		if (outFileExt != inFileExt) { // file extensions differ, determine new output format:

			std::string outBitFormat{ci.outBitFormat};
			if (ci.outBitFormat.empty()) { // user changed file extension only. Attempt to choose appropriate output sub format:
				std::cout << "Output Bit Format not specified" << std::endl;
				determineBestBitFormat(outBitFormat, ci);
			}
			ci.outputFormat = determineOutputFormat(outFileExt, outBitFormat);
			if (ci.outputFormat) {
				ci.outBitFormat = outBitFormat;
				std::cout << "Changing output file format to " << outFileExt << std::endl;
			} else { // cannot determine subformat of output file
				std::cout << "Warning: NOT Changing output file format ! (extension different, but format will remain the same)" << std::endl;
			}
		}
	}

	try {

		if (ci.bUseDoublePrecision) {

#ifdef USE_QUADMATH
			std::cout << "Using quadruple-precision for calculations.\n";
#else
			std::cout << "Using double precision for calculations." << std::endl;
#endif

			if (ci.dsfInput) {
				ci.bEnablePeakDetection = false;
				return convert_DsfFile_Double(ci) ? EXIT_SUCCESS : EXIT_FAILURE;
			}

			if (ci.dffInput) {
				ci.bEnablePeakDetection = false;
				return convert_DffFile_Double(ci) ? EXIT_SUCCESS : EXIT_FAILURE;
			}

			ci.bEnablePeakDetection = true;

			if(ci.bDemodulateIQ) {
                if(ci.IQModulationType == WFM) {
					// WFM decoding too slow to read peaks
                    ci.bEnablePeakDetection = false;

					// determine the strategy for lowpass-filtering
					if(ci.outputSampleRate <= 30000) {
						// deactivate MPX Decoder's LPF - just use the Sample Rate Converter's lowpass
						ci.IQModulationType = WFM_NO_LOWPASS;

					} else if(ci.outputSampleRate <= 96000) {
						// deactivate MPX Decoder's LPF
						ci.IQModulationType = WFM_NO_LOWPASS;

						// use the Sample Rate Converter's lowpass, but change the cutoff to 15khz etc
						ci.lpfCutoff =
							100.0 * MpxDecoder::getLpfT() / (0.5 * ci.outputSampleRate); // converts absolute freq (Hz) to % of output nyquist
						ci.lpfTransitionWidth =
							100.0 * MpxDecoder::getLpfW() / (0.5 * ci.outputSampleRate);
					}/* else {
						// leave MPX decoder's LPF active
					}*/

                } else {
                    ci.bEnablePeakDetection = true;
                    ci.normalizeAmount = 1.00;
                    ci.bNormalize = true;
                }
				return convert_IQFile_Double(ci) ? EXIT_SUCCESS : EXIT_FAILURE;
			}

			return convert_SndfileHandle_Double(ci) ? EXIT_SUCCESS : EXIT_FAILURE;

		} // if (ci.bUseDoublePrecision)


#ifdef USE_QUADMATH
		std::cout << "Using quadruple-precision for calculations.\n";
#endif
		if (ci.dsfInput) {
			ci.bEnablePeakDetection = false;
			return convert_DsfFile_Float(ci) ? EXIT_SUCCESS : EXIT_FAILURE;
		}

		if (ci.dffInput) {
			ci.bEnablePeakDetection = false;
			return convert_DffFile_Float(ci) ? EXIT_SUCCESS : EXIT_FAILURE;
		}

		ci.bEnablePeakDetection = true;

		if(ci.bDemodulateIQ) {
			return convert_IQFile_Float(ci) ? EXIT_SUCCESS : EXIT_FAILURE;
		}

		return convert_SndfileHandle_Float(ci) ? EXIT_SUCCESS : EXIT_FAILURE;

	} //ends try block

	catch (const std::exception& e) {
		std::cerr << "fatal error: " << e.what();
		return EXIT_FAILURE;
	}
}

std::function<void(int)> OutputManager::progressFunc = [](int percentComplete) {
	std::cout << percentComplete << "%"
								 #ifndef COMPILING_ON_ANDROID
								 << "\b\b\b"
								 #endif
								 << std::flush;
};

std::function<void (int)> OutputManager::getProgressFunc()
{
	return progressFunc;
}

void OutputManager::setProgressFunc(const std::function<void (int)> &value)
{
	progressFunc = value;
}

void OutputManager::callProgressFunc(int percentComplete) {
	return progressFunc(percentComplete);
}

} // namespace ReSampler
