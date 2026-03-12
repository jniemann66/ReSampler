#!/usr/bin/env bash

# new method of generating impulse responses, which doesn't require an input file

# note: ensure ReSampler in your PATH
resampler_path=ReSampler

# specify folder locations
output_path=./outputs

# clear old outputs:
rm $output_path/*.*
rm $output_path/._*

# 32-bit float, double-precision
$resampler_path --exportIR 44100 -o $output_path/44kHz_pulse@-3dB-to22k-dp.wav -r 22050 --noPeakChunk --doubleprecision
$resampler_path --exportIR 44100 -o $output_path/44kHz_pulse@-3dB-to32k-dp.wav -r 32000 --noPeakChunk --doubleprecision --mt
$resampler_path --exportIR 44100 -o $output_path/44kHz_pulse@-3dB-to44k-dp.wav -r 44100 --noPeakChunk --doubleprecision
$resampler_path --exportIR 44100 -o $output_path/44kHz_pulse@-3dB-to48k-dp.wav -r 48000 --noPeakChunk --doubleprecision --mt
$resampler_path --exportIR 44100 -o $output_path/44kHz_pulse@-3dB-to88k-dp.wav -r 88200 --noPeakChunk --doubleprecision
$resampler_path --exportIR 44100 -o $output_path/44kHz_pulse@-3dB-to96k-dp.wav -r 96000 --noPeakChunk --doubleprecision --mt
$resampler_path --exportIR 44100 -o $output_path/44kHz_pulse@-3dB-to176k-dp.wav -r 176400 --noPeakChunk --doubleprecision
$resampler_path --exportIR 44100 -o $output_path/44kHz_pulse@-3dB-to192k-dp.wav -r 192000 --noPeakChunk --doubleprecision --mt

# 32-bit float, double-precision, steep LPF
$resampler_path --exportIR 44100 -o $output_path/44kHz_pulse@-3dB-to22k-dp-steep.wav -r 22050 --noPeakChunk --doubleprecision --steepLPF
$resampler_path --exportIR 44100 -o $output_path/44kHz_pulse@-3dB-to32k-dp-steep.wav -r 32000 --noPeakChunk --doubleprecision --steepLPF --mt
$resampler_path --exportIR 44100 -o $output_path/44kHz_pulse@-3dB-to44k-dp-steep.wav -r 44100 --noPeakChunk --doubleprecision --steepLPF
$resampler_path --exportIR 44100 -o $output_path/44kHz_pulse@-3dB-to48k-dp-steep.wav -r 48000 --noPeakChunk --doubleprecision --steepLPF --mt
$resampler_path --exportIR 44100 -o $output_path/44kHz_pulse@-3dB-to88k-dp-steep.wav -r 88200 --noPeakChunk --doubleprecision --steepLPF
$resampler_path --exportIR 44100 -o $output_path/44kHz_pulse@-3dB-to96k-dp-steep.wav -r 96000 --noPeakChunk --doubleprecision --steepLPF --mt
$resampler_path --exportIR 44100 -o $output_path/44kHz_pulse@-3dB-to176k-dp-steep.wav -r 176400 --noPeakChunk --doubleprecision --steepLPF
$resampler_path --exportIR 44100 -o $output_path/44kHz_pulse@-3dB-to192k-dp-steep.wav -r 192000 --noPeakChunk --doubleprecision --steepLPF --mt

# 32-bit float, double-precision, relaxed LPF
$resampler_path --exportIR 44100 -o $output_path/44kHz_pulse@-3dB-to22k-dp-relaxed.wav -r 22050 --noPeakChunk --doubleprecision --relaxedLPF
$resampler_path --exportIR 44100 -o $output_path/44kHz_pulse@-3dB-to32k-dp-relaxed.wav -r 32000 --noPeakChunk --doubleprecision --relaxedLPF --mt
$resampler_path --exportIR 44100 -o $output_path/44kHz_pulse@-3dB-to44k-dp-relaxed.wav -r 44100 --noPeakChunk --doubleprecision --relaxedLPF
$resampler_path --exportIR 44100 -o $output_path/44kHz_pulse@-3dB-to48k-dp-relaxed.wav -r 48000 --noPeakChunk --doubleprecision --relaxedLPF --mt
$resampler_path --exportIR 44100 -o $output_path/44kHz_pulse@-3dB-to88k-dp-relaxed.wav -r 88200 --noPeakChunk --doubleprecision --relaxedLPF
$resampler_path --exportIR 44100 -o $output_path/44kHz_pulse@-3dB-to96k-dp-relaxed.wav -r 96000 --noPeakChunk --doubleprecision --relaxedLPF --mt
$resampler_path --exportIR 44100 -o $output_path/44kHz_pulse@-3dB-to176k-dp-relaxed.wav -r 176400 --noPeakChunk --doubleprecision --relaxedLPF
$resampler_path --exportIR 44100 -o $output_path/44kHz_pulse@-3dB-to192k-dp-relaxed.wav -r 192000 --noPeakChunk --doubleprecision --relaxedLPF --mt

# make spectrograms
./make-spectrums.sh
