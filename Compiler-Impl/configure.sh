#!/bin/bash
set -e

export base=`pwd`

scripts/get_llvm_src_tree.sh
scripts/install-shadowstack-files.sh


mkdir -p debug-build
mkdir -p release-build

cd debug-build
../scripts/cmake_debug.sh

cd ../release-build
../scripts/cmake_release.sh

#cd ../musl
#CC=$base/debug-install/bin/clang ./configure --enable-debug --disable-shared --prefix=$base/debug-install
