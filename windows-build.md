## Building ReSampler on Windows

### using Visual Studio
- Just open the project file (ReSampler.vcxproj) in Visual Studio
- Choose the configuration you want (eg Release x64)
- BUILD !!

### using minGW-W64 (with raw compiler invocations) under Windows (to build 64-bit .exe):
- use MinGW-W64 with Posix Threads and SEH (*tip: install [Git for Windows](https://github.com/git-for-windows) and use Git Bash!*)
- {...}\MinGW-W64\mingw64\bin must be added to Path
- unlike *nix systems, there is no standard place to put libraries and include files
- locations of include files can be specified by -Idir
- locations of library files can be specified by -Ldir
- if libraries are .lib (instead of .a), use -llib&lt;name&gt; instead of -l&lt;name&gt; 
- launch from git bash
- depends on these 64-bit dlls: **libfftw3-3.dll  libgcc_s_seh-1.dll  libsndfile-1.dll  libstdc++-6.dll  libwinpthread-1.dll**

standard 64-bit build:
~~~
g++ -pthread -std=c++11 main.cpp ReSampler.cpp conversioninfo.cpp -Ilibsndfile/include -Ifftw64 -Lfftw64 -llibfftw3-3 -Llibsndfile/lib -llibsndfile-1 -o x64/minGW-W64/ReSampler.exe -O3
~~~

AVX build:
~~~
g++ -pthread -std=c++11 main.cpp ReSampler.cpp conversioninfo.cpp -Ilibsndfile/include -Ifftw64 -Lfftw64 -llibfftw3-3 -Llibsndfile/lib -llibsndfile-1 -o x64/minGW-W64-AVX/ReSampler.exe -O3 -DUSE_AVX -mavx
~~~

AVX + FMA build (requires >= Intel Haswell or AMD PileDriver):
~~~
g++ -pthread -std=c++11 main.cpp ReSampler.cpp conversioninfo.cpp -Ilibsndfile/include -Ifftw64 -Lfftw64 -llibfftw3-3 -Llibsndfile/lib -llibsndfile-1 -o x64/minGW-W64-AVX/ReSampler.exe -O3 -DUSE_AVX -DUSE_FMA -mavx -mfma
~~~

Quad-Precision build (experimental) - GCC / minGW only:
~~~
 g++ -pthread -std=gnu++11 main.cpp ReSampler.cpp conversioninfo.cpp -Ilibsndfile/include -Ifftw64 -Lfftw64 -llibfftw3-3 -Llibsndfile/lib -llibsndfile-1 -lquadmath -o x64/minGW-W64/ReSampler-QuadMath.exe -O3 -DUSE_QUADMATH
~~~

*note the use of **-std=gnu++11** to enable the gcc-specific language extensions for quad precision - the 'Q' initializer suffix and __float128 type*

### Using CMake

*for IDEs that support CMake, it is usually very easy to simply open the CMakeLists file and start building*

#### building in Windows using QtCreator
*even though this project doesn't use any Qt libraries, Qt Creator is nevertheless a good IDE for building this project*  
- open a new project in Qt Creator by choosing **File -> "Open File or Project ..."**, then select the **CMakeLists.txt** file as the project file
- configure the kit of your choice (Note: as of Qt 5.12, 64-bit mingw is available for windows)
- copy dll files (libsndfile etc) files into the build folder. (Build directory can be seen using **QtCreator -> Projects -> Build -> Build Directory**

#### building using CLion
open the CMakeLists.txt in the ReSampler directory as a Project

#### building using CMake from command line

generally, the process goes like this:
~~~
cd path-to-where-you-want-the-binary
cmake -DCMAKE_BUILD_TYPE=Release path-to-ReSampler -G "MinGW Makefiles"
mingw32-make.exe
~~~

*note: annoyingly, you can't run the cmake MinGW Makefiles generator in git-bash on Windows; run it in a command prompt instead*

#### Examples

create a single-config minGW project
~~~
cmake <ReSampler Source Path> -DCMAKE_BUILD_TYPE=Debug -G "MinGW Makefiles"
cmake <ReSampler Source Path> -DCMAKE_BUILD_TYPE=Release -G "MinGW Makefiles"
cmake <ReSampler Source Path> -DCMAKE_BUILD_TYPE=ReleaseAVX -G "MinGW Makefiles"
~~~

activate verbose output in make
~~~
cmake <ReSamplerPath> -DCMAKE_BUILD_TYPE=Release -DCMAKE_VERBOSE_MAKEFILE:BOOL=ON -G "MinGW Makefiles"
~~~

#### Using CMake to generate a multi-config Visual Studio Project

*note: the cmake-generated Visual Studio project is similar to (but not the same as) the one in this Repo, which was created manually*

~~~
cmake <ReSampler Source Path> -A x64
~~~

create a multi-config Visual Studio project using a specific generator
~~~
cmake <ReSampler Source Path> -G "Visual Studio 14 2015 Win64"
# or
cmake <ReSampler Source Path> -G "Visual Studio 14 2015" -A x64
~~~

*if you need more help with CMake (and believe me, it's perfectly normal if you do), I have collected some resources [here](https://gist.github.com/jniemann66/9d3a4f5d20c6710cf7a0bed10905af91)*
