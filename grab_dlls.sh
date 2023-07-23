#!/usr/bin/env bash

# copy required runtime dlls to destination-folder

if [[ -z "$1" ]]; then
   echo "usage: $0 <destination-folder>"
   exit 1
fi

dest=$1
libsndfile_dll=../ReSampler/libsndfile-1.2.0-win64/bin/sndfile.dll
libfftw_dll=../ReSampler/fftw64/libfftw3-3.dll

echo "copying c++ runtime dlls from ${MINGW_PREFIX}/bin"
cp ${MINGW_PREFIX}/bin/libstdc++-6.dll $dest
cp ${MINGW_PREFIX}/bin/libwinpthread-1.dll $dest
cp ${MINGW_PREFIX}/bin/libgcc_s_seh-1.dll $dest

echo "copying libsndfile dll from ${libsndfile_dll}"
cp ${libsndfile_dll} $dest

echo "copying fftw dll from ${libfftw_dll}"
cp ${libfftw_dll} $dest
