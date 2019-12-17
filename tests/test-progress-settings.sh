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
    #resampler_path=../x64/Release/ReSampler.exe
    #resampler_path="E:\Temp\ReSampler1.3.6\ReSampler.exe"
    resampler_path=../x64/minGW-W64/ReSampler.exe
else
    resampler_path=../ReSampler
fi

# clear old outputs:
rm $output_path/*.*
rm $output_path/._*

# 32-bit float, double-precision sweeps
$resampler_path -i $input_path/96khz_sweep-3dBFS_32f.wav -o $output_path/96khz_sweep-3dBFS_32f-to22k-dp.wav -r 22050 --noPeakChunk --doubleprecision --progress-updates 100
$resampler_path -i $input_path/96khz_sweep-3dBFS_32f.wav -o $output_path/96khz_sweep-3dBFS_32f-to32k-dp.wav -r 32000 --noPeakChunk --doubleprecision --mt --progress-updates 90
$resampler_path -i $input_path/96khz_sweep-3dBFS_32f.wav -o $output_path/96khz_sweep-3dBFS_32f-to44k-dp.wav -r 44100 --noPeakChunk --doubleprecision --progress-updates 80
$resampler_path -i $input_path/96khz_sweep-3dBFS_32f.wav -o $output_path/96khz_sweep-3dBFS_32f-to48k-dp.wav -r 48000 --noPeakChunk --doubleprecision --mt --progress-updates 70
$resampler_path -i $input_path/96khz_sweep-3dBFS_32f.wav -o $output_path/96khz_sweep-3dBFS_32f-to88k-dp.wav -r 88200 --noPeakChunk --doubleprecision --progress-updates 60
$resampler_path -i $input_path/96khz_sweep-3dBFS_32f.wav -o $output_path/96khz_sweep-3dBFS_32f-to96k-dp.wav -r 96000 --noPeakChunk --doubleprecision --mt --progress-updates 50
$resampler_path -i $input_path/96khz_sweep-3dBFS_32f.wav -o $output_path/96khz_sweep-3dBFS_32f-to176k-dp.wav -r 176400 --noPeakChunk --doubleprecision --progress-updates 40
$resampler_path -i $input_path/96khz_sweep-3dBFS_32f.wav -o $output_path/96khz_sweep-3dBFS_32f-to192k-dp.wav -r 192000 --noPeakChunk --doubleprecision --mt --progress-updates 30

# 32-bit float, double-precision sweeps, steep LPF
$resampler_path -i $input_path/96khz_sweep-3dBFS_32f.wav -o $output_path/96khz_sweep-3dBFS_32f-to22k-dp-steep.wav -r 22050 --noPeakChunk --doubleprecision --steepLPF --progress-updates 20
$resampler_path -i $input_path/96khz_sweep-3dBFS_32f.wav -o $output_path/96khz_sweep-3dBFS_32f-to32k-dp-steep.wav -r 32000 --noPeakChunk --doubleprecision --steepLPF --mt --progress-updates 10
$resampler_path -i $input_path/96khz_sweep-3dBFS_32f.wav -o $output_path/96khz_sweep-3dBFS_32f-to44k-dp-steep.wav -r 44100 --noPeakChunk --doubleprecision --steepLPF --progress-updates 5
$resampler_path -i $input_path/96khz_sweep-3dBFS_32f.wav -o $output_path/96khz_sweep-3dBFS_32f-to48k-dp-steep.wav -r 48000 --noPeakChunk --doubleprecision --steepLPF --mt --progress-updates 2
$resampler_path -i $input_path/96khz_sweep-3dBFS_32f.wav -o $output_path/96khz_sweep-3dBFS_32f-to88k-dp-steep.wav -r 88200 --noPeakChunk --doubleprecision --steepLPF --progress-updates 0
$resampler_path -i $input_path/96khz_sweep-3dBFS_32f.wav -o $output_path/96khz_sweep-3dBFS_32f-to96k-dp-steep.wav -r 96000 --noPeakChunk --doubleprecision --steepLPF --mt --progress-updates 0
$resampler_path -i $input_path/96khz_sweep-3dBFS_32f.wav -o $output_path/96khz_sweep-3dBFS_32f-to176k-dp-steep.wav -r 176400 --noPeakChunk --doubleprecision --steepLPF --progress-updates 0
$resampler_path -i $input_path/96khz_sweep-3dBFS_32f.wav -o $output_path/96khz_sweep-3dBFS_32f-to192k-dp-steep.wav -r 192000 --noPeakChunk --doubleprecision --steepLPF --mt --progress-updates 0

# 32-bit float, double-precision sweeps, relaxed LPF
$resampler_path -i $input_path/96khz_sweep-3dBFS_32f.wav -o $output_path/96khz_sweep-3dBFS_32f-to22k-dp-relaxed.wav -r 22050 --noPeakChunk --doubleprecision --relaxedLPF --progress-updates 1000
$resampler_path -i $input_path/96khz_sweep-3dBFS_32f.wav -o $output_path/96khz_sweep-3dBFS_32f-to32k-dp-relaxed.wav -r 32000 --noPeakChunk --doubleprecision --relaxedLPF --mt --progress-updates 1000
$resampler_path -i $input_path/96khz_sweep-3dBFS_32f.wav -o $output_path/96khz_sweep-3dBFS_32f-to44k-dp-relaxed.wav -r 44100 --noPeakChunk --doubleprecision --relaxedLPF --progress-updates -1
$resampler_path -i $input_path/96khz_sweep-3dBFS_32f.wav -o $output_path/96khz_sweep-3dBFS_32f-to48k-dp-relaxed.wav -r 48000 --noPeakChunk --doubleprecision --relaxedLPF --mt --progress-updates -1
$resampler_path -i $input_path/96khz_sweep-3dBFS_32f.wav -o $output_path/96khz_sweep-3dBFS_32f-to88k-dp-relaxed.wav -r 88200 --noPeakChunk --doubleprecision --relaxedLPF --progress-updates -40000000000
$resampler_path -i $input_path/96khz_sweep-3dBFS_32f.wav -o $output_path/96khz_sweep-3dBFS_32f-to96k-dp-relaxed.wav -r 96000 --noPeakChunk --doubleprecision --relaxedLPF --mt --progress-updates -40000000000
$resampler_path -i $input_path/96khz_sweep-3dBFS_32f.wav -o $output_path/96khz_sweep-3dBFS_32f-to176k-dp-relaxed.wav -r 176400 --noPeakChunk --doubleprecision --relaxedLPF --progress-updates -40000000000
$resampler_path -i $input_path/96khz_sweep-3dBFS_32f.wav -o $output_path/96khz_sweep-3dBFS_32f-to192k-dp-relaxed.wav -r 192000 --noPeakChunk --doubleprecision --relaxedLPF --mt --progress-updates -40000000000

# make spectrograms
./make-spectrograms.sh
