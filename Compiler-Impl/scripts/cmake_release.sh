#!/bin/sh
set -e

$base/scripts/libunwind_release_cmake.sh
cd $base
INSTALL_PATH=$PWD/release-install
mkdir -p $INSTALL_PATH
mkdir -p release-build
cd release-build

cmake -G "Ninja" -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_C_COMPILER=/usr/bin/clang \
  -DCMAKE_CXX_COMPILER=/usr/bin/clang++ \
  -DLLVM_ENABLE_ASSERTIONS=OFF \
  -DLLVM_BUILD_TESTS=OFF \
  -DLLVM_BUILD_EXAMPLES=OFF \
  -DLLVM_INCLUDE_TESTS=OFF\
  -DLLVM_INCLUDE_EXAMPLES=OFF \
  -DLLVM_TARGETS_TO_BUILD="X86" \
  -DLLVM_PARALLEL_LINK_JOBS="4" \
  -DBUILD_SHARED_LIBS=OFF \
  -DLLVM_ENABLE_LTO=OFF \
  -DLLVM_ENABLE_LLD=ON \
  -DCMAKE_INSTALL_PREFIX=$INSTALL_PATH \
  -DLLVM_BINUTILS_INCDIR=/usr/include \
  $base/llvm

#cmake --build .

