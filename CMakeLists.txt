project(ReSampler C CXX)
cmake_minimum_required(VERSION 3.11)
find_package(Threads REQUIRED)
set(CMAKE_CXX_STANDARD 17)


add_compile_definitions(COMPILER_ID="${CMAKE_CXX_COMPILER_ID}" COMPILER_VERSION="${CMAKE_CXX_COMPILER_VERSION}")

if(ANDROID)
    set(BUILD_SHARED_LIBS_SAVED "${BUILD_SHARED_LIBS}")
    set(BUILD_SHARED_LIBS OFF)
    add_subdirectory (android/src/fftw-3.3.8)
    include_directories(android/src/fftw-3.3.8/api)
    add_subdirectory (android/src/libsndfile)
    include_directories($CMAKE_SOURCE_DIR/android/src/libsndfile/src)
    set(BUILD_SHARED_LIBS "${BUILD_SHARED_LIBS_SAVED}")

    set(SOURCE_FILES
        alignedmalloc.h
        biquad.h
        conversioninfo.h
        conversioninfo.cpp
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
        main.cpp
        ReSampler.cpp
        ReSampler.h
        srconvert.h
        iqdemodulator.h
        mpxdecode.h
        effect.h
        effectchain.h
        stereoimager.h
        fadeeffect.h
        )

    add_library(ReSampler SHARED ${SOURCE_FILES})
    target_link_libraries(ReSampler sndfile fftw3 log)

else()

    if(NOT CMAKE_CONFIGURATION_TYPES)
        # single-config build (build type decided by CMAKE_BUILD_TYPE )

        # for single-config builds, add additional configurations
        set (CMAKE_CXX_FLAGS_RELEASEAVX ${CMAKE_CXX_FLAGS_RELEASE_INIT} " -DUSE_AVX -mavx")
        set (CMAKE_CXX_FLAGS_RELEASEQUADMATH ${CMAKE_CXX_FLAGS_RELEASE_INIT} " -DUSE_QUADMATH")

        if(NOT CMAKE_BUILD_TYPE)
            message(STATUS "build type not specified - using default")
            set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the build type" FORCE)
        endif()

        # Set the possible values of build type for cmake-gui
        set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo" "ReleaseAVX" "ReleaseQuadmath")

        message(STATUS "build type: '${CMAKE_BUILD_TYPE}'")

    endif()

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
        ReSampler.cpp
        ReSampler.h
        srconvert.h
        conversioninfo.cpp
        iqdemodulator.h
        mpxdecode.h
        effect.h
        effectchain.h
        stereoimager.h
        fadeeffect.h
        )

    if (WIN32)

        message(STATUS "libsndfile library location: " ${SNDFILE_LIBRARY})

        if(CMAKE_SIZEOF_VOID_P EQUAL 4)
            message(STATUS "Windows 32-bit")
            set (SNDFILE_DIR libsndfile-1.2.0-win32)
            set (FFTW_DIR fftw32)
        else()
            message(STATUS "Windows 64-bit")
            set (SNDFILE_DIR libsndfile-1.2.0-win64)
            set (FFTW_DIR fftw64)
        endif()

        include_directories(${SNDFILE_DIR}/include ${FFTW_DIR})
        link_directories(${SNDFILE_DIR}/lib ${FFTW_DIR})
        find_library(SNDFILE_LIBRARY NAMES sndfile HINTS ${SNDFILE_DIR}/lib)

        add_library(ReSamplerLib ${SOURCE_FILES})

        if(MSVC)
            message(STATUS "Visual Studio")
            target_link_libraries(ReSamplerLib libsndfile.lib libfftw3-3.lib ${CMAKE_THREAD_LIBS_INIT})

            if(NOT CMAKE_SIZEOF_VOID_P EQUAL 4)
                # create additional 64-bit config: ReleaseAVX
                set (CMAKE_CONFIGURATION_TYPES "Debug;Release;ReleaseAVX" CACHE STRING "" FORCE)

                # add /arch:AVX and /DUSE_AVX flags for ReleaseAVX configuration
                set (CMAKE_CXX_FLAGS_RELEASEAVX ${CMAKE_CXX_FLAGS_RELEASE_INIT} " /arch:AVX /DUSE_AVX")

                # same linker flags
                set (CMAKE_EXE_LINKER_FLAGS_RELEASEAVX ${CMAKE_EXE_LINKER_FLAGS_RELEASE_INIT})
                set (CMAKE_SHARED_LINKER_FLAGS_RELEASEAVX ${CMAKE_SHARED_LINKER_FLAGS_RELEASE})

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

        #Not Windows ...

        if(APPLE)
            # since Mojave, cmake no longer finds these automatically:
            # (see https://stackoverflow.com/questions/54068035/linking-not-working-in-homebrews-cmake-since-mojave)
            include_directories(/usr/local/include)
            link_directories(/usr/local/lib)
        endif()

        add_library(ReSamplerLib SHARED ${SOURCE_FILES})
        if(CMAKE_BUILD_TYPE STREQUAL "ReleaseQuadmath")
            message(STATUS "Linking with Quadmath library")
            set(CMAKE_CXX_EXTENSIONS TRUE)  # -std=gnu++11 instead of -std=c++11
            target_link_libraries(ReSamplerLib sndfile fftw3 quadmath ${CMAKE_THREAD_LIBS_INIT})
        else()
            target_link_libraries(ReSamplerLib sndfile fftw3 ${CMAKE_THREAD_LIBS_INIT})
        endif()

    endif()

    add_executable(ReSampler main.cpp)
    target_link_libraries(ReSampler ReSamplerLib)

endif()
