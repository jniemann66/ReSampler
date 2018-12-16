/*
* Copyright (C) 2016 - 2018 Judd Niemann - All Rights Reserved.
* You may use, distribute and modify this code under the
* terms of the GNU Lesser General Public License, version 2.1
*
* You should have received a copy of GNU Lesser General Public License v2.1
* with this file. If not, please refer to: https://github.com/jniemann66/ReSampler
*/

#ifndef RESAMPLER_CSV_H
#define RESAMPLER_CSV_H

//
// csv.h : defines module for exporting audio data as a csv file
// Created by judd on 11/12/18.
//

#include <iostream>
#include <cassert>
#include <cstdint>
#include <string>
#include <fstream>

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
	Scientific
};

enum CsvNumericBase {
	Binary = 2,
	Octal = 8,
	Decimal = 10,
	Hexadecimal = 16
};

class CsvFile {
public:
    CsvFile(const std::string& path, CsvOpenMode mode = csv_write) : path(path), mode(mode), signedness(Signed), numericBase(Decimal), numBits(16), numSignificantDigits(10)
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

    ~CsvFile() {
        if(file.is_open()) {
            file.close();
        }
    }

    template <typename T>
    int64_t write(const T* buffer, int64_t count) {
        int64_t i;
        for(i = 0; i < count; i++) {
            file << buffer[i];
            if(++currentChannel < numChannels) {
                file << ",";
            } else {
                file << "\r\n";
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
public:
    CsvNumericFormat getNumericFormat() const {
        return numericFormat;
    }

    void setNumericFormat(CsvNumericFormat numericFormat) {
        CsvFile::numericFormat = numericFormat;
    }

    CsvSignedness getSignedness() const {
        return signedness;
    }

    void setSignedness(CsvSignedness signedness) {
        CsvFile::signedness = signedness;
    }

    CsvNumericBase getNumericBase() const {
        return numericBase;
    }

    void setNumericBase(CsvNumericBase numericBase) {
        CsvFile::numericBase = numericBase;
    }

    int getNumBits() const {
        return numBits;
    }

    void setNumBits(int numBits) {
        CsvFile::numBits = numBits;
    }

    int getNumSignificantDigits() const {
        return numSignificantDigits;
    }

    void setNumSignificantDigits(int numSignificantDigits) {
        CsvFile::numSignificantDigits = numSignificantDigits;
    }

private:
    CsvSignedness signedness;
	CsvNumericBase numericBase;
	int numBits;
	int numSignificantDigits;
  
public:
    int getNumChannels() const {
        return numChannels;
    }

    void setNumChannels(int numChannels) {
        CsvFile::numChannels = numChannels;
    }

private:
    int currentChannel;
    bool err;
};


#endif //RESAMPLER_CSV_H
