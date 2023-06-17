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
$resampler_path -b u8x -i $input_path/96khz_sweep-3dBFS_32f.wav -o $output_path/96khz_sweep-3dBFS_32f-to22k-dp.csv -r 22050 --noPeakChunk --doubleprecision
$resampler_path -b u8x -i $input_path/96khz_sweep-3dBFS_32f.wav -o $output_path/96khz_sweep-3dBFS_32f-to32k-dp.csv -r 32000 --noPeakChunk --doubleprecision --mt
$resampler_path -b u8x -i $input_path/96khz_sweep-3dBFS_32f.wav -o $output_path/96khz_sweep-3dBFS_32f-to44k-dp.csv -r 44100 --noPeakChunk --doubleprecision
$resampler_path -b u8x -i $input_path/96khz_sweep-3dBFS_32f.wav -o $output_path/96khz_sweep-3dBFS_32f-to48k-dp.csv -r 48000 --noPeakChunk --doubleprecision --mt
$resampler_path -b u8x -i $input_path/96khz_sweep-3dBFS_32f.wav -o $output_path/96khz_sweep-3dBFS_32f-to88k-dp.csv -r 88200 --noPeakChunk --doubleprecision
$resampler_path -b u8x -i $input_path/96khz_sweep-3dBFS_32f.wav -o $output_path/96khz_sweep-3dBFS_32f-to96k-dp.csv -r 96000 --noPeakChunk --doubleprecision --mt
$resampler_path -b u8x -i $input_path/96khz_sweep-3dBFS_32f.wav -o $output_path/96khz_sweep-3dBFS_32f-to176k-dp.csv -r 176400 --noPeakChunk --doubleprecision
$resampler_path -b u8x -i $input_path/96khz_sweep-3dBFS_32f.wav -o $output_path/96khz_sweep-3dBFS_32f-to192k-dp.csv -r 192000 --noPeakChunk --doubleprecision --mt

# 32-bit float, double-precision sweeps, steep LPF
$resampler_path -b u8x -i $input_path/96khz_sweep-3dBFS_32f.wav -o $output_path/96khz_sweep-3dBFS_32f-to22k-dp-steep.csv -r 22050 --noPeakChunk --doubleprecision --steepLPF
$resampler_path -b u8x -i $input_path/96khz_sweep-3dBFS_32f.wav -o $output_path/96khz_sweep-3dBFS_32f-to32k-dp-steep.csv -r 32000 --noPeakChunk --doubleprecision --steepLPF --mt
$resampler_path -b u8x -i $input_path/96khz_sweep-3dBFS_32f.wav -o $output_path/96khz_sweep-3dBFS_32f-to44k-dp-steep.csv -r 44100 --noPeakChunk --doubleprecision --steepLPF
$resampler_path -b u8x -i $input_path/96khz_sweep-3dBFS_32f.wav -o $output_path/96khz_sweep-3dBFS_32f-to48k-dp-steep.csv -r 48000 --noPeakChunk --doubleprecision --steepLPF --mt
$resampler_path -b u8x -i $input_path/96khz_sweep-3dBFS_32f.wav -o $output_path/96khz_sweep-3dBFS_32f-to88k-dp-steep.csv -r 88200 --noPeakChunk --doubleprecision --steepLPF
$resampler_path -b u8x -i $input_path/96khz_sweep-3dBFS_32f.wav -o $output_path/96khz_sweep-3dBFS_32f-to96k-dp-steep.csv -r 96000 --noPeakChunk --doubleprecision --steepLPF --mt
$resampler_path -b u8x -i $input_path/96khz_sweep-3dBFS_32f.wav -o $output_path/96khz_sweep-3dBFS_32f-to176k-dp-steep.csv -r 176400 --noPeakChunk --doubleprecision --steepLPF
$resampler_path -b u8x -i $input_path/96khz_sweep-3dBFS_32f.wav -o $output_path/96khz_sweep-3dBFS_32f-to192k-dp-steep.csv -r 192000 --noPeakChunk --doubleprecision --steepLPF --mt

# 32-bit float, double-precision sweeps, relaxed LPF
$resampler_path -b u8x -i $input_path/96khz_sweep-3dBFS_32f.wav -o $output_path/96khz_sweep-3dBFS_32f-to22k-dp-relaxed.csv -r 22050 --noPeakChunk --doubleprecision --relaxedLPF
$resampler_path -b u8x -i $input_path/96khz_sweep-3dBFS_32f.wav -o $output_path/96khz_sweep-3dBFS_32f-to32k-dp-relaxed.csv -r 32000 --noPeakChunk --doubleprecision --relaxedLPF --mt
$resampler_path -b u8x -i $input_path/96khz_sweep-3dBFS_32f.wav -o $output_path/96khz_sweep-3dBFS_32f-to44k-dp-relaxed.csv -r 44100 --noPeakChunk --doubleprecision --relaxedLPF
$resampler_path -b u8x -i $input_path/96khz_sweep-3dBFS_32f.wav -o $output_path/96khz_sweep-3dBFS_32f-to48k-dp-relaxed.csv -r 48000 --noPeakChunk --doubleprecision --relaxedLPF --mt
$resampler_path -b u8x -i $input_path/96khz_sweep-3dBFS_32f.wav -o $output_path/96khz_sweep-3dBFS_32f-to88k-dp-relaxed.csv -r 88200 --noPeakChunk --doubleprecision --relaxedLPF
$resampler_path -b u8x -i $input_path/96khz_sweep-3dBFS_32f.wav -o $output_path/96khz_sweep-3dBFS_32f-to96k-dp-relaxed.csv -r 96000 --noPeakChunk --doubleprecision --relaxedLPF --mt
$resampler_path -b u8x -i $input_path/96khz_sweep-3dBFS_32f.wav -o $output_path/96khz_sweep-3dBFS_32f-to176k-dp-relaxed.csv -r 176400 --noPeakChunk --doubleprecision --relaxedLPF
$resampler_path -b u8x -i $input_path/96khz_sweep-3dBFS_32f.wav -o $output_path/96khz_sweep-3dBFS_32f-to192k-dp-relaxed.csv -r 192000 --noPeakChunk --doubleprecision --relaxedLPF --mt