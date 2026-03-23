#!/bin/bash

# Script to build and optionally package ReSampler on macOS.
# Run this from the project source directory on a macOS host.

OS_TYPE=$(uname -s)
if [ "$OS_TYPE" = "Darwin" ]; then
    echo "Running on macOS. Proceeding with script execution."
else
    echo "This script is intended for macOS only. Current OS is: $OS_TYPE"
    echo "Exiting."
    exit 1
fi

CLANG_BUILD_DIR="build/clang"
PACKAGE_BUILD_DIR="build/package"

# Function to build with Clang (the standard compiler on macOS)
build_clang() {
    echo "--- Building with Clang ---"
    mkdir -p "$CLANG_BUILD_DIR"
    cmake . -B./"$CLANG_BUILD_DIR" -DCMAKE_CXX_COMPILER=clang++
    cmake --build "$CLANG_BUILD_DIR" #--verbose
}

# Function to package a macOS DMG
package_macos() {
    echo "--- Packaging macOS DMG ---"
    cmake . -B "$PACKAGE_BUILD_DIR" -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_COMPILER=clang++
    cmake --build "$PACKAGE_BUILD_DIR"
    cd "$PACKAGE_BUILD_DIR" && cpack -G DragNDrop && cd ../..
}

# Parse arguments
PACKAGE=false
for arg in "$@"; do
    case "$arg" in
        --package) PACKAGE=true ;;
        *) echo "Unknown argument: $arg"; exit 1 ;;
    esac
done

echo ""
build_clang

if [ "$PACKAGE" = true ]; then
    echo ""
    package_macos
fi
