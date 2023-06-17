/*
* Copyright (C) 2016 - 2023 Judd Niemann - All Rights Reserved.
* You may use, distribute and modify this code under the
* terms of the GNU Lesser General Public License, version 2.1
*
* You should have received a copy of GNU Lesser General Public License v2.1
* with this file. If not, please refer to: https://github.com/jniemann66/ReSampler
*/

// csv.h : defines module for exporting audio data as a csv file

#ifndef RESAMPLER_CSV_H
#define RESAMPLER_CSV_H

#include "osspecific.h"

#include <algorithm>
#include <iostream>
#include <iomanip>
#include <cassert>
#include <cstdint>
#include <string>
#include <fstream>
#include <cmath>
#include <string>
#include <vector>

namespace ReSampler {

enum CsvOpenMode {
	csv_read,
	csv_write
};

enum CsvSignedness {
	Signed,
	Unsigned
};

enum CsvNumericFormat {
	Integer,
	FloatingPoint,
	Fixed,
	Scientific
};

enum CsvNumericBase {
	Binary = 2,
	Octal = 8,
	Decimal = 10,
	Hexadecimal = 16
};

enum IntegerWriteScalingStyle {
	Pow2Clip,
	Pow2Minus1
};

class CsvFile {
public:
	CsvFile(const std::string& path, CsvOpenMode mode = csv_write) : path(path), mode(mode), numChannels(2), numericFormat(Integer), signedness(Signed), numericBase(Decimal), numBits(16), precision(10), integerWriteScalingStyle(Pow2Minus1),
		intMaxAmplitude(32767), unsignedOffset(0)
	{

		file.exceptions(std::ifstream::failbit | std::ifstream::badbit);
		currentChannel = 0;

		switch (mode) {
		case csv_read:
			try {
				file.open(path, std::ios::in | std::ios::binary);
				err = false;
			}
			catch (std::ios_base::failure& e) {
				e.what();
				err = true;
				return;
			}
			break;

		case csv_write:
			try {
				file.open(path, std::ios::out | std::ios::binary);
				err = false;
			}
			catch (std::ios_base::failure& e) {
				e.what();
				err = true;
				return;
			}
			break;
		}
	}

	bool isErr() const {
		return err;
	}

	~CsvFile() {
		if(file.is_open()) {
			file.close();
		}
	}

	template <typename T>
	int64_t write(const T* buffer, int64_t count) {
		if(err) {
			return 0;
		}
		int64_t i;
		for(i = 0; i < count; i++) {
			switch(numericFormat) {
				case CsvNumericFormat::FloatingPoint:
					file << buffer[i];
					break;

				default:
					file << scaleToInt<int>(buffer[i]);
					break;
			}

			if(++currentChannel < numChannels) {
				file << ",";
			} else {
				file  << "\r\n";
				currentChannel = 0;
			}
		}
		return i;
	}

private:
	std::string path;
	CsvOpenMode mode;
	std::fstream file;
	int numChannels;
	CsvNumericFormat numericFormat;
	double scaleFactor;
	CsvSignedness signedness;
	CsvNumericBase numericBase;
	int numBits;
	int precision;
	IntegerWriteScalingStyle integerWriteScalingStyle;
	int currentChannel;
	bool err;
	int intMaxAmplitude;
	int unsignedOffset;

	template <typename IntType, typename FloatType>
	IntType scaleToInt(FloatType x) {
		return unsignedOffset + std::min(std::max(-intMaxAmplitude, static_cast<IntType>(std::round(scaleFactor * x))), intMaxAmplitude - 1);
	}

	void setStreamFormat() {
		if(file.is_open()) {
			file.unsetf(std::ios_base::floatfield);
			if(numericFormat == FloatingPoint) {
				file << std::setprecision(precision);
			} else if (numericFormat == Integer) {
				file.setf(std::ios_base::fixed);
				file << std::setprecision(0);
			}
			else if (numericFormat == Fixed) {
				file.setf(std::ios_base::fixed);
				file << std::setprecision(precision);
			}
			else if (numericFormat == Scientific) {
				file.setf(std::ios_base::scientific, std::ios_base::floatfield);
				file << std::setprecision(precision);
			}

			if (numericBase == Hexadecimal) {
				file.setf(std::ios_base::hex, std::ios_base::basefield);
				file.setf(std::ios_base::showbase);
			}
			else if (numericBase == Octal) {
				file.setf(std::ios_base::oct, std::ios_base::basefield);
				file.setf(std::ios_base::showbase);
			}
			else {
				file.setf(std::ios_base::dec, std::ios_base::basefield);
			}

		}
	}

public:
	CsvNumericFormat getNumericFormat() const {
		return numericFormat;
	}

	void setNumericFormat(CsvNumericFormat numericFormat) {
		CsvFile::numericFormat = numericFormat;
		setStreamFormat();
	}

	CsvSignedness getSignedness() const {
		return signedness;
	}

	void setSignedness(CsvSignedness signedness) {
		CsvFile::signedness = signedness;
		setStreamFormat();
	}

	CsvNumericBase getNumericBase() const {
		return numericBase;
	}

	void setNumericBase(CsvNumericBase numericBase) {
		CsvFile::numericBase = numericBase;
		setStreamFormat();
	}

	int getNumBits() const {
		return numBits;
	}

	void setNumBits(int numBits) {
		intMaxAmplitude = 1 << (numBits - 1);
		unsignedOffset = (signedness == Signed) ? 0 : intMaxAmplitude;
		scaleFactor = static_cast<double>((integerWriteScalingStyle == Pow2Minus1) ? intMaxAmplitude - 1 : intMaxAmplitude);
		std::cout << "csv output: number of bits: " << numBits << ", scaleFactor: " << scaleFactor;
		std::cout << ", integer output range: "
			<< unsignedOffset + std::max(-intMaxAmplitude, static_cast<int>(std::round(scaleFactor * -1.0)))
			<< " to "
			<< unsignedOffset +  std::min(intMaxAmplitude - 1, static_cast<int>(std::round(scaleFactor * 1.0)))
			<< std::endl;
		CsvFile::numBits = numBits;
		setStreamFormat();
	}

	int getPrecision() const {
		return precision;
	}

	void setPrecision(int precision) {
		CsvFile::precision = precision;
		setStreamFormat();
	}

	IntegerWriteScalingStyle getIntegerWriteScalingStyle() const {
		return integerWriteScalingStyle;
	}

	void setIntegerWriteScalingStyle(IntegerWriteScalingStyle integerWriteScalingStyle) {
		CsvFile::integerWriteScalingStyle = integerWriteScalingStyle;
	}

	int getNumChannels() const {
		return numChannels;
	}

	void setNumChannels(int numChannels) {
		CsvFile::numChannels = numChannels;
	}

};

} // namespace ReSampler

#endif //RESAMPLER_CSV_H
