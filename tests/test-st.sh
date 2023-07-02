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

# 32-bit float sweeps
$resampler_path -i $input_path/96khz_sweep-3dBFS_32f.wav -o $output_path/96khz_sweep-3dBFS_32f-to22k.wav -r 22050 --noPeakChunk
$resampler_path -i $input_path/96khz_sweep-3dBFS_32f.wav -o $output_path/96khz_sweep-3dBFS_32f-to32k.wav -r 32000 --noPeakChunk
$resampler_path -i $input_path/96khz_sweep-3dBFS_32f.wav -o $output_path/96khz_sweep-3dBFS_32f-to44k.wav -r 44100 --noPeakChunk
$resampler_path -i $input_path/96khz_sweep-3dBFS_32f.wav -o $output_path/96khz_sweep-3dBFS_32f-to48k.wav -r 48000 --noPeakChunk
$resampler_path -i $input_path/96khz_sweep-3dBFS_32f.wav -o $output_path/96khz_sweep-3dBFS_32f-to88k.wav -r 88200 --noPeakChunk
$resampler_path -i $input_path/96khz_sweep-3dBFS_32f.wav -o $output_path/96khz_sweep-3dBFS_32f-to96k.wav -r 96000 --noPeakChunk
$resampler_path -i $input_path/96khz_sweep-3dBFS_32f.wav -o $output_path/96khz_sweep-3dBFS_32f-to176k.wav -r 176400 --noPeakChunk
$resampler_path -i $input_path/96khz_sweep-3dBFS_32f.wav -o $output_path/96khz_sweep-3dBFS_32f-to192k.wav -r 192000 --noPeakChunk
# note: with floating-point formats, the PEAK chunk has a unique timestamp in it, which throws the MD5 checksums, hence --noPeakChunk 

# 32-bit float, double-precision sweeps
$resampler_path -i $input_path/96khz_sweep-3dBFS_32f.wav -o $output_path/96khz_sweep-3dBFS_32f-to22k-dp.wav -r 22050 --noPeakChunk --doubleprecision
$resampler_path -i $input_path/96khz_sweep-3dBFS_32f.wav -o $output_path/96khz_sweep-3dBFS_32f-to32k-dp.wav -r 32000 --noPeakChunk --doubleprecision
$resampler_path -i $input_path/96khz_sweep-3dBFS_32f.wav -o $output_path/96khz_sweep-3dBFS_32f-to44k-dp.wav -r 44100 --noPeakChunk --doubleprecision
$resampler_path -i $input_path/96khz_sweep-3dBFS_32f.wav -o $output_path/96khz_sweep-3dBFS_32f-to48k-dp.wav -r 48000 --noPeakChunk --doubleprecision
$resampler_path -i $input_path/96khz_sweep-3dBFS_32f.wav -o $output_path/96khz_sweep-3dBFS_32f-to88k-dp.wav -r 88200 --noPeakChunk --doubleprecision
$resampler_path -i $input_path/96khz_sweep-3dBFS_32f.wav -o $output_path/96khz_sweep-3dBFS_32f-to96k-dp.wav -r 96000 --noPeakChunk --doubleprecision
$resampler_path -i $input_path/96khz_sweep-3dBFS_32f.wav -o $output_path/96khz_sweep-3dBFS_32f-to176k-dp.wav -r 176400 --noPeakChunk --doubleprecision
$resampler_path -i $input_path/96khz_sweep-3dBFS_32f.wav -o $output_path/96khz_sweep-3dBFS_32f-to192k-dp.wav -r 192000 --noPeakChunk --doubleprecision

# 32-bit float, double-precision sweeps, steep LPF
$resampler_path -i $input_path/96khz_sweep-3dBFS_32f.wav -o $output_path/96khz_sweep-3dBFS_32f-to22k-dp-steep.wav -r 22050 --noPeakChunk --doubleprecision --steepLPF
$resampler_path -i $input_path/96khz_sweep-3dBFS_32f.wav -o $output_path/96khz_sweep-3dBFS_32f-to32k-dp-steep.wav -r 32000 --noPeakChunk --doubleprecision --steepLPF
$resampler_path -i $input_path/96khz_sweep-3dBFS_32f.wav -o $output_path/96khz_sweep-3dBFS_32f-to44k-dp-steep.wav -r 44100 --noPeakChunk --doubleprecision --steepLPF
$resampler_path -i $input_path/96khz_sweep-3dBFS_32f.wav -o $output_path/96khz_sweep-3dBFS_32f-to48k-dp-steep.wav -r 48000 --noPeakChunk --doubleprecision --steepLPF
$resampler_path -i $input_path/96khz_sweep-3dBFS_32f.wav -o $output_path/96khz_sweep-3dBFS_32f-to88k-dp-steep.wav -r 88200 --noPeakChunk --doubleprecision --steepLPF
$resampler_path -i $input_path/96khz_sweep-3dBFS_32f.wav -o $output_path/96khz_sweep-3dBFS_32f-to96k-dp-steep.wav -r 96000 --noPeakChunk --doubleprecision --steepLPF
$resampler_path -i $input_path/96khz_sweep-3dBFS_32f.wav -o $output_path/96khz_sweep-3dBFS_32f-to176k-dp-steep.wav -r 176400 --noPeakChunk --doubleprecision --steepLPF
$resampler_path -i $input_path/96khz_sweep-3dBFS_32f.wav -o $output_path/96khz_sweep-3dBFS_32f-to192k-dp-steep.wav -r 192000 --noPeakChunk --doubleprecision --steepLPF

# 32-bit float, double-precision sweeps, minimum Phase
$resampler_path -i $input_path/96khz_sweep-3dBFS_32f.wav -o $output_path/96khz_sweep-3dBFS_32f-to22k-minphase.wav -r 22050 --noPeakChunk --doubleprecision --minphase
$resampler_path -i $input_path/96khz_sweep-3dBFS_32f.wav -o $output_path/96khz_sweep-3dBFS_32f-to32k-minphase.wav -r 32000 --noPeakChunk --doubleprecision --minphase
$resampler_path -i $input_path/96khz_sweep-3dBFS_32f.wav -o $output_path/96khz_sweep-3dBFS_32f-to44k-minphase.wav -r 44100 --noPeakChunk --doubleprecision --minphase
$resampler_path -i $input_path/96khz_sweep-3dBFS_32f.wav -o $output_path/96khz_sweep-3dBFS_32f-to48k-minphase.wav -r 48000 --noPeakChunk --doubleprecision --minphase
$resampler_path -i $input_path/96khz_sweep-3dBFS_32f.wav -o $output_path/96khz_sweep-3dBFS_32f-to88k-minphase.wav -r 88200 --noPeakChunk --doubleprecision --minphase
$resampler_path -i $input_path/96khz_sweep-3dBFS_32f.wav -o $output_path/96khz_sweep-3dBFS_32f-to96k-minphase.wav -r 96000 --noPeakChunk --doubleprecision --minphase
$resampler_path -i $input_path/96khz_sweep-3dBFS_32f.wav -o $output_path/96khz_sweep-3dBFS_32f-to176k-minphase.wav -r 176400 --noPeakChunk --doubleprecision --minphase
$resampler_path -i $input_path/96khz_sweep-3dBFS_32f.wav -o $output_path/96khz_sweep-3dBFS_32f-to192k-minphase.wav -r 192000 --noPeakChunk --doubleprecision --minphase

# 16-bit sweeps, relaxed LPF
$resampler_path -i $input_path/96khz_sweep-3dBFS_32f.wav -o $output_path/96khz_sweep-3dBFS_16-to22k-relaxed.wav -r 22050 -b 16 --relaxedLPF
$resampler_path -i $input_path/96khz_sweep-3dBFS_32f.wav -o $output_path/96khz_sweep-3dBFS_16-to32k-relaxed.wav -r 32000 -b 16 --relaxedLPF
$resampler_path -i $input_path/96khz_sweep-3dBFS_32f.wav -o $output_path/96khz_sweep-3dBFS_16-to44k-relaxed.wav -r 44100 -b 16 --relaxedLPF
$resampler_path -i $input_path/96khz_sweep-3dBFS_32f.wav -o $output_path/96khz_sweep-3dBFS_16-to48k-relaxed.wav -r 48000 -b 16 --relaxedLPF
$resampler_path -i $input_path/96khz_sweep-3dBFS_32f.wav -o $output_path/96khz_sweep-3dBFS_16-to88k-relaxed.wav -r 88200 -b 16 --relaxedLPF
$resampler_path -i $input_path/96khz_sweep-3dBFS_32f.wav -o $output_path/96khz_sweep-3dBFS_16-to96k-relaxed.wav -r 96000 -b 16 --relaxedLPF
$resampler_path -i $input_path/96khz_sweep-3dBFS_32f.wav -o $output_path/96khz_sweep-3dBFS_16-to176k-relaxed.wav -r 176400 -b 16 --relaxedLPF
$resampler_path -i $input_path/96khz_sweep-3dBFS_32f.wav -o $output_path/96khz_sweep-3dBFS_16-to192k-relaxed.wav -r 192000 -b 16 --relaxedLPF

# 16-bit sweeps, dither
$resampler_path -i $input_path/96khz_sweep-3dBFS_32f.wav -o $output_path/96khz_sweep-3dBFS_16-to22k-dither.wav -r 22050 -b 16 --dither --seed 666
$resampler_path -i $input_path/96khz_sweep-3dBFS_32f.wav -o $output_path/96khz_sweep-3dBFS_16-to32k-dither.wav -r 32000 -b 16 --dither --seed 666
$resampler_path -i $input_path/96khz_sweep-3dBFS_32f.wav -o $output_path/96khz_sweep-3dBFS_16-to44k-dither.wav -r 44100 -b 16 --dither --seed 666
$resampler_path -i $input_path/96khz_sweep-3dBFS_32f.wav -o $output_path/96khz_sweep-3dBFS_16-to48k-dither.wav -r 48000 -b 16 --dither --seed 666
$resampler_path -i $input_path/96khz_sweep-3dBFS_32f.wav -o $output_path/96khz_sweep-3dBFS_16-to88k-dither.wav -r 88200 -b 16 --dither --seed 666
$resampler_path -i $input_path/96khz_sweep-3dBFS_32f.wav -o $output_path/96khz_sweep-3dBFS_16-to96k-dither.wav -r 96000 -b 16 --dither --seed 666
$resampler_path -i $input_path/96khz_sweep-3dBFS_32f.wav -o $output_path/96khz_sweep-3dBFS_16-to176k-dither.wav -r 176400 -b 16 --dither --seed 666
$resampler_path -i $input_path/96khz_sweep-3dBFS_32f.wav -o $output_path/96khz_sweep-3dBFS_16-to192k-dither.wav -r 192000 -b 16 --dither --seed 666

# test alternative formats
$resampler_path -i $input_path/guitar.flac -o $output_path/guitar-aiff.aiff -r 48000
$resampler_path -i $input_path/guitar.flac -o $output_path/guitar-au.au -r 48000
$resampler_path -i $input_path/guitar.flac -o $output_path/guitar-avr.avr -r 48000
$resampler_path -i $input_path/guitar.flac -o $output_path/guitar-caf.caf -r 48000
$resampler_path -i $input_path/guitar.flac -o $output_path/guitar-flac.flac -r 96000
$resampler_path -i $input_path/guitar.flac -o $output_path/guitar-mat.mat -r 48000
$resampler_path -i $input_path/guitar.flac -o $output_path/guitar-mpc.mpc -r 48000
$resampler_path -i $input_path/guitar.flac -o $output_path/guitar-oga.oga -r 48000
$resampler_path -i $input_path/guitar.flac -o $output_path/guitar-paf.paf -r 48000
$resampler_path -i $input_path/guitar.flac -o $output_path/guitar-pvf.pvf -r 48000
$resampler_path -i $input_path/guitar.flac -o $output_path/guitar-raw.raw -r 48000
$resampler_path -i $input_path/guitar.flac -o $output_path/guitar-sd2.sd2 -r 48000
$resampler_path -i $input_path/guitar.flac -o $output_path/guitar-sf.sf -r 48000
$resampler_path -i $input_path/guitar.flac -o $output_path/guitar-voc.voc -r 48000
$resampler_path -i $input_path/guitar.flac -o $output_path/guitar-rf64.rf64 -r 48000
$resampler_path -i $input_path/guitar.flac -o $output_path/guitar-w64.w64 -r 48000
$resampler_path -i $input_path/guitar.flac -o $output_path/guitar-wav.wav -r 48000

# mono-only formats
$resampler_path -i $input_path/guitar-mono.flac -o $output_path/guitar-htk.htk -r 48000
$resampler_path -i $input_path/guitar-mono.flac -o $output_path/guitar-iff.iff -r 48000
$resampler_path -i $input_path/guitar-mono.flac -o $output_path/guitar-sds.sds -r 48000
$resampler_path -i $input_path/guitar-mono.flac -o $output_path/guitar-wve.wve -r 48000
$resampler_path -i $input_path/guitar-mono.flac -o $output_path/guitar-xi.xi -r 48000

# make spectrograms
./make-spectrograms.sh

# check files against last results:
md5sum -c checklist.chk > results.txt

# generate new checksums:
md5sum $output_path/* > checklist.chk
