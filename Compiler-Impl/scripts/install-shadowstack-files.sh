#!/bin/bash

#This script softlinks our modified files into the LLVM source tree

#Path to llvm source tree
llvm=`pwd`/llvm
clang=`pwd`/clang
src=`pwd`/src
runtime=`pwd`/compiler-rt
musl=`pwd`/musl
unwind=`pwd`/libunwind

#clang codegen
ccg=$clang/lib/CodeGen

#clang driver
cdriver=$clang/lib/Driver

#llvm include
llvminc=$llvm/include/llvm

#llvm codegen
lcg=$llvm/lib/CodeGen

#llvm IR
lir=$llvm/lib/IR

ltrans=$llvm/lib/Transforms

#compiler-rt directory
crtlib=$runtime/lib

rm -f $llvm/lib/Target/X86/X86.h
ln -s $src/X86.h $llvm/lib/Target/X86/X86.h


rm -f $llvm/lib/Target/X86/X86TargetMachine.cpp
ln -s $src/X86TargetMachine.cpp $llvm/lib/Target/X86/X86TargetMachine.cpp

ln -s $src/X86ShadowStackMem.cpp $llvm/lib/Target/X86/X86ShadowStackMem.cpp
ln -s $src/X86ShadowStackCon.cpp $llvm/lib/Target/X86/X86ShadowStackCon.cpp
ln -s $src/X86ShadowStackReg.cpp $llvm/lib/Target/X86/X86ShadowStackReg.cpp
ln -s $src/X86ShadowStackSeg.cpp $llvm/lib/Target/X86/X86ShadowStackSeg.cpp

ln -s $src/X86ShadowStackMPX.cpp $llvm/lib/Target/X86/X86ShadowStackMPX.cpp
ln -s $src/X86FunctionCountPass.cpp $llvm/lib/Target/X86/X86FunctionCountPass.cpp

rm -f $llvm/lib/Target/X86/X86RegisterInfo.cpp
ln -s $src/X86RegisterInfo.cpp $llvm/lib/Target/X86/

rm -f $llvm/lib/Target/X86/X86MCInstLower.cpp
ln -s $src/X86MCInstLower.cpp $llvm/lib/Target/X86/

rm -f $llvm/lib/Target/X86/CMakeLists.txt
ln -s $src/x86-CMakeLists.txt $llvm/lib/Target/X86/CMakeLists.txt

rm -f $crtlib/CMakeLists.txt
ln -s $src/crt-lib-CMakeLists.txt $crtlib/CMakeLists.txt

#rm -r $crtlib/shadowstack
ln -s $src/shadowstack $crtlib/shadowstack

rm $cdriver/ToolChains/CommonArgs.cpp
ln -s $src/CommonArgs.cpp $cdriver/ToolChains

mkdir -p $llvm/include/ShadowStack
ln -s $src/ShadowStackConstants.h $llvm/include/ShadowStack/ShadowStackConstants.h
ln -s $src/ShadowStackFeatures.h $llvm/include/ShadowStack/ShadowStackFeatures.h

rm -f $musl/src/setjmp/x86_64/longjmp.s
ln -s $src/longjmp.s $musl/src/setjmp/x86_64/

rm -f $unwind/src/UnwindRegistersRestore.S 
ln -s $src/UnwindRegistersRestore.S $unwind/src

rm -f $src/shadowstack/setjmp.s
rm -f $src/shadowstack/longjmp.s
ln -s ../setjmp.s $src/shadowstack
ln -s ../longjmp.s $src/shadowstack

#rm $ltrans/Instrumentation/CMakeLists.txt
#ln -s $src/llvm-Transforms-CMakeLists.txt $ltrans/Instrumentation/CMakeLists.txt
#
#rm $ccg/BackendUtil.cpp
#ln -s $src/BackendUtil.cpp $ccg/BackendUtil.cpp
#
#rm $ltrans/Instrumentation/OTITypeTreePass.cpp
#ln -s $src/pass/OTITypeTreePass.cpp $ltrans/Instrumentation
#
#rm $llvminc/InitializePasses.h
#ln -s $src/InitializePasses.h $llvminc
#
#rm $llvminc/LinkAllPasses.h
#ln -s $src/LinkAllPasses.h $llvminc
#
#rm $llvminc/Transforms/Instrumentation.h
#ln -s $src/Instrumentation.h $llvminc/Transforms
#
#rm $llvminc/Transforms/OTI
#ln -s $src/OTI $llvminc/Transforms
#
#rm $ccg/CGDecl.cpp
#ln -s $src/CGDecl.cpp $ccg
#
#rm $ccg/CGExprCXX.cpp
#ln -s $src/CGExprCXX.cpp $ccg
#
#rm $ccg/CGClass.cpp
#ln -s $src/CGClass.cpp $ccg
#
#rm $ccg/CodeGenFunction.h
#ln -s $src/CodeGenFunction.h $ccg
#
#rm $ccg/ItaniumCXXABI.cpp
#ln -s $src/ItaniumCXXABI.cpp $ccg
#
#rm $ccg/CodeGenModule.h
#ln -s $src/CodeGenModule.h $ccg
#
#rm $lcg/TargetPassConfig.cpp
#ln -s $src/TargetPassConfig.cpp $lcg
#

