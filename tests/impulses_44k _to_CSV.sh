#!/usr/bin/env bash

# note: ensure ReSampler in your PATH
resampler_path=ReSampler

# specify folder locations
input_path=./inputs
output_path=./outputs

# do specific things according to OS (if neccessary):
# function tolower(){
#     echo $1 | sed "y/ABCDEFGHIJKLMNOPQRSTUVWXYZ/abcdefghijklmnopqrstuvwxyz/"
# }
# os=`tolower $OSTYPE`
# if [ $os == 'cygwin' ] || [ $os == 'msys' ]
# then 
#     echo windows
# else
#     echo not-windows
# fi

# clear old outputs:
rm $output_path/*.*
rm $output_path/._*

# 32-bit float, double-precision sweeps
$resampler_path -i $input_path/44kHz_pulse@-3dB.wav -o $output_path/44kHz_pulse@-3dB-to22k-dp.csv -r 22050 --noPeakChunk --doubleprecision
$resampler_path -i $input_path/44kHz_pulse@-3dB.wav -o $output_path/44kHz_pulse@-3dB-to32k-dp.csv -r 32000 --noPeakChunk --doubleprecision --mt
$resampler_path -i $input_path/44kHz_pulse@-3dB.wav -o $output_path/44kHz_pulse@-3dB-to44k-dp.csv -r 44100 --noPeakChunk --doubleprecision
$resampler_path -i $input_path/44kHz_pulse@-3dB.wav -o $output_path/44kHz_pulse@-3dB-to48k-dp.csv -r 48000 --noPeakChunk --doubleprecision --mt
$resampler_path -i $input_path/44kHz_pulse@-3dB.wav -o $output_path/44kHz_pulse@-3dB-to88k-dp.csv -r 88200 --noPeakChunk --doubleprecision
$resampler_path -i $input_path/44kHz_pulse@-3dB.wav -o $output_path/44kHz_pulse@-3dB-to96k-dp.csv -r 96000 --noPeakChunk --doubleprecision --mt
$resampler_path -i $input_path/44kHz_pulse@-3dB.wav -o $output_path/44kHz_pulse@-3dB-to176k-dp.csv -r 176400 --noPeakChunk --doubleprecision
$resampler_path -i $input_path/44kHz_pulse@-3dB.wav -o $output_path/44kHz_pulse@-3dB-to192k-dp.csv -r 192000 --noPeakChunk --doubleprecision --mt

# make spectrograms
#./make-spectrums.sh
