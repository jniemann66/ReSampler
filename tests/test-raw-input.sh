#!/usr/bin/env bash

input_path=./inputs
output_path=./outputs

function tolower(){
    echo $1 | sed "y/ABCDEFGHIJKLMNOPQRSTUVWXYZ/abcdefghijklmnopqrstuvwxyz/"
}

os=`tolower $OSTYPE`

# set converter path according to OS:
if [ $os == 'cygwin' ] || [ $os == 'msys' ]
then 
    #Windows ...
    resampler_path=ReSampler.exe
else
    resampler_path=../ReSampler
fi

# clear old outputs:
rm $output_path/*.*
rm $output_path/._*

# 32-bit float sweeps to raw pcm
$resampler_path -i $input_path/96khz_sweep-3dBFS_32f.wav -o $output_path/96khz_sweep-3dBFS_32f-to44k8.raw -r 44100 -b u8
$resampler_path -i $input_path/96khz_sweep-3dBFS_32f.wav -o $output_path/96khz_sweep-3dBFS_32f-to44k16.raw -r 44100 -b 16
$resampler_path -i $input_path/96khz_sweep-3dBFS_32f.wav -o $output_path/96khz_sweep-3dBFS_32f-to44k24.raw -r 44100 -b 24
$resampler_path -i $input_path/96khz_sweep-3dBFS_32f.wav -o $output_path/96khz_sweep-3dBFS_32f-to44k32.raw -r 44100 -b 32
$resampler_path -i $input_path/96khz_sweep-3dBFS_32f.wav -o $output_path/96khz_sweep-3dBFS_32f-to44k32f.raw -r 44100 -b 32f
$resampler_path -i $input_path/96khz_sweep-3dBFS_32f.wav -o $output_path/96khz_sweep-3dBFS_32f-to44k64f.raw -r 44100 -b 64f
$resampler_path -i $input_path/96khz_sweep-3dBFS_32f.wav -o $output_path/96khz_sweep-3dBFS_32f-to44kalaw.raw -r 44100 -b alaw
$resampler_path -i $input_path/96khz_sweep-3dBFS_32f.wav -o $output_path/96khz_sweep-3dBFS_32f-to44kulaw.raw -r 44100 -b ulaw
$resampler_path -i $input_path/96khz_sweep-3dBFS_32f.wav -o $output_path/96khz_sweep-3dBFS_32f-to44kgsm610.raw -r 44100 -b gsm610
$resampler_path -i $input_path/96khz_sweep-3dBFS_32f.wav -o $output_path/96khz_sweep-3dBFS_32f-to44kdwvw12.raw -r 44100 -b dwvw12
$resampler_path -i $input_path/96khz_sweep-3dBFS_32f.wav -o $output_path/96khz_sweep-3dBFS_32f-to44kdwvw16.raw -r 44100 -b dwvw16
$resampler_path -i $input_path/96khz_sweep-3dBFS_32f.wav -o $output_path/96khz_sweep-3dBFS_32f-to44kdwvw24.raw -r 44100 -b dwvw24
# stereo file to raw pcm
$resampler_path -i $input_path/guitar.flac -o $output_path/guitar.raw -r 44100 -b 16

# convert raw pcm files back to wav 
$resampler_path -o $output_path/96khz_sweep-3dBFS_44k8.wav -i $output_path/96khz_sweep-3dBFS_32f-to44k8.raw -r 44100 -b u8 --raw-input 44100 u8
$resampler_path -o $output_path/96khz_sweep-3dBFS_44k16.wav -i $output_path/96khz_sweep-3dBFS_32f-to44k16.raw -r 44100 -b 16 --raw-input 44100 16
$resampler_path -o $output_path/96khz_sweep-3dBFS_44k24.wav -i $output_path/96khz_sweep-3dBFS_32f-to44k24.raw -r 44100 -b 24 --raw-input 44100 24
$resampler_path -o $output_path/96khz_sweep-3dBFS_44k32.wav -i $output_path/96khz_sweep-3dBFS_32f-to44k32.raw -r 44100 -b 32 --raw-input 44100 32
$resampler_path -o $output_path/96khz_sweep-3dBFS_44k32f.wav -i $output_path/96khz_sweep-3dBFS_32f-to44k32f.raw -r 44100 -b 32f --raw-input 44100 32f
$resampler_path -o $output_path/96khz_sweep-3dBFS_44k64f.wav -i $output_path/96khz_sweep-3dBFS_32f-to44k64f.raw -r 44100 -b 64f --raw-input 44100 64f
$resampler_path -o $output_path/96khz_sweep-3dBFS_44kalaw.wav -i $output_path/96khz_sweep-3dBFS_32f-to44kalaw.raw -r 44100 -b alaw --raw-input 44100 alaw
$resampler_path -o $output_path/96khz_sweep-3dBFS_44kulaw.wav -i $output_path/96khz_sweep-3dBFS_32f-to44kulaw.raw -r 44100 -b ulaw --raw-input 44100 ulaw
# raw stereo file to wav
$resampler_path -o $output_path/guitar.wav -i $output_path/guitar.raw -r 44100 -b 16 --raw-input 44100 16 2