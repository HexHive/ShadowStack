# ShadowStack
LLVM Implementation of different ShadowStack schemes for x86_64

How to build & run:
```
cd Compiler-Impl
make
```

This will put compiler binaries in debug-build
To get a release build, do
`make release`


To run different versions of shadow stack, toggle the macros in `Compiler-Impl/src/ShadowStackFeatures.h`
