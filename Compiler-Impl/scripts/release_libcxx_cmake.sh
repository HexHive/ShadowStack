cd ..
HS_HOME=$PWD
HS_SYSROOT=$HS_HOME/release-install
mkdir -p libcxx-release-build
cd libcxx-release-build

cmake -GNinja -DLLVM_PATH=$HS_HOME/llvm \
 -DCMAKE_BUILD_TYPE=Release \
 -DCMAKE_CXX_COMPILER=clang++ \
 -DCMAKE_C_COMPILER=clang \
 -DCMAKE_CXX_FLAGS="-v" \
 -DCMAKE_C_FLAGS="-v"  \
 -DCMAKE_INSTALL_PREFIX=$HS_SYSROOT \
 -DLIBCXX_CXX_ABI_INCLUDE_PATHS=$HS_HOME/libcxxabi/include \
 -DLIBCXX_CXX_ABI=libcxxabi \
 -DLIBCXX_CXX_ABI_LIBRARY_PATH=$HS_SYSROOT/lib \
 -DLIBCXX_HAS_GCC_S_LIB=ON \
 -DLIBCXX_ENABLE_SHARED=ON \
 ../libcxx

