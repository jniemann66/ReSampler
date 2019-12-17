# building ReSampler (tested on Ubuntu 16.04)

Building on linux is fairly straightforward and consists of the following 3 steps:

- Build and install fftw library
- Build and install libsndfile library
- Build and install ReSampler

## build environment
~~~
sudo apt-get install build-essential
~~~

## fftw build/install

#sudo apt-get install libfftw3-dev libfftw3-doc

[fftw source](http://www.fftw.org/fftw-3.3.6-pl2.tar.gz)

*extract it to projects folder, and cd to it*
~~~
./configure
make
sudo make install
~~~

## libsndfile build/install

[libsndfile 1.0.28](http://www.mega-nerd.com/libsndfile/files/libsndfile-1.0.28.tar.gz)

*extract it to projects folder, and cd to it*

~~~
sudo apt install autoconf autogen automake build-essential libasound2-dev \
    libflac-dev libogg-dev libtool libvorbis-dev pkg-config python

./configure --prefix=/usr    \
            --disable-static \
            --docdir=/usr/share/doc/libsndfile-1.0.28 &&
make
make check
sudo make install
~~~

## building ReSampler

clone this repository to a local directory, and use one of the following command lines to compile:

#### using gcc:

standard 64-bit build:
~~~
g++ -pthread -std=c++11 main.cpp ReSampler.cpp conversioninfo.cpp -lfftw3 -lsndfile -o ReSampler -O3
~~~

AVX Build:
~~~
g++ -pthread -std=c++11 main.cpp ReSampler.cpp conversioninfo.cpp -lfftw3 -lsndfile -o ReSampler -O3 -DUSE_AVX -mavx
~~~

AVX + FMA (>= Haswell, PileDriver):
~~~
g++ -pthread -std=c++11 main.cpp ReSampler.cpp conversioninfo.cpp -lfftw3 -lsndfile -o ReSampler -O3 -DUSE_AVX -DUSE_FMA -mavx -mfma
~~~

Quad Precision (experimental)
~~~
g++ -pthread -std=gnu++11 main.cpp ReSampler.cpp conversioninfo.cpp -lfftw3 -lsndfile -o ReSampler -O3 -lquadmath -DUSE_QUADMATH
~~~

#### using clang:
~~~
clang++ -pthread -std=c++11 main.cpp ReSampler.cpp conversioninfo.cpp -lfftw3 -lsndfile -o ReSampler-clang -O3
~~~

#### using cmake:
~~~
cd path-to-where-you-want-the-binary
cmake -DCMAKE_BUILD_TYPE=Release path-to-ReSampler
make
~~~

# misc tasks:

## setting up C++ environment in vscode

ensure that the vscode [c++ tools](https://marketplace.visualstudio.com/items?itemName=ms-vscode.cpptools) are installed.

the g++ commands can be put into the **tasks.json** file. For example.:

~~~
{
    "version": "0.1.0",
    "command": "g++",
    "isShellCommand": true,
    "showOutput": "always",
    "args": [
        "-pthread",
        "-std=c++11",
        "main.cpp",
        "ReSampler.cpp",
        "conversioninfo.cpp",
        "-l",
        "fftw3",
        "-l",
        "sndfile",
        "-o",
        "ReSampler",
        "-O3",
        "-v"
    ]
}
~~~

[documentation](https://code.visualstudio.com/docs/languages/cpp)

## show where gcc is looking for header files:
~~~
`gcc -print-prog-name=cc1plus` -v
`gcc -print-prog-name=cc1` -v
~~~

## show where gcc is looking for libraries:
~~~
gcc -print-search-dirs
~~~

## display info about the binary you just built:
file ReSampler

*sample output:*

~~~
ReSampler: ELF 64-bit LSB executable, x86-64, version 1 (SYSV), dynamically linked, interpreter /lib64/ld-linux-x86-64.so.2, for GNU/Linux 2.6.32, BuildID[sha1]=2873279a9b0040a268f7c485de9027660ab3617c, not stripped
~~~
