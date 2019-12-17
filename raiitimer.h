/*
* Copyright (C) 2016 - 2019 Judd Niemann - All Rights Reserved.
* You may use, distribute and modify this code under the
* terms of the GNU Lesser General Public License, version 2.1
*
* You should have received a copy of GNU Lesser General Public License v2.1
* with this file. If not, please refer to: https://github.com/jniemann66/ReSampler
*/

#ifndef _RAIITIMER_H
#define _RAIITIMER_H 1

#include <iostream>
#include <iomanip>
#include <chrono>

// class RaiiTimer : starts a high-resolution timer upon construction and prints elapsed time to stdout upon destruction
// For convenience, a reference time value (in ms) for comparison may be provided using the parameter msComparison.

namespace ReSampler {

class RaiiTimer {
public:

	explicit RaiiTimer(double msComparison = 0.0) : msComparison(msComparison) {
		beginTimer = std::chrono::high_resolution_clock::now();    
	}

	~RaiiTimer() {
		endTimer = std::chrono::high_resolution_clock::now();
		auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTimer - beginTimer).count();
		std::cout << "Time=" << duration << " ms";
		if(msComparison != 0.0) {
			double relativeSpeed = msComparison / duration;
			auto ss = std::cout.precision();
			std::cout << " [" << std::setprecision(1) << relativeSpeed << "x]" << std::setprecision(
					static_cast<int>(ss));
		} 
		std::cout << "\n" << std::endl;
	}

private:
	std::chrono::time_point<std::chrono::high_resolution_clock> beginTimer;
	std::chrono::time_point<std::chrono::high_resolution_clock> endTimer;
	double msComparison;
};

} // namespace ReSampler

#endif // _RAIITIMER_H
