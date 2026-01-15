#!/bin/bash

# script to produce in-source builds for both gcc AND clang
# (run this from project src dir)

# Define build directories based on compiler choice
GCC_BUILD_DIR="build/gcc"
CLANG_BUILD_DIR="build/clang"

# Function to build with GCC
build_gcc() {
    echo "--- Building with GCC ---"
    mkdir -p "$GCC_BUILD_DIR"

    # Configure the project using GCC
    cmake -B./"$GCC_BUILD_DIR" -DCMAKE_CXX_COMPILER=g++ .

    # Build the project
    cmake --build "$GCC_BUILD_DIR"
}

# Function to build with Clang
build_clang() {
    echo "--- Building with Clang ---"
    mkdir -p "$CLANG_BUILD_DIR"

    # Configure the project using Clang
    cmake -B./"$CLANG_BUILD_DIR" -DCMAKE_CXX_COMPILER=clang++ .

    # Build the project
    cmake --build "$CLANG_BUILD_DIR"
}

build_gcc
build_clang
