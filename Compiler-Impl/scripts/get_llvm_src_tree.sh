#!/bin/bash
command -v curl >/dev/null 2>&1 || { sudo apt-get install curl; }
command -v wget >/dev/null 2>&1 || { sudo apt-get install wget; }
command -v cmake >/dev/null 2>&1 || { sudo apt-get install cmake; }
command -v ninja >/dev/null 2>&1 || { sudo apt-get install ninja-build; }
dpkg -l libgtest-dev>/dev/null 2>&1 || { yes | sudo apt-get install libgtest-dev; }
dpkg -l binutils-dev>/dev/null 2>&1 || { yes | sudo apt-get install binutils-dev; }

LLVM_COMMIT=20230f9b42887e7a3a13c81731656a9bfd90c93a
CLANG_COMMIT=81684cc58243b49af786438d33fcbbf12a596c56
COMPILER_RT_COMMIT=e27398661ef8ea118124519a2f97ec345fcc2d6a

#get LLVM
git clone https://github.com/llvm-mirror/llvm.git
cd llvm
git checkout $LLVM_COMMIT
cd ..

#get Clang
git clone https://github.com/llvm-mirror/clang.git
cd clang
git checkout $CLANG_COMMIT
cd ..

#get compiler-rt
git clone https://github.com/llvm-mirror/compiler-rt.git
cd compiler-rt
git checkout $COMPILER_RT_COMMIT
cd ..
patch -p7 -u < sanitizer_common.patch

#get libc++
wget --retry-connrefused --tries=100 releases.llvm.org/5.0.1/libcxx-5.0.1.src.tar.xz
tar -xf libcxx-5.0.1.src.tar.xz
mv libcxx-5.0.1.src libcxx
rm libcxx-5.0.1.src.tar.xz

#get libc++abi
wget --retry-connrefused --tries=100 releases.llvm.org/5.0.1/libcxxabi-5.0.1.src.tar.xz
tar -xf libcxxabi-5.0.1.src.tar.xz
mv libcxxabi-5.0.1.src libcxxabi
rm libcxxabi-5.0.1.src.tar.xz

#get libunwind
wget --retry-connrefused --tries=100 releases.llvm.org/5.0.1/libunwind-5.0.1.src.tar.xz
tar -xf libunwind-5.0.1.src.tar.xz
mv libunwind-5.0.1.src libunwind
rm libunwind-5.0.1.src.tar.xz

#get musl
wget --retry-connrefused --tries=100 https://www.musl-libc.org/releases/musl-1.1.16.tar.gz
tar -xf musl-1.1.16.tar.gz
mv musl-1.1.16 musl
rm musl-1.1.16.tar.gz

#Set up llvm, clang, compiler-rt for inline build (will musl / libcxx separately)
cd llvm/tools
ln -s ../../clang .
cd ../projects
ln -s ../../compiler-rt .
cd ..
cd ..

