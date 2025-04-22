#!/bin/bash
set -e

mkdir -p build
cd build
cmake ..
cmake --build . -j $(nproc)

cd test
ctest --output-on-failure
# ctest -V