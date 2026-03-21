cmake_minimum_required(VERSION 3.11)
project(ReSampler C CXX)
find_package(Threads REQUIRED)
set(CMAKE_CXX_STANDARD 17)

add_compile_definitions(COMPILER_ID="${CMAKE_CXX_COMPILER_ID}" COMPILER_VERSION="${CMAKE_CXX_COMPILER_VERSION}")

set(SOURCE_FILES
	alignedmalloc.h
	biquad.h
	conversioninfo.h
	csv.h
	dff.h
	ditherer.h
	dsf.h
	FIRFilter.h
	fraction.h
	factorial.h
	noiseshape.h
	osspecific.h
	raiitimer.h
	ReSampler.h
	srconvert.h
	iqdemodulator.h
	mpxdecode.h
	effect.h
	effectchain.h
	stereoimager.h
	fadeeffect.h
	ReSampler.cpp
	conversioninfo.cpp
	signalgenerator.cpp
)

if(ANDROID)
	set(BUILD_SHARED_LIBS_SAVED "${BUILD_SHARED_LIBS}")
	set(BUILD_SHARED_LIBS OFF)
	add_subdirectory(android/src/fftw-3.3.8)
	include_directories(android/src/fftw-3.3.8/api)
	add_subdirectory(android/src/libsndfile)
	include_directories(${CMAKE_SOURCE_DIR}/android/src/libsndfile/src)
	set(BUILD_SHARED_LIBS "${BUILD_SHARED_LIBS_SAVED}")
	add_library(ReSampler SHARED ${SOURCE_FILES})
	target_link_libraries(ReSampler sndfile fftw3 log)

else()

	if(NOT CMAKE_CONFIGURATION_TYPES)
		# single-config build (build type decided by CMAKE_BUILD_TYPE)

		# for single-config builds, add additional configurations
		set(CMAKE_CXX_FLAGS_RELEASEAVX "${CMAKE_CXX_FLAGS_RELEASE_INIT} -DUSE_AVX -mavx2")
		set(CMAKE_CXX_FLAGS_RELEASEQUADMATH "${CMAKE_CXX_FLAGS_RELEASE_INIT} -DUSE_QUADMATH")

		if(NOT CMAKE_BUILD_TYPE)
			message(STATUS "build type not specified - using default")
			set(CMAKE_BUILD_TYPE "ReleaseAVX" CACHE STRING "Choose the build type" FORCE)
		endif()

		# Set the possible values of build type for cmake-gui
		set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo" "ReleaseAVX" "ReleaseQuadmath")

		message(STATUS "build type: '${CMAKE_BUILD_TYPE}'")
	endif()

	if(WIN32 OR CMAKE_CROSSCOMPILING)

		if(CMAKE_SIZEOF_VOID_P EQUAL 4)
			message(STATUS "Windows 32-bit")
			set(SNDFILE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/libsndfile-1.2.0-win32")
			set(FFTW_DIR "${CMAKE_CURRENT_SOURCE_DIR}/fftw32")
		else()
			message(STATUS "Windows 64-bit")
			set(SNDFILE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/libsndfile-1.2.0-win64")
			set(FFTW_DIR "${CMAKE_CURRENT_SOURCE_DIR}/fftw64")
		endif()

		message(STATUS "libsndfile dir: " ${SNDFILE_DIR})
		message(STATUS "fftw dir: " ${FFTW_DIR})

		include_directories(${SNDFILE_DIR}/include ${FFTW_DIR})
		link_directories(${SNDFILE_DIR}/lib ${FFTW_DIR})

		find_library(SNDFILE_LIBRARY
			NAMES sndfile
			HINTS ${SNDFILE_DIR}
		)

	message(STATUS "libsndfile library location: " ${SNDFILE_LIBRARY})

	add_library(ReSamplerLib ${SOURCE_FILES})

	if(MSVC)
		message(STATUS "Visual Studio")
		target_link_libraries(ReSamplerLib libsndfile.lib libfftw3-3.lib ${CMAKE_THREAD_LIBS_INIT})

		if(NOT CMAKE_SIZEOF_VOID_P EQUAL 4)
			# create additional 64-bit config: ReleaseAVX
			set(CMAKE_CONFIGURATION_TYPES "Debug;Release;ReleaseAVX" CACHE STRING "" FORCE)

			# add /arch:AVX and /DUSE_AVX flags for ReleaseAVX configuration
			set(CMAKE_CXX_FLAGS_RELEASEAVX ${CMAKE_CXX_FLAGS_RELEASE_INIT} " /arch:AVX /DUSE_AVX")

			# same linker flags
			set(CMAKE_EXE_LINKER_FLAGS_RELEASEAVX ${CMAKE_EXE_LINKER_FLAGS_RELEASE_INIT})
			set(CMAKE_SHARED_LINKER_FLAGS_RELEASEAVX ${CMAKE_SHARED_LINKER_FLAGS_RELEASE})
		endif()

	else()
		message(STATUS "mingw dll location is here: ${MINGW_PREFIX}/bin")
		if(CMAKE_BUILD_TYPE STREQUAL "ReleaseQuadmath")
			set(CMAKE_CXX_EXTENSIONS TRUE) # -std=gnu++11 instead of -std=c++11
			message(STATUS "Linking with Quadmath library")
			target_link_libraries(ReSamplerLib ${SNDFILE_LIBRARY} libfftw3-3 libquadmath ${CMAKE_THREAD_LIBS_INIT})
		else()
			target_link_libraries(ReSamplerLib ${SNDFILE_LIBRARY} libfftw3-3 ${CMAKE_THREAD_LIBS_INIT})
		endif()
	endif()

else()

	# Not Windows ...

	if(APPLE)
		if("${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "arm64")
			message("Building on Apple Silicon (ARM64)")
			# AVX/AVX2 are x86 extensions - remove them for ARM64 builds
			string(REPLACE "-DUSE_AVX" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
			string(REPLACE "-mavx2" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
			set(CMAKE_CXX_FLAGS_RELEASEAVX "${CMAKE_CXX_FLAGS_RELEASE_INIT}")
			# headers and libraries: this is where homebrew puts them on Arm Macs:
			include_directories(/opt/homebrew/include)
			link_directories(/opt/homebrew/lib)
		elseif("${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "x86_64")
			# This might be an Intel Mac, or an Apple Silicon Mac running CMake via Rosetta 2
			message("Building on Intel or via Rosetta (x86_64)")
			# since Mojave, cmake no longer finds these automatically:
			# (see https://stackoverflow.com/questions/54068035/linking-not-working-in-homebrews-cmake-since-mojave)
			include_directories(/usr/local/include)
			link_directories(/usr/local/lib)
		endif()
	endif()

	add_library(ReSamplerLib SHARED ${SOURCE_FILES})

	if(CMAKE_BUILD_TYPE STREQUAL "ReleaseQuadmath")

		# cmake >=3.19:
		# include(CheckSourceCompiles)
		# check_source_compiles(
		# 	"#include <quadmath.h>\nint main() {return quadmath_atan(1.0Q);}"
		# 	QUADMATH_AVAILABLE
		# )

		# Search for the quadmath header file
		include(CheckIncludeFile)
		check_include_file("quadmath.h" HAVE_QUADMATH_HEADER)

		# Search for the quadmath library
		find_library(HAVE_QUADMATH_LIBRARY quadmath) # 2026-01-16 : fails to detect

		if(HAVE_QUADMATH_HEADER)
			message(STATUS "Linking with Quadmath library")
			set(CMAKE_CXX_EXTENSIONS TRUE) # -std=gnu++11 instead of -std=c++11
			target_link_libraries(ReSamplerLib sndfile fftw3 quadmath ${CMAKE_THREAD_LIBS_INIT})
		else()
			message(STATUS "Quadmath is not available. (Try installing a package such as 'libquadmath-devel'")
			endif()

		else()
			target_link_libraries(ReSamplerLib sndfile fftw3 ${CMAKE_THREAD_LIBS_INIT})
		endif()

	endif()

	add_executable(ReSampler main.cpp)
	target_link_libraries(ReSampler ReSamplerLib)

	# Install the executable
	install(TARGETS ReSampler RUNTIME DESTINATION bin)

	# On Windows (cross-compile or native), bundle the required DLLs
	if(WIN32 OR CMAKE_CROSSCOMPILING)
		install(FILES
			"${FFTW_DIR}/libfftw3-3.dll"
			"${SNDFILE_DIR}/bin/sndfile.dll"
			DESTINATION bin
		)
	endif()

endif()

# -------------------------------------------------------------------------
# CPack packaging
# -------------------------------------------------------------------------

set(CPACK_PACKAGE_NAME "ReSampler")
set(CPACK_PACKAGE_VENDOR "jniemann66")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "High-quality audio sample rate converter")
set(CPACK_PACKAGE_VERSION "2.1.2")
set(CPACK_PACKAGE_VERSION_MAJOR "2")
set(CPACK_PACKAGE_VERSION_MINOR "1")
set(CPACK_PACKAGE_VERSION_PATCH "2")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "ReSampler")
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
	set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
endif()
# Note: add a LICENSE file (LGPL-2.1) to the project root to enable the
# license page in NSIS installers and embed it in DEB/RPM packages.

# --- Windows: NSIS installer ---
set(CPACK_NSIS_DISPLAY_NAME "ReSampler")
set(CPACK_NSIS_PACKAGE_NAME "ReSampler ${CPACK_PACKAGE_VERSION}")
set(CPACK_NSIS_INSTALL_ROOT "$PROGRAMFILES64")
set(CPACK_NSIS_MODIFY_PATH ON)   # offer to add install dir to PATH
set(CPACK_NSIS_CREATE_ICONS_EXTRA
    "CreateShortCut '$SMPROGRAMS\\\\$STARTMENU_FOLDER\\\\ReSampler.lnk' '$INSTDIR\\\\bin\\\\ReSampler.exe'")
set(CPACK_NSIS_DELETE_ICONS_EXTRA
    "Delete '$SMPROGRAMS\\\\$STARTMENU_FOLDER\\\\ReSampler.lnk'")

# --- Linux: DEB package ---
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "jniemann66")
set(CPACK_DEBIAN_PACKAGE_SECTION "sound")
set(CPACK_DEBIAN_PACKAGE_DEPENDS "libsndfile1 (>= 1.0), libfftw3-double3 (>= 3.3)")

# --- Linux: RPM package ---
set(CPACK_RPM_PACKAGE_LICENSE "LGPL-2.1")
set(CPACK_RPM_PACKAGE_GROUP "Applications/Multimedia")
set(CPACK_RPM_PACKAGE_REQUIRES "libsndfile >= 1.0, fftw >= 3.3")

# Default generators per platform
if(WIN32 OR CMAKE_CROSSCOMPILING)
	set(CPACK_GENERATOR "NSIS")
elseif(APPLE)
	set(CPACK_GENERATOR "ZIP")
else()
	set(CPACK_GENERATOR "DEB;RPM")
endif()

include(CPack)
