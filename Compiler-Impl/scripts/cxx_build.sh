#!/bin/bash

set -x

#cd ..
rm -rf libcxx-build libcxxabi-build libunwind-build

#order matters here
mkdir libunwind-build
cd libunwind-build
../scripts/libunwind_cmake.sh
ninja
ninja install
cd ..

mkdir libcxxabi-build
cd libcxxabi-build
../scripts/libcxxabi_cmake.sh
ninja
ninja install
cd ..

mkdir libcxx-build
cd libcxx-build
../scripts/libcxx_cmake.sh
ninja
ninja install
cd ..

set +x
