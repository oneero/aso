#!/bin/bash
BUILD_TYPE=${1:-Debug}

rm -rf build
cmake -B build -G Ninja \
      -DCMAKE_CXX_COMPILER=clang++ \
      -DCMAKE_BUILD_TYPE=$BUILD_TYPE

cmake --build build --clean-first

./build/aso
