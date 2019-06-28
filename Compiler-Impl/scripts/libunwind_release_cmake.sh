cd ..
HS_HOME=$PWD
HS_SYSROOT=$HS_HOME/release-install
HS_RT=$HS_SYSROOT/lib/clang/7.0.0/lib/linux
mkdir -p libunwind-release-build
cd libunwind-release-build

cmake -GNinja -DLLVM_PATH=$HS_HOME/llvm \
 -DCMAKE_BUILD_TYPE=Release \
 -DCMAKE_CXX_COMPILER=clang++ \
 -DCMAKE_C_COMPILER=clang \
 -DCMAKE_C_FLAGS="-v" \
 -DCMAKE_CXX_FLAGS="-v" \
 -DCMAKE_INSTALL_PREFIX=$HS_SYSROOT \
 -DLIBUNWIND_ENABLE_SHARED=ON \
 ../libunwind
 
