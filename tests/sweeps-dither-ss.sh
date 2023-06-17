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
$resampler_path -i $input_path/96khz_sweep-3dBFS_32f.wav -o $output_path/96khz_sweep-3dBFS_32f-to22k-dp.wav -r 22050 --noPeakChunk --doubleprecision --dither --singlestage -b 24
$resampler_path -i $input_path/96khz_sweep-3dBFS_32f.wav -o $output_path/96khz_sweep-3dBFS_32f-to32k-dp.wav -r 32000 --noPeakChunk --doubleprecision --dither --singlestage -b 24 --mt
$resampler_path -i $input_path/96khz_sweep-3dBFS_32f.wav -o $output_path/96khz_sweep-3dBFS_32f-to44k-dp.wav -r 44100 --noPeakChunk --doubleprecision --dither --singlestage -b 24
$resampler_path -i $input_path/96khz_sweep-3dBFS_32f.wav -o $output_path/96khz_sweep-3dBFS_32f-to48k-dp.wav -r 48000 --noPeakChunk --doubleprecision --dither --singlestage -b 24 --mt
$resampler_path -i $input_path/96khz_sweep-3dBFS_32f.wav -o $output_path/96khz_sweep-3dBFS_32f-to88k-dp.wav -r 88200 --noPeakChunk --doubleprecision --dither --singlestage -b 24
$resampler_path -i $input_path/96khz_sweep-3dBFS_32f.wav -o $output_path/96khz_sweep-3dBFS_32f-to96k-dp.wav -r 96000 --noPeakChunk --doubleprecision --dither --singlestage -b 24 --mt
$resampler_path -i $input_path/96khz_sweep-3dBFS_32f.wav -o $output_path/96khz_sweep-3dBFS_32f-to176k-dp.wav -r 176400 --noPeakChunk --doubleprecision --dither --singlestage -b 24
$resampler_path -i $input_path/96khz_sweep-3dBFS_32f.wav -o $output_path/96khz_sweep-3dBFS_32f-to192k-dp.wav -r 192000 --noPeakChunk --doubleprecision --dither --singlestage -b 24 --mt

# 32-bit float, double-precision sweeps, steep LPF
$resampler_path -i $input_path/96khz_sweep-3dBFS_32f.wav -o $output_path/96khz_sweep-3dBFS_32f-to22k-dp-steep.wav -r 22050 --noPeakChunk --doubleprecision --dither --singlestage -b 24 --steepLPF
$resampler_path -i $input_path/96khz_sweep-3dBFS_32f.wav -o $output_path/96khz_sweep-3dBFS_32f-to32k-dp-steep.wav -r 32000 --noPeakChunk --doubleprecision --dither --singlestage -b 24 --steepLPF --mt
$resampler_path -i $input_path/96khz_sweep-3dBFS_32f.wav -o $output_path/96khz_sweep-3dBFS_32f-to44k-dp-steep.wav -r 44100 --noPeakChunk --doubleprecision --dither --singlestage -b 24 --steepLPF
$resampler_path -i $input_path/96khz_sweep-3dBFS_32f.wav -o $output_path/96khz_sweep-3dBFS_32f-to48k-dp-steep.wav -r 48000 --noPeakChunk --doubleprecision --dither --singlestage -b 24 --steepLPF --mt
$resampler_path -i $input_path/96khz_sweep-3dBFS_32f.wav -o $output_path/96khz_sweep-3dBFS_32f-to88k-dp-steep.wav -r 88200 --noPeakChunk --doubleprecision --dither --singlestage -b 24 --steepLPF
$resampler_path -i $input_path/96khz_sweep-3dBFS_32f.wav -o $output_path/96khz_sweep-3dBFS_32f-to96k-dp-steep.wav -r 96000 --noPeakChunk --doubleprecision --dither --singlestage -b 24 --steepLPF --mt
$resampler_path -i $input_path/96khz_sweep-3dBFS_32f.wav -o $output_path/96khz_sweep-3dBFS_32f-to176k-dp-steep.wav -r 176400 --noPeakChunk --doubleprecision --dither --singlestage -b 24 --steepLPF
$resampler_path -i $input_path/96khz_sweep-3dBFS_32f.wav -o $output_path/96khz_sweep-3dBFS_32f-to192k-dp-steep.wav -r 192000 --noPeakChunk --doubleprecision --dither --singlestage -b 24 --steepLPF --mt

# 32-bit float, double-precision sweeps, relaxed LPF
$resampler_path -i $input_path/96khz_sweep-3dBFS_32f.wav -o $output_path/96khz_sweep-3dBFS_32f-to22k-dp-relaxed.wav -r 22050 --noPeakChunk --doubleprecision --dither --singlestage -b 24 --relaxedLPF
$resampler_path -i $input_path/96khz_sweep-3dBFS_32f.wav -o $output_path/96khz_sweep-3dBFS_32f-to32k-dp-relaxed.wav -r 32000 --noPeakChunk --doubleprecision --dither --singlestage -b 24 --relaxedLPF --mt
$resampler_path -i $input_path/96khz_sweep-3dBFS_32f.wav -o $output_path/96khz_sweep-3dBFS_32f-to44k-dp-relaxed.wav -r 44100 --noPeakChunk --doubleprecision --dither --singlestage -b 24 --relaxedLPF
$resampler_path -i $input_path/96khz_sweep-3dBFS_32f.wav -o $output_path/96khz_sweep-3dBFS_32f-to48k-dp-relaxed.wav -r 48000 --noPeakChunk --doubleprecision --dither --singlestage -b 24 --relaxedLPF --mt
$resampler_path -i $input_path/96khz_sweep-3dBFS_32f.wav -o $output_path/96khz_sweep-3dBFS_32f-to88k-dp-relaxed.wav -r 88200 --noPeakChunk --doubleprecision --dither --singlestage -b 24 --relaxedLPF
$resampler_path -i $input_path/96khz_sweep-3dBFS_32f.wav -o $output_path/96khz_sweep-3dBFS_32f-to96k-dp-relaxed.wav -r 96000 --noPeakChunk --doubleprecision --dither --singlestage -b 24 --relaxedLPF --mt
$resampler_path -i $input_path/96khz_sweep-3dBFS_32f.wav -o $output_path/96khz_sweep-3dBFS_32f-to176k-dp-relaxed.wav -r 176400 --noPeakChunk --doubleprecision --dither --singlestage -b 24 --relaxedLPF
$resampler_path -i $input_path/96khz_sweep-3dBFS_32f.wav -o $output_path/96khz_sweep-3dBFS_32f-to192k-dp-relaxed.wav -r 192000 --noPeakChunk --doubleprecision --dither --singlestage -b 24 --relaxedLPF --mt

# make spectrograms
./make-spectrograms.sh