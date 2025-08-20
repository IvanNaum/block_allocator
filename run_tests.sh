#!/bin/bash

# Script for CI

set -e

BUILD_DIR="build"
rm -rf "${BUILD_DIR}"
mkdir "${BUILD_DIR}"
cd "${BUILD_DIR}"
cmake -DBUILD_TESTS=ON .. 
cd ..
cmake --build "./${BUILD_DIR}"

cd "${BUILD_DIR}"
./tests/test_allocator 
