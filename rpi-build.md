# building ReSampler for Raspbian on Raspberry Pi

Building on Raspbian consists of the following 4 steps:

- Install development tools
- Build and install fftw library
- Build and install libsndfile library
- Build and install ReSampler

#### build environment
~~~
sudo apt-get install build-essential
~~~

#### fftw build/install

~~~
#sudo apt-get install libfftw3-dev libfftw3-doc
~~~

#### libsndfile build/install

~~~
sudo apt-get install libsndfile-dev
~~~

## building ReSampler

#### using gcc

clone this repository to a local directory, and invoke gcc

~~~
g++ -pthread -std=c++11 main.cpp ReSampler.cpp conversioninfo.cpp -lfftw3 -lsndfile -o ReSampler -O3
~~~

using specific compiler options for Raspberry Pi 3:

*see [GCC compiler optimization for ARM-based systems](https://gist.github.com/fm4dd/c663217935dc17f0fc73c9c81b0aa845)*

~~~
g++ -pthread -std=c++11 main.cpp ReSampler.cpp conversioninfo.cpp -lfftw3 -lsndfile -o ReSampler -O3 -mcpu=cortex-a53 -mfloat-abi=hard -mfpu=neon-fp-armv8 -mneon-for-64bits
~~~

#### using CMake

~~~
mkdir some-directory
cd some-directory
cmake -DCMAKE_BUILD_TYPE=Release path-to-ReSampler-source
make
~~~