/*
* Copyright (C) 2016 - 2026 Judd Niemann - All Rights Reserved.
* You may use, distribute and modify this code under the
* terms of the GNU Lesser General Public License, version 2.1
*
* You should have received a copy of GNU Lesser General Public License v2.1
* with this file. If not, please refer to: https://github.com/jniemann66/ReSampler
*/

// main.cpp : defines main entry point

#include <iostream>
#include <string>

#if defined(__ANDROID__)

// define COMPILING_ON_ANDROID macro first before including any user headers
#define COMPILING_ON_ANDROID

#ifdef __aarch64__
#define COMPILING_ON_ANDROID64
#endif

#include <android/log.h>

// https://gist.github.com/dzhioev/6127982
class androidbuf : public std::streambuf {
public:
	enum { bufsize = 1024 }; // ... or some other suitable buffer size
	androidbuf(const int log_priority, const char * log_tag) :LOG_PRIORITY(log_priority), LOG_TAG(log_tag) { this->setp(buffer, buffer + bufsize - 1); };
private:
	int overflow(int c) {
		if (c == traits_type::eof()) {
			*this->pptr() = traits_type::to_char_type(c);
			this->sbumpc();
		}
		return this->sync() ? traits_type::eof() : traits_type::not_eof(c);
	}

	int sync() {
		int rc = 0;
		if (this->pbase() != this->pptr()) {
			__android_log_print(LOG_PRIORITY, LOG_TAG, "%s", std::string(this->pbase(), this->pptr() - this->pbase()).c_str());
			rc = 0;
			this->setp(buffer, buffer + bufsize - 1);
		}
		return rc;
	}

	char buffer[bufsize];
	const char * LOG_TAG;
	const int LOG_PRIORITY;
};

void androidCleanup() {
	delete std::cout.rdbuf(0);
	delete std::cerr.rdbuf(0);
}

#endif // defined(__ANDROID__)

#include "ReSampler.h"
#include "mpxdecode.h"
#include "iqdemodulator.h"

int main(int argc, char * argv[])
{

#ifdef COMPILING_ON_ANDROID
	std::cout.rdbuf(new androidbuf(ANDROID_LOG_INFO, "ReSampler"));
	std::cerr.rdbuf(new androidbuf(ANDROID_LOG_ERROR, "ReSampler"));
#endif

	int result = ReSampler::runCommand(argc, argv);

#ifdef COMPILING_ON_ANDROID
	androidCleanup();
#endif

	return result;
}
