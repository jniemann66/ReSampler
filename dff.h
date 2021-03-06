/*
* Copyright (C) 2016 - 2021 Judd Niemann - All Rights Reserved.
* You may use, distribute and modify this code under the
* terms of the GNU Lesser General Public License, version 2.1
*
* You should have received a copy of GNU Lesser General Public License v2.1
* with this file. If not, please refer to: https://github.com/jniemann66/ReSampler
*/

#ifndef DFF_H_
#define DFF_H_

#include "osspecific.h"

#ifdef BYTESWAP_METHOD_MSVCRT
#include <stdlib.h>
#endif

#include <algorithm>
#include <iostream>
#include <cassert>
#include <cstdint>
#include <string>
#include <fstream>

#define DFF_MAX_CHANNELS 6 
#define DFF_FORMAT 0x00300000 // note: take care to make sure this doesn't clash with future libsndfile formats (unlikely)
#pragma pack(push, r1, 1)

namespace ReSampler {

struct dffChunkHeader {
	uint32_t ckID; // chunkid
	uint64_t ckDataSize; // chunk data size, in bytes
};

struct FormatVersionChunk {
	uint32_t ckID; // 'FVER'
	uint64_t ckDataSize; // 4
	uint32_t version; // 0x01050000 version 1.5.0.0 DSDIFF
};

struct SampleRateChunk {
	uint32_t ckID; // 'FS '
	uint64_t ckDataSize; // 4
	uint32_t sampleRate; // sample rate in [Hz]
};

struct ChannelsChunk {
	uint32_t ckID; // 'CHNL'
	uint64_t ckDataSize;
	uint16_t numChannels; // number of audio channels
	uint32_t channelID[DFF_MAX_CHANNELS]; // channels ID's
};

struct CompressionTypeChunk {
	uint32_t ckID; // 'CMPR'
	uint64_t CkDataSize;
	uint32_t compressionType; // compression ID code
	uint8_t Count; // length of the compression name
	char compressionName[257]; // human readable type name
};

struct AbsoluteStartTimeChunk {
	uint32_t ckID; // 'ABSS'
	uint64_t ckDataSize;
	uint16_t hours; // hours
	uint8_t minutes; // minutes
	uint8_t seconds; // seconds
	uint32_t samples; // samples
};

struct LoudspeakerConfigurationChunk {
	uint32_t ckID; // 'LSCO'
	uint64_t ckDataSize; // 2
	uint16_t lsConfig; // loudspeaker configuration
};

struct PropertyChunk {
	uint32_t ckID; // 'PROP'
	uint64_t ckDataSize;
	uint32_t propType; // 'SND '
	SampleRateChunk sampleRateChunk;
	ChannelsChunk channelsChunk;
	CompressionTypeChunk compressionTypeChunk;
	AbsoluteStartTimeChunk absoluteStartTimeChunk;
	LoudspeakerConfigurationChunk loudspeakerConfigurationChunk;
};

struct DSDSoundDataHeader {
	uint32_t ckID; // 'DSD '
	uint64_t ckDataSize;
};

struct FormDSDChunk {
	uint32_t ckID; // 'FRM8'
	uint64_t ckDataSize; // FORM's data size, in bytes
	uint32_t formType; // 'DSD '
	FormatVersionChunk formatVersionChunk;
	PropertyChunk propertyChunk;
	DSDSoundDataHeader dsdSoundDataHeader;
};

#pragma pack(pop, r1)

// chunk IDs:
#define CKID_FRM8 0x46524d38
#define CKID_FVER 0x46564552
#define CKID_PROP 0x50524f50
#define CKID_COMT 0x434f4d54
#define CKID_DSD 0x44534420
#define CKID_DST 0x44535420
#define CKID_DSTI 0x44535449
#define CKID_DIIN 0x4449494e
#define CKID_MANF 0x4d414e46
#define CKID_FS	0x46532020
#define CKID_CHNL 0x43484e4c
#define CKID_CMPR 0x434d5052
#define CKID_ABSS 0x41425353
#define CKID_LSCO 0x4c53434f
#define CKID_FRTE 0x46525445
#define CKID_DSTF 0x44535446
#define CKID_DSTC 0x44535443
#define CKID_EMID 0x454d4944
#define CKID_MARK 0x4d41524b
#define CKID_DIAR 0x44494152
#define CKID_DITI 0x44495449

enum DffOpenMode {
	Dff_read,
	Dff_write
};

// DffFile interface:

class DffFile
{
public:
	// Construction / destruction

#ifdef __clang__
	// see www.open-std.org/jtc1/sc22/wg21/docs/cwg_defects.html#777
	explicit DffFile(const std::string& path, int mode = Dff_read, int ignored1 = 0, int ignored2 = 0, int ignored3 = 0) : path(path), mode(static_cast<DffOpenMode>(mode))
#else
	template <typename... OtherArgs>
	explicit DffFile(const std::string& path, int mode = Dff_read, OtherArgs... ignored) : path(path), mode(static_cast<DffOpenMode>(mode))
#endif

	{
		file.exceptions(std::ifstream::failbit | std::ifstream::badbit);

		switch (mode) {
		case Dff_read:
			try {
				file.open(path, std::ios::in | std::ios::binary);
				err = false;
			}

			catch (std::ios_base::failure& e) {
				e.what();
				err = true;
				return;
			}

			makeTbl();
			readHeaders();

			if (err)
				return;

			bufferSize = blockSize * numChannels;
			inputBuffer = new uint8_t[bufferSize];
			totalBytesRead = 0;
			endOfBlock = bufferSize;
			bufferIndex = endOfBlock; // empty (zero -> full)
			currentBit = 0;
			currentChannel = 0;
			break;

		case Dff_write:
			break;
		}
	}

	~DffFile() {
		if (file.is_open())
			file.close();

		delete[] inputBuffer;
	}

	// API:

	bool error() const {
		return err;
	}

	unsigned int channels() const {
		return numChannels;
	}

	unsigned int samplerate() const {
		return _sampleRate;
	}

	uint64_t frames() const {
		return numFrames;
	}

	uint64_t samples() const {
		return numSamples;
	}

	int format() const {
		return DFF_FORMAT;
	}

	template<typename FloatType>
	uint64_t read(FloatType* buffer, uint64_t count) {

		/*

		In a dff file,
			* data is big-endian
			* Channel interleaving is done at the byte level.
			* In each byte, the MSB is played first; the LSB is played last.

		*/

		// Caller expects interleaving to be done at the _sample_ level

		uint64_t samplesRead = 0;

		for (uint64_t i = 0; i < count; ++i) {

			if (bufferIndex >= endOfBlock) { // end of buffer ; fetch more data from file
				endOfBlock = readBlocks();
				if (endOfBlock == 0) {
					break; // no more data
				}
				bufferIndex = 0;
			}

			buffer[i] = samplTbl[inputBuffer[bufferIndex + currentChannel]][currentBit];
			++samplesRead;

			// cycle through channels, then bits, then bufferIndex

			if (++currentChannel == numChannels) {
				currentChannel = 0;
				if (++currentBit == 8) {
					currentBit = 0;
					bufferIndex += numChannels;
				}
			}
		}
		return samplesRead;
	}

	// testRead() : reads the entire file
	// and confirms number of samples read equals number of samples expected:

	void testRead() {
		float sampleBuffer[8192];
		uint64_t samplesRead = 0;
		uint64_t totalSamplesRead = 0;
		while ((samplesRead = read(sampleBuffer, 8192)) != 0) {
			totalSamplesRead += samplesRead;
		}
		std::cout << "samples expected: " << numSamples << std::endl;
		std::cout << "total samples retrieved: " << totalSamplesRead << std::endl;
	}

	uint64_t seek(uint64_t pos, int whence) {
		(void)whence; // unused
		// To-do: allow seeks to positions other than beginning (requires proper calculations)
		// reset state to initial conditions:
		totalBytesRead = 0;
		endOfBlock = bufferSize;
		bufferIndex = endOfBlock; // empty (zero -> full)
		currentBit = 0;
		currentChannel = 0;

		// rewind file pointer
		file.clear(); // in case of eof
		file.seekg(startOfData + pos);
		return pos;
	}

private:
	FormDSDChunk formDSDChunk{};
	std::string path;
	DffOpenMode mode;
	std::fstream file;
	bool err;
	const uint32_t blockSize = 4096;
	uint64_t endOfBlock;
	uint64_t bufferSize;
	uint64_t totalSoundDataBytes{};
	uint64_t totalBytesRead;
	uint32_t numChannels{};
	uint32_t _sampleRate{};
	uint64_t numSamples{};
	uint64_t numFrames{};
	uint8_t* inputBuffer;
	uint64_t bufferIndex;
	uint32_t currentChannel;
	uint32_t currentBit;
	uint64_t startOfData{};
	double samplTbl[256][8]{};

	void getChunkHeader(dffChunkHeader* chunkHeader) {
		chunkHeader->ckID = bigEndianRead32();
		chunkHeader->ckDataSize = bigEndianRead64();
	}

#if !defined(BYTESWAP_METHOD_MSVCRT) && !defined(BYTESWAP_METHOD_BUILTIN)
	uint16_t swapEndian(uint16_t x) {
		union {
			struct {
				uint8_t a;
				uint8_t b;
			};
			uint16_t n;
		} y, z;

		y.n = x;
		z.a = y.b;
		z.b = y.a;
		return z.n;
	}

	uint32_t swapEndian(uint32_t x) {
		union {
			struct {
				uint8_t a;
				uint8_t b;
				uint8_t c;
				uint8_t d;
			};
			uint32_t n;
		} y, z;

		y.n = x;
		z.a = y.d;
		z.b = y.c;
		z.c = y.b;
		z.d = y.a;
		return z.n;
	}

	uint64_t swapEndian(uint64_t x) {
		union {
			struct {
				uint8_t a;
				uint8_t b;
				uint8_t c;
				uint8_t d;
				uint8_t e;
				uint8_t f;
				uint8_t g;
				uint8_t h;
			};
			uint64_t n;
		} y, z;

		y.n = x;
		z.a = y.h;
		z.b = y.g;
		z.c = y.f;
		z.d = y.e;
		z.e = y.d;
		z.f = y.c;
		z.g = y.b;
		z.h = y.a;
		return z.n;
	}
#endif

	uint8_t bigEndianRead8() {
		uint8_t v;
		file.read((char*)&v, sizeof(v));
		return v;
	}

	uint16_t bigEndianRead16() {
		uint16_t v;
		file.read((char*)&v, sizeof(v));

#if  defined(BYTESWAP_METHOD_MSVCRT)
		return _byteswap_ushort(v);
#elif defined(BYTESWAP_METHOD_BUILTIN)
		return __builtin_bswap16(v);
#else
		return swapEndian(v);
#endif

	}

	uint32_t bigEndianRead32() {
		uint32_t v;
		file.read((char*)&v, sizeof(v));

#if  defined(BYTESWAP_METHOD_MSVCRT)
		return _byteswap_ulong(v);
#elif defined(BYTESWAP_METHOD_BUILTIN)
		return __builtin_bswap32(v);
#else
		return swapEndian(v);
#endif

	}

	uint64_t bigEndianRead64() {
		uint64_t v;
		file.read((char*)&v, sizeof(v));

#if  defined(BYTESWAP_METHOD_MSVCRT)
		return _byteswap_uint64(v);
#elif defined(BYTESWAP_METHOD_BUILTIN)
		return __builtin_bswap64(v);
#else
		return swapEndian(v);
#endif

	}

	// readPropChunks() : read the sub-chunks of PROP chunk
	void readPropChunks(int64_t chunkSize) {
		int64_t dataRemaining = chunkSize;
		dffChunkHeader nextChunkHeader;
		do {

			if (err)
				break;

			getChunkHeader(&nextChunkHeader);

			uint64_t dataSize = nextChunkHeader.ckDataSize;

			switch (nextChunkHeader.ckID) {
			case CKID_FS:
				formDSDChunk.propertyChunk.sampleRateChunk.ckID = nextChunkHeader.ckID;
				formDSDChunk.propertyChunk.sampleRateChunk.ckDataSize = dataSize;
				_sampleRate = formDSDChunk.propertyChunk.sampleRateChunk.sampleRate = bigEndianRead32();
				break;

			case CKID_CHNL:
				formDSDChunk.propertyChunk.channelsChunk.ckID = nextChunkHeader.ckID;
				formDSDChunk.propertyChunk.channelsChunk.ckDataSize = dataSize;
				numChannels = formDSDChunk.propertyChunk.channelsChunk.numChannels = bigEndianRead16();
				for (unsigned int c = 0; c < std::min(numChannels, (uint32_t)DFF_MAX_CHANNELS); ++c) {
					formDSDChunk.propertyChunk.channelsChunk.channelID[c] = bigEndianRead32();
				}
				if (numChannels > DFF_MAX_CHANNELS) {
					std::cout << "Too manny Channels: " << numChannels << " (max " << DFF_MAX_CHANNELS << ")" << std::endl;
					err = true;
					return;
				}
				break;

			case CKID_CMPR:
				formDSDChunk.propertyChunk.compressionTypeChunk.ckID = nextChunkHeader.ckID;
				formDSDChunk.propertyChunk.compressionTypeChunk.CkDataSize = dataSize;
				formDSDChunk.propertyChunk.compressionTypeChunk.compressionType = bigEndianRead32();
				formDSDChunk.propertyChunk.compressionTypeChunk.Count = bigEndianRead8();
				file.read(formDSDChunk.propertyChunk.compressionTypeChunk.compressionName, formDSDChunk.propertyChunk.compressionTypeChunk.Count+1);
				formDSDChunk.propertyChunk.compressionTypeChunk.compressionName[formDSDChunk.propertyChunk.compressionTypeChunk.Count] = 0; // null terminator
				if (formDSDChunk.propertyChunk.compressionTypeChunk.compressionType != CKID_DSD) {
					std::cout << "Sorry, compression not supported: "  << formDSDChunk.propertyChunk.compressionTypeChunk.compressionName << std::endl;
					err = true; // can't handle compressed data
					return;
				}
				break;

			case CKID_ABSS:
				formDSDChunk.propertyChunk.absoluteStartTimeChunk.ckID = nextChunkHeader.ckID;
				formDSDChunk.propertyChunk.absoluteStartTimeChunk.ckDataSize = dataSize;
				formDSDChunk.propertyChunk.absoluteStartTimeChunk.hours = bigEndianRead16();
				formDSDChunk.propertyChunk.absoluteStartTimeChunk.minutes = bigEndianRead8();
				formDSDChunk.propertyChunk.absoluteStartTimeChunk.seconds = bigEndianRead8();
				formDSDChunk.propertyChunk.absoluteStartTimeChunk.samples = bigEndianRead32();
				break;

			case CKID_LSCO:
				formDSDChunk.propertyChunk.loudspeakerConfigurationChunk.ckID = nextChunkHeader.ckID;
				formDSDChunk.propertyChunk.loudspeakerConfigurationChunk.ckDataSize = nextChunkHeader.ckDataSize;
				formDSDChunk.propertyChunk.loudspeakerConfigurationChunk.lsConfig = bigEndianRead16();
				break;

			default:
				file.seekg(dataSize, file.cur); // who cares ? skip to next chunk ...
			}
			dataRemaining -= (sizeof(dffChunkHeader) + dataSize);
		} while (dataRemaining > 0);
	}

	void readHeaders() {
		formDSDChunk.ckID = bigEndianRead32();
		if (formDSDChunk.ckID != CKID_FRM8) {
			err = true;
			return;
		}
		formDSDChunk.ckDataSize = bigEndianRead64();
		formDSDChunk.formType = bigEndianRead32();
		if (formDSDChunk.formType != CKID_DSD) {
			err = true;
			return;
		}

		dffChunkHeader nextChunkHeader;

		// read the chunks we care about ...
		do {
			getChunkHeader(&nextChunkHeader);

			uint64_t dataSize = nextChunkHeader.ckDataSize;

			switch (nextChunkHeader.ckID) {
			case CKID_FVER:
				formDSDChunk.formatVersionChunk.ckID = nextChunkHeader.ckID;
				formDSDChunk.formatVersionChunk.ckDataSize = nextChunkHeader.ckDataSize;
				formDSDChunk.formatVersionChunk.version = bigEndianRead32();
				break;

			case CKID_PROP:
				formDSDChunk.propertyChunk.ckID = nextChunkHeader.ckID;
				formDSDChunk.propertyChunk.ckDataSize = dataSize;
				formDSDChunk.propertyChunk.propType = bigEndianRead32();
				readPropChunks(dataSize - sizeof(formDSDChunk.propertyChunk.propType));
				break;

			case CKID_DSD:
				formDSDChunk.dsdSoundDataHeader.ckID = nextChunkHeader.ckID;
				formDSDChunk.dsdSoundDataHeader.ckDataSize = nextChunkHeader.ckDataSize;
				totalSoundDataBytes = formDSDChunk.dsdSoundDataHeader.ckDataSize;
				assert(totalSoundDataBytes % numChannels == 0); // must be multiple of numChannels
				numSamples = 8 * totalSoundDataBytes;
				numFrames = numSamples / numChannels;
				break;

			default:
				file.seekg(dataSize, file.cur); // who cares ? skip to next chunk ...
			}
		} while (nextChunkHeader.ckID != CKID_DSD);

		startOfData = static_cast<uint64_t>(file.tellg()); // should be ready to read data stream now ...
	}

	uint64_t readBlocks() {
		if (file.eof()) {
			return 0;
		}

		uint64_t bytesRemaining = totalSoundDataBytes - totalBytesRead;
		uint64_t bytesToRead = std::min(bufferSize, bytesRemaining);
		file.read((char*)inputBuffer, static_cast<std::streamsize>(bytesToRead));
		uint64_t bytesActuallyRead = static_cast<uint64_t>(file.gcount());
		totalBytesRead += bytesActuallyRead;
		return bytesActuallyRead;
	}

	void makeTbl() { // generate sample translation table
		for (int i = 0; i < 256; ++i) {
			for (int j = 0; j < 8; ++j) {
				samplTbl[i][j] = (i & (1 << (7-j))) ? 1.0 : -1.0; // MSB-first
			}
		}
	}
};

} // namespace ReSampler

#endif // DFF_H_
