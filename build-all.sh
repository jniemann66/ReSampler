#!/bin/bash

# script to produce in-source builds for multiple compilers and build types
# (gcc, clang, mingw-w64 cross-compiler)
# run this from project src dir. 
# this is designed to only be run from Linux (not git bash on windows etc) 

# Check the operating system type using 'uname -s'
OS_TYPE=$(uname -s)
if [ "$OS_TYPE" = "Linux" ]; then
    echo "Running on Linux. Proceeding with script execution."
else
    echo "This script is intended for Linux only. Current OS is: $OS_TYPE"
    echo "Exiting."
    exit 1
fi

# Define build directories based on compiler choice
GCC_BUILD_DIR="build/gcc"
GCC_QUADMATH_BUILD_DIR="build/gcc_quadmath"
CLANG_BUILD_DIR="build/clang"

# Function to build with GCC
build_gcc() {
    echo "--- Building with GCC ---"
    mkdir -p "$GCC_BUILD_DIR"

    # Configure the project using GCC
    cmake . -B./"$GCC_BUILD_DIR" -DCMAKE_CXX_COMPILER=g++

    # Build the project
    cmake --build "$GCC_BUILD_DIR" #--verbose
}

# Function to build with GCC Quad-Precision
build_gcc_quadmath() {
    echo "--- Building with GCC (Quad Precision) ---"
    mkdir -p "$GCC_QUADMATH_BUILD_DIR"

    # Configure the project using GCC
    cmake . -B./"$GCC_QUADMATH_BUILD_DIR" -DCMAKE_BUILD_TYPE=ReleaseQuadmath -DCMAKE_CXX_COMPILER=g++

    # Build the project
    cmake --build "$GCC_QUADMATH_BUILD_DIR" #--verbose
}


# Function to build with Clang
build_clang() {
    echo "--- Building with Clang ---"
    mkdir -p "$CLANG_BUILD_DIR"

    # Configure the project using Clang
    cmake . -B./"$CLANG_BUILD_DIR" -DCMAKE_CXX_COMPILER=clang++

    # Build the project
    cmake --build "$CLANG_BUILD_DIR" #--verbose
}

build_gcc
build_clang
build_gcc_quadmath
