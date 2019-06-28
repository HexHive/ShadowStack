cd ..
HS_HOME=$PWD
HS_SYSROOT=$HS_HOME/release-install
mkdir -p libcxxabi-release-build
cd libcxxabi-release-build

cmake -GNinja -DLLVM_PATH=$HS_HOME/llvm \
 -DCMAKE_C_FLAGS="-v" \
 -DCMAKE_CXX_FLAGS="-v" \
 -DCMAKE_BUILD_TYPE=Release \
 -DCMAKE_CXX_COMPILER=clang++ \
 -DCMAKE_C_COMPILER=clang \
 -DCMAKE_INSTALL_PREFIX=$HS_SYSROOT \
 -DLIBCXXABI_LIBCXX_PATH=$HS_HOME/libcxx \
 -DLIBCXXABI_LIBCXX_INCLUDES=$HS_HOME/libcxx/include \
 -DLIBCXXABI_LIBUNWIND_PATH=$HS_HOME/libunwind \
 -DLIBCXXABI_LIBUNWIND_INCLUDES=$HS_HOME/libunwind/include \
 -DLIBCXXABI_USE_LLVM_UNWINDER=ON \
 -DLIBCXXABI_ENABLE_SHARED=ON \
 ../libcxxabi
