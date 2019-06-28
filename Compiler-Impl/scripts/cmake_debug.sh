#!/bin/sh
set -e
$base/scripts/libunwind_cmake.sh
echo "base = "$base
cd $base
INSTALL_PATH=$PWD/debug-install
mkdir -p $INSTALL_PATH
mkdir -p debug-build
cd debug-build

cmake -G "Ninja" -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_C_COMPILER=clang \
  -DCMAKE_CXX_COMPILER=clang++ \
  -DLLVM_ENABLE_ASSERTIONS=ON \
  -DLLVM_BUILD_TESTS=OFF \
  -DLLVM_BUILD_EXAMPLES=OFF \
  -DLLVM_INCLUDE_TESTS=OFF\
  -DLLVM_INCLUDE_EXAMPLES=OFF \
  -DLLVM_TARGETS_TO_BUILD="X86" \
  -DLLVM_ENABLE_LTO=OFF \
  -DLLVM_PARALLEL_LINK_JOBS="4" \
  -DCMAKE_C_FLAGS=-fstandalone-debug \
  -DCMAKE_CXX_FLAGS=-fstandalone-debug \
  -DBUILD_SHARED_LIBS=ON \
  -DCMAKE_INSTALL_PREFIX=$INSTALL_PATH \
  -DLLVM_BINUTILS_INCDIR=/usr/include \
  $base/llvm

#cmake --build .

