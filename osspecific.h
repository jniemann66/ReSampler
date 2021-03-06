/*
* Copyright (C) 2016 - 2021 Judd Niemann - All Rights Reserved.
* You may use, distribute and modify this code under the
* terms of the GNU Lesser General Public License, version 2.1
*
* You should have received a copy of GNU Lesser General Public License v2.1
* with this file. If not, please refer to: https://github.com/jniemann66/ReSampler
*/

#ifndef OSSPECIFIC_H
#define OSSPECIFIC_H 1

// macros which address differences between compilers / operating systems go here.

#ifdef _WIN32
/* begin Windows-specific */

#if defined(_MSC_VER)
/* MSVC */
#define NOMINMAX // disable min() and max() macros (use std:: library instead)
#pragma warning(disable : 4996) // suppress pointless MS "deprecation" warnings
#pragma warning(disable : 4244) // suppress double-to-float warnings
#define BYTESWAP_METHOD_MSVCRT

#else
/* Not MSVC */
#define BYTESWAP_METHOD_BUILTIN // note : gcc >= 4.8.1 , clang >= 3.5
#endif

#define TEMPFILE_OPEN_METHOD_WINAPI
// Note: tmpfile() doesn't seem to work reliably with MSVC - probably related to this:
// http://www.mega-nerd.com/libsndfile/api.html#open_fd (see note regarding differing versions of MSVC runtime DLL)

#ifndef UNICODE
#define UNICODE // turns TCHAR into wchar_t
#endif 

#include <Windows.h>
#include <codecvt>
/* end Windows-Specific*/

#else 

/* begin Non-Windows */
#include <cstdint>
typedef uint64_t __int64;
#define stricmp strcasecmp
#define TEMPFILE_OPEN_METHOD_STD_TMPFILE
#define BYTESWAP_METHOD_BUILTIN
/* end non-Windows */

#endif

#endif // OSSPECIFIC_H
