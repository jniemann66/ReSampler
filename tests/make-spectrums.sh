#!/usr/bin/env bash

numThreads=4

# notes on obtaining spectrogram utility:
# spectrogram tool available from http://www.mega-nerd.com/libsndfile/tools/#spectrogram
# ... and https://github.com/erikd/sndfile-tools/tree/master/src

# Windows binary for spectrogram.exe available here: https://hydrogenaud.io/index.php/topic,102495.0.html
# Windows dependencies: libcairo-2.dll libpng12-0.dll libsndfile-1.dll libfftw3-3.dll
# Ubuntu: sudo apt-get install sndfile-tools

function tolower(){
    echo $1 | sed "y/ABCDEFGHIJKLMNOPQRSTUVWXYZ/abcdefghijklmnopqrstuvwxyz/"
}

os=`tolower $OSTYPE`

# set spectrogram path according to OS:
if [ $os == 'cygwin' ] || [ $os == 'msys' ]
then 
    spectrogram_tool="sndspec"
else
    #*nix ...
    spectrogram_tool="sndspec"
fi

echo $(tput setaf 2)cleaning ./spectrums folder ...$(tput setaf 7)
rm ./spectrums/*.*

echo $(tput setaf 2)generating spectrums ...$(tput setaf 1)

if [[ "$OSTYPE" == "darwin"* ]]; 
then 
    #----------#
    #  series  #
    #----------#
    # sndspec.exe c:\Tests\impulse.wav --spectrum -t 0.0 1.0 -W rectangular -w 3840 -h 2160
    find ./outputs -type f ! -name '*.png' ! -name '*.txt' -exec $spectrogram_tool --dyn-range=190 '{}' 1200 960 '{}'.png \;
    
else 
    #----------#
    # parallel #
    #----------#

    #cmdline for 'spectrogram' program:
    #find ./outputs -type f ! -name '*.png' ! -name '*.txt' ! -name '*.sd2' ! -name '*.raw' -print0 | xargs -i --null -n 1 -P $numThreads $spectrogram_tool --dyn-range=190 {} 1200 960 {}.png 

    #cmdline for 'sndspec' program:
    find ./outputs -type f ! -name '*.png' ! -name '*.txt' ! -name '*.sd2' ! -name '*.raw' -print0 | xargs -i --null -n 1 -P $numThreads $spectrogram_tool {} --spectrum -t 0.0 1.0 -W rectangular -w 3840 -h 2160 -o ./outputs 
fi

echo $(tput setaf 2)moving spectrums to ./spectrums folder$(tput setaf 7)
mv ./outputs/*.png ./spectrums