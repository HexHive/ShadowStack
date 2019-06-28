echo $base
cd $base
HS_HOME=$base
HS_SYSROOT=$HS_HOME/debug-install
HS_RT=$HS_SYSROOT/lib/clang/7.0.0/lib/linux
mkdir -p libunwind-debug-build
cd libunwind-debug-build

cmake -G Ninja -DLLVM_PATH=$HS_HOME/llvm \
 -DCMAKE_BUILD_TYPE=Debug \
 -DCMAKE_CXX_COMPILER=clang++ \
 -DCMAKE_C_COMPILER=clang \
 -DCMAKE_C_FLAGS="-O0 -v" \
 -DCMAKE_CXX_FLAGS="-O0 -v" \
 -DCMAKE_INSTALL_PREFIX=$HS_SYSROOT \
 -DLIBUNWIND_ENABLE_SHARED=ON \
 ../libunwind
 
