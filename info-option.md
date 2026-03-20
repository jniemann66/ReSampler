# The --info option

The `--info` option displays header and metadata information for an input file, without performing any conversion.

## Usage

```
ReSampler -i <inputfile> --info
```

## Output

The following information is shown (where available):

- Format, sub-type, channels, sample rate, frame count, duration, and whether the file is seekable
- Standard metadata strings (title, artist, album, date, comment, etc.)
- Broadcast Extension (bext) chunk fields (description, originator, origination date/time, time reference, coding history)
- Loop info (time signature, BPM, root key)
- Instrument/sampler data (base note, velocity range, loop points)
- Cue points (index, sample offset, name)
- libsndfile's internal parse log, which shows the raw chunk structure of the file — particularly useful for formats such as CAF, which may contain format-specific chunks not exposed through the standard metadata API

## DSF files

For DSF files, the following information is shown:

- Format version and format ID
- Channel type (Mono, Stereo, Quad, 5.1, etc.)
- Channels, sample rate, bit order (LSB/MSB first), samples per channel, block size
- File size and duration
- Whether an ID3v2 metadata block is present (and its byte offset within the file)

## DFF (DSDIFF) files

For DFF files, the following information is shown:

- DSDIFF version
- Channels, with individual channel IDs (e.g. `SLFT`, `SRGT`, `MLFT`, `MRGT`, `LFE `)
- Sample rate, frame count, duration
- Compression type and name (e.g. `DSD ` / "not compressed")
- Loudspeaker configuration
- Absolute start time (HH:MM:SS + samples)
