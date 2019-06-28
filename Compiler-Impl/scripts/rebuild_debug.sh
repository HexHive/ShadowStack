#!/bin/bash
#rebuild debug version of clang
make
#rebuild release version of clang
ninja -C release-build install
#rebuild musl
make -C musl clean
make -C musl
make -C musl install

#rebuild debug version of libunwind
ninja -C libunwind-debug-build clean
ninja -C libunwind-debug-build install

ninja -C libcxxabi-debug-build clean 
ninja -C libcxx-debug-build clean

ninja -C libcxxabi-debug-build install
ninja -C libcxx-debug-build install
