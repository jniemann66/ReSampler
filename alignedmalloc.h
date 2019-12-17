/*
* Copyright (C) 2016 - 2019 Judd Niemann - All Rights Reserved.
* You may use, distribute and modify this code under the
* terms of the GNU Lesser General Public License, version 2.1
*
* You should have received a copy of GNU Lesser General Public License v2.1
* with this file. If not, please refer to: https://github.com/jniemann66/ReSampler
*/

// alignedmalloc.h : for portable dynamic allocation of aligned memory

// usage:
// aligned_malloc(size, alignment)
// alignment must be a power-of-2
// returns a pointer to aligned allocated memory (or zero if unsuccessful)

// explanation:
// _aligned_malloc() is Windows-specific
// other systems use posix_memalign()
// the function signatures also differ

#ifndef alignedmalloc_H
#define alignedmalloc_H

#include <cstddef>

#ifdef _WIN32
	#include <malloc.h>
#else
	#include <cstdlib>
#endif


inline void* aligned_malloc(size_t size, size_t alignment) {
	
	if (size == 0) {
		return nullptr;
	}
	
#ifdef _WIN32 
	return _aligned_malloc(size, alignment);
#else 
	void *memory;
	return posix_memalign(&memory, alignment, size) ? nullptr : memory; // (note: posix_memalign returns 0 if successful, non-zero error code if not)
#endif

}

inline void aligned_free(void *ptr) {
#ifdef _WIN32
	_aligned_free(ptr);
#else 
	free(ptr);
#endif
}

#endif // alignedmalloc_H
