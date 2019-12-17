/* 
* Copyright (C) 2016 - 2019 Judd Niemann - All Rights Reserved.
* You may use, distribute and modify this code under the
* terms of the GNU Lesser General Public License, version 2.1
*
* You should have received a copy of GNU Lesser General Public License v2.1
* with this file. If not, please refer to: https://github.com/jniemann66/ReSampler
*/

#ifndef DSF_H_
#define DSF_H_

// dsf.h
// simple dsf file reader

#include "osspecific.h"

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <string>
#include <iostream>
#include <fstream>

#define DSF_FORMAT 0x00310000 // note: take care to make sure this doesn't clash with future libsndfile formats (unlikely)

#pragma pack(push, r1, 1)

namespace ReSampler {

struct DsfDSDChunk {
	uint32_t header;	// expected: "DSD "
	uint64_t length;	// expected: 28
	uint64_t filesize;
	uint64_t metadataPtr;
};

enum DsfChannelType {
	mono = 1,
	stereo,
	ch3,
	quad,
	ch4,
	ch5,
	ch51
};

struct DsfFmtChunk {
	uint32_t header;	// expected: "fmt "
	uint64_t length;	// expected: 52
	uint32_t version;	// expected: 1
	uint32_t formatID;	// expected: 0
	uint32_t channelType;
	uint32_t numChannels;
	uint32_t sampleRate;	// expected: 2822400 or 5644800

	uint32_t bitOrder;		// Note: in the spec, this field is called 'Bits per sample.'
							// However, apparently (as stated in Annotation 4), it actually indicates the bit order (not the sample width). 
							// 1 -> LSB first, 8 -> MSB First

	uint64_t numSamples;
	uint32_t blockSize;	// expected: 4096
	uint32_t reserved;	// expected: zero
};

struct DsfDataChunk {
	uint32_t header;	// expected: "data"
	uint64_t length;	// expected: 12 + sample data length
};
#pragma pack(pop, r1)

enum DsfOpenMode {
	Dsf_read,
	Dsf_write
};

#define DSF_ID_DSD 0x20445344
#define DSF_ID_FMT 0x20746d66
#define DSF_ID_DATA 0x61746164

#define DSF_STD_BLOCKSIZE 4096
#define DSF_MAX_BLOCKSIZE 32768
#define DSF_MAX_FMTCHUNKSIZE 1024

// DsfFile interface:

class DsfFile 
{
public:
	// Construction / destruction

#ifdef __clang__
	// see www.open-std.org/jtc1/sc22/wg21/docs/cwg_defects.html#777
	explicit DsfFile(const std::string& path, int mode = Dsf_read, int ignored1 = 0, int ignored2 = 0, int ignored3 = 0) : path(path), mode(static_cast<DsfOpenMode>(mode))
#else
	template<typename... OtherArgs>
	DsfFile(const std::string& path, int mode = Dsf_read, OtherArgs... ignored) : path(path), mode(static_cast<DsfOpenMode>(mode))
#endif

	{
		assertSizes();
		file.exceptions(std::ifstream::failbit | std::ifstream::badbit);

		switch (mode) {
		case Dsf_read:
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
			for (int n = 0; n < 6; ++n) {
				channelBuffer[n] = new uint8_t[blockSize];
			}
			bufferIndex = blockSize; // empty (zero -> full)
			currentBit = 0;
			currentChannel = 0;
			break;

		case Dsf_write:
			break;
		}
	}

	~DsfFile() {
		if(file.is_open())
			file.close();
		for (int n = 0; n < 6; ++n) {
			delete[] channelBuffer[n];
		}
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
		return DSF_FORMAT;
	}

	// read() : reads count interleaved FloatType samples into buffer

	template<typename FloatType>
	uint64_t read(FloatType* buffer, uint64_t count) {

		/*
		
		In a dsf file, 

		Channel interleaving is done at the block level:
		
		{BLOCKSIZE bytes} -> Channel 0, 
		{BLOCKSIZE bytes} -> channel 1, 
		... 
		{BLOCKSIZE bytes} -> channel n
		
		In each byte, 
			if(bitOrder == 1)
				the LSB is played first; the MSB is played last.
			if(bitOrder == 8)
				the MSB is played first; the LSB is played last.

		*/

		// Caller expects interleaving to be done at the _sample_ level 

		uint64_t samplesRead = 0;

		for (uint64_t i = 0; i < count; ++i) {

			if (bufferIndex == blockSize) { // end of buffer ; fetch more data from file
				if (readBlocks() == 0) { 
					break; // no more data
				}
				bufferIndex = 0;
			}

			buffer[i] = static_cast<FloatType>(samplTbl[channelBuffer[currentChannel][bufferIndex]][currentBit]);
		
			++samplesRead;

			// cycle through channels, then bits, then bufferIndex

			if (++currentChannel == numChannels) {
				currentChannel = 0;
				if (++currentBit == 8) {
					currentBit = 0;
					++bufferIndex;
				}
			}
		}
		return samplesRead;
	}

	// testRead() : reads the entire file 
	// and confirms number of samples read equals number of samples expected:

	void testRead() {

		const size_t bufSize = 8192;

		float sampleBuffer[bufSize];
		uint64_t totalSamplesRead = 0;
		uint64_t samplesRead = 0;

		while ((samplesRead = read(sampleBuffer, bufSize)) != 0) {
			totalSamplesRead += samplesRead;
		}

		std::cout << "samples expected: " << numSamples << std::endl;
		std::cout << "total samples retrieved: " << totalSamplesRead << std::endl;
	}

	uint64_t seek(uint64_t pos, int whence) {
		(void)whence; // unused
		// reset initial conditions:
		bufferIndex = blockSize; // empty (zero -> full)
		currentBit = 0;
		currentChannel = 0;

		// seek:
		file.clear();
		file.seekg(startOfData + pos);
		return pos;
	}

private:
	DsfDSDChunk dsfDSDChunk;
	DsfFmtChunk dsfFmtChunk;
	DsfDataChunk dsfDataChunk;
	DsfChannelType dsfChannelType;
	std::string path;
	DsfOpenMode mode;
	std::fstream file;
	bool err;
	uint32_t blockSize;
	uint32_t numChannels;
	uint32_t _sampleRate;
	uint64_t numSamples;
	uint64_t numFrames;
	uint8_t* channelBuffer[6];
	uint64_t bufferIndex;
	uint32_t currentChannel;
	uint32_t currentBit;
	uint64_t startOfData;
	uint64_t endOfData;
	double samplTbl[256][8];
	
	void assertSizes() {
		static_assert(sizeof(dsfDSDChunk) == 28, "");
		static_assert(sizeof(dsfFmtChunk) == 52, "");
		static_assert(sizeof(dsfDataChunk) == 12, "");
	}

	// checkWarnChunkSize() :
	// Check that chunk is expected size. 
	// Send warning if not.
	// Return difference between chunk length and expected chunk length

	template<typename T> int checkWarnChunkSize(size_t statedLength, const char* chunkName) {
		if (sizeof(T) != statedLength) {
			std::cout << "warning: '" << chunkName << "' chunk is " << statedLength << " bytes. (" << sizeof(T) << " bytes expected)" << std::endl;
		}
		return static_cast<int>(statedLength) - static_cast<int>(sizeof(T));
	}

	// warnWrongChunk() : inform user that expected chunk is missing
	void warnWrongChunk(const char* chunkName) {
		std::cout << "error: '" << chunkName << "' chunk missing !" << std::endl;
	}

	// readHeaders() : read and interpret the file header chunks ("DSD", "fmt ", and "data")
	void readHeaders() {

		// read DSD chunk:
		file.read((char*)&dsfDSDChunk, sizeof(dsfDSDChunk));
		if (dsfDSDChunk.header != DSF_ID_DSD) {
			warnWrongChunk("DSD ");
			err = true;
			return;
		}
		checkWarnChunkSize<DsfDSDChunk>(dsfDSDChunk.length, "DSD ");
		
		// read fmt chunk:
		file.read((char*)&dsfFmtChunk, sizeof(dsfFmtChunk));
		if (dsfFmtChunk.header != DSF_ID_FMT) {
			warnWrongChunk("fmt ");
			err = true;
			return;
		}
		
		std::streamoff offset = checkWarnChunkSize<DsfFmtChunk>(dsfFmtChunk.length, "fmt ");
		if (offset != 0) { // allow for some flexibility in chunk size, since spec says 'usually 52'
			if (dsfFmtChunk.length > DSF_MAX_FMTCHUNKSIZE) { // ... but not too much flexibility
				err = true;
				return;
			}
			file.seekg(offset, std::ios_base::cur); // relative seek to next chunk 
		}

		// read data chunk:
		file.read((char*)&dsfDataChunk, sizeof(dsfDataChunk));
		if (dsfDataChunk.header != DSF_ID_DATA) {
			warnWrongChunk("data");
			err = true;
			return;
		}

		// check block size:
		blockSize = dsfFmtChunk.blockSize;
		if (blockSize != DSF_STD_BLOCKSIZE) {
			std::cout << "Non-standard block size: " << blockSize << std::endl;
			if (blockSize > DSF_MAX_BLOCKSIZE) {
				std::cout << "Block size too large!" << std::endl;
				err = true;
				return;
			}
		}

		numChannels = dsfFmtChunk.numChannels;
		_sampleRate = dsfFmtChunk.sampleRate;
		numFrames = dsfFmtChunk.numSamples;
		numSamples = numFrames * numChannels;
		dsfChannelType = (DsfChannelType)dsfFmtChunk.channelType;
		
		if (dsfFmtChunk.bitOrder == 8) {
			std::cout << "bitstream in MSB-first format" << std::endl;
		}

		startOfData = static_cast<uint64_t>(file.tellg());
		endOfData = dsfDSDChunk.length + dsfFmtChunk.length + dsfDataChunk.length;

		assert( // metadata tag either non-existent or at end of data
			(dsfDSDChunk.metadataPtr == 0) ||
			(dsfDSDChunk.metadataPtr == endOfData)
		);
	}

	// readBlocks() : reads blockSize bytes into each channelBuffer for numChannels channels
	uint32_t readBlocks() {
		if (file.tellg() >= static_cast<std::istream::pos_type>(endOfData))
			return 0;

		for (size_t ch = 0; ch < numChannels; ++ch) {
			file.read((char*)channelBuffer[ch], blockSize);
		}
		return blockSize;
	}

	// makeTbl() : translates all possible uint8_t values into sets of 8 floating point sample values.
	void makeTbl() { // generate sample translation table
		for (int i = 0; i < 256; ++i) {
			for (int j = 0; j < 8; ++j) {
				int mask = 1 << ((dsfFmtChunk.bitOrder == 8) ? 7 - j : j); // reverse bits if 'bitOrder' == 8
				samplTbl[i][j] = (i & mask) ? 1.0 : -1.0;
			}
		}
	}
};

} // namespace ReSampler

#endif // DSF_H_
