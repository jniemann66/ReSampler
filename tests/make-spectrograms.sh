#!/usr/bin/env bash

numThreads=4

# notes on obtaining spectrogram utility:
# spectrogram tool available from http://www.mega-nerd.com/libsndfile/tools/#spectrogram
# ... and https://github.com/erikd/sndfile-tools/tree/master/src

# Windows binary for spectrogram.exe available here: https://hydrogenaud.io/index.php/topic,102495.0.html
# Windows dependencies: libcairo-2.dll libpng12-0.dll libsndfile-1.dll libfftw3-3.dll
# Ubuntu: sudo apt-get install sndfile-tools

# note: ensure sndspec or spectrogram (depending on which one you use) is in your PATH
spectrogram_tool="sndspec"

echo $(tput setaf 2)cleaning ./spectrograms folder ...$(tput setaf 7)
rm ./spectrograms/*.*

echo $(tput setaf 2)generating spectrograms ...$(tput setaf 1)

if [[ "$OSTYPE" == "darwin"* ]]; 
then 
    #----------#
    #  series  #
    #----------#
    find ./outputs -type f ! -name '*.png' ! -name '*.txt' -exec $spectrogram_tool --dyn-range=190 '{}' 1200 960 '{}'.png \;
else 
    #----------#
    # parallel #
    #----------#

    #cmdline for 'spectrogram' program:
    #find ./outputs -type f ! -name '*.png' ! -name '*.txt' ! -name '*.sd2' ! -name '*.raw' -print0 | xargs -i --null -n 1 -P $numThreads $spectrogram_tool --dyn-range=190 {} 1200 960 {}.png 

    #cmdline for 'sndspec' program:
    find ./outputs -type f ! -name '*.png' ! -name '*.txt' ! -name '*.sd2' ! -name '*.raw' -print0 | xargs -i --null -n 1 -P $numThreads $spectrogram_tool --dyn-range=190 {} -w 1200 -h 960 -o ./outputs 
fi

echo $(tput setaf 2)moving spectrograms to ./spectrograms folder$(tput setaf 7)
mv ./outputs/*.png ./spectrograms