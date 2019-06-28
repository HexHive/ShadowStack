cd ..
HS_HOME=$PWD
HS_SYSROOT=$HS_HOME/debug-install
mkdir -p libcxxabi-debug-build
cd libcxxabi-debug-build

cmake -GNinja -DLLVM_PATH=$HS_HOME/llvm \
 -DCMAKE_C_FLAGS="-O0 -v" \
 -DCMAKE_CXX_FLAGS="-O0 -v" \
 -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_C_COMPILER=$HOME/ShadowStack-MPK/Compiler-Impl/debug-install/bin/clang \
  -DCMAKE_CXX_COMPILER=$HOME/ShadowStack-MPK/Compiler-Impl/debug-install/bin/clang++  \
 -DCMAKE_INSTALL_PREFIX=$HS_SYSROOT \
 -DLIBCXXABI_LIBCXX_PATH=$HS_HOME/libcxx \
 -DLIBCXXABI_LIBCXX_INCLUDES=$HS_HOME/libcxx/include \
 -DLIBCXXABI_LIBUNWIND_PATH=$HS_HOME/libunwind \
 -DLIBCXXABI_LIBUNWIND_INCLUDES=$HS_HOME/libunwind/include \
 -DLIBCXXABI_USE_LLVM_UNWINDER=ON \
 -DLIBCXXABI_ENABLE_SHARED=ON \
 ../libcxxabi
