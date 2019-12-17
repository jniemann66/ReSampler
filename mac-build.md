# Prerequisites

XCode

# building ReSampler (tested on macOS 10.12 - Sierra)

Building on Mac is fairly straightforward and consists of the following 3 steps:

- install fftw library
- install libsndfile library
- Build ReSampler


## install homebrew

~~~
ruby -e "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/master/install)" < /dev/null 2> /dev/null
~~~

## install fftw

~~~
brew install fftw
~~~

## install libsndfile

~~~
brew install libsndfile
~~~

## building ReSampler

clone this repository to a local directory, 

~~~
git clone https://github.com/jniemann66/ReSampler.git
~~~

and use one of the following command lines to compile:

#### using clang:
~~~
clang++ -pthread -std=c++11 main.cpp ReSampler.cpp conversioninfo.cpp -lfftw3 -lsndfile -o ReSampler-clang -O3 -L/usr/local/lib -I/usr/local/include
~~~

AVX Build:
~~~
clang++ -pthread -std=c++11 main.cpp ReSampler.cpp conversioninfo.cpp -lfftw3 -lsndfile -o ReSampler -O3 -DUSE_AVX -mavx -L/usr/local/lib -I/usr/local/include
~~~

AVX + FMA Build:
~~~
clang++ -pthread -std=c++11 main.cpp ReSampler.cpp conversioninfo.cpp -lfftw3 -lsndfile -o ReSampler -O3 -DUSE_AVX -DUSE_FMA -mavx -mfma -L/usr/local/lib -I/usr/local/include
~~~

#### using cmake:
~~~
cd path-to-where-you-want-the-binary
cmake -DCMAKE_BUILD_TYPE=Release path-to-ReSampler
make
~~~

# miscellaneous

## compiling sndfile-tools 

~~~
brew install autoconf
brew install automake
brew install libtool
brew install pkg-config
brew install libsamplerate
brew install Cairo

git clone https://github.com/erikd/sndfile-tools.git
cd sndfile-tools

./configure
make

cp sndfile-spectrogram /usr/local/bin
cp sndfile-waveform /usr/local/bin
cp sndfile-resample /usr/local/bin
cp sndfile-mix-to-mono /usr/local/bin
~~~
