//
//  X86ShadowStackSeg.cpp
//  LLVMX86CodeGen
//
//  Created by Kelvin Zhang on 2/17/18.
//
#include "X86InstrBuilder.h"
#include "X86InstrInfo.h"
#include "X86MachineFunctionInfo.h"
#include "X86Subtarget.h"
#include "X86TargetMachine.h"
#include "llvm/ADT/SmallSet.h"
#include "llvm/Analysis/EHPersonalities.h"
#include "llvm/CodeGen/MachineFrameInfo.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/MachineModuleInfo.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"
#include "llvm/CodeGen/WinEHFuncInfo.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/Function.h"
#include "llvm/MC/MCAsmInfo.h"
#include "llvm/MC/MCSymbol.h"
#include "llvm/Support/Debug.h"
#include "llvm/Target/TargetOptions.h"
#include "ShadowStack/ShadowStackConstants.h"
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <iterator>


using namespace llvm;
extern cl::opt<bool> mpk;

namespace{
    class X86ShadowStackSeg : public MachineFunctionPass{
    public:
        static char ID;
        X86ShadowStackSeg() : MachineFunctionPass(ID){}
        StringRef getPassName() const override { 
			return "X86 Shadow Stack Segment Scheme"; }
        bool runOnMachineFunction(MachineFunction &MF) override;
    };
}

char X86ShadowStackSeg::ID = 0;

FunctionPass *llvm::createX86ShadowStackSeg() { return new X86ShadowStackSeg(); }

extern bool mayWriteMem(const MachineFunction &MF);
extern void WRPKRUI(MachineBasicBlock &MBB, const MachineBasicBlock::iterator &MBBI, const DebugLoc &DL, const TargetInstrInfo *TII, int imm);
extern void XORReg(MachineBasicBlock &MBB, const MachineBasicBlock::iterator &MBBI, const DebugLoc &DL, const TargetInstrInfo *TII, unsigned Reg);
extern void checkRA(MachineBasicBlock &MBB, MachineBasicBlock::iterator &MBBI, const TargetInstrInfo *TII);

static void emitSegmentProlog(MachineFunction &MF)
{
    const TargetInstrInfo *TII = MF.getSubtarget().getInstrInfo();
    const DebugLoc DL = MF.front().size() == 0 ? DebugLoc() :MF.front().front().getDebugLoc();
    MachineBasicBlock &MBB = MF.front();
    const MachineBasicBlock::iterator &MBBI = MBB.begin();

    unsigned raReg = X86::R10;
    if(mpk.getValue())
    {
        raReg = X86::RCX;
        //save ecx and edx
        BuildMI(MBB, MBBI, DL, TII->get(X86::MOV64rr), X86::R10).addReg(X86::RCX, RegState::Kill);
        BuildMI(MBB, MBBI, DL, TII->get(X86::MOV64rr), X86::R11).addReg(X86::RDX, RegState::Kill);

        //disable MPK protections
        WRPKRUI(MBB, MBBI, DL, TII, 0);
    }
    //get the return addr
    //mov RAX, [rsp]
    MachineInstrBuilder getRA = BuildMI(MBB, MBBI, DL, TII->get(X86::MOV64rm), X86::RAX);
    addDirectMem(getRA, X86::RSP);

    //mov raReg, fs[0]
    BuildMI(MBB, MBBI, DL, TII->get(X86::MOV64rm), raReg)
		.addReg(0)          //base
    	.addImm(1)          //scale
    	.addReg(0)          //index
    	.addImm(0)          //disp
    	.addReg(X86::GS);    //segment

    //mov [raReg], rax
    MachineInstrBuilder putRA = BuildMI(MBB, MBBI, DL, TII->get(X86::MOV64mr));
    addDirectMem(putRA, raReg).addReg(X86::RAX);
    
    //mov [raReg+8], rax
    MachineInstrBuilder putSP = BuildMI(MBB, MBBI, DL, TII->get(X86::MOV64mr));
    addRegOffset(putSP, raReg, false, 8).addReg(X86::RSP);

    //add raReg, 16
    BuildMI(MBB, MBBI, DL, TII->get(X86::ADD64ri8), raReg)
    .addReg(raReg)
    .addImm(MF.getDataLayout().getPointerSize() * 2);

    //update shadow stack pointer
    //mov fs:[0], raReg
    BuildMI(MBB, MBBI, DL, TII->get(X86::MOV64mr))
    .addReg(0)          //base
    .addImm(1)          //scale
    .addReg(0)          //index
    .addImm(0)          //disp
    .addReg(X86::GS)    //segment
    .addReg(raReg);  //raReg...

    if(mpk.getValue())
    {
        XORReg(MBB, MBBI, DL, TII, X86::ECX);
        //reenable protection
        BuildMI(MBB, MBBI, DL, TII->get(X86::MOV64ri), X86::RAX).addImm(WRITE_PROTECTION);
        BuildMI(MBB, MBBI, DL, TII->get(X86::WRPKRUr));

        //restore ecx and edx
        BuildMI(MBB, MBBI, DL, TII->get(X86::MOV64rr), X86::RCX).addReg(X86::R10, RegState::Kill);
        BuildMI(MBB, MBBI, DL, TII->get(X86::MOV64rr), X86::RDX).addReg(X86::R11, RegState::Kill);
    }
}



static void emitSegmentEpilog(MachineBasicBlock &MBB)
{
    MachineBasicBlock::iterator &MBBI = --MBB.end();

    const DebugLoc &DL = MBBI->getDebugLoc();
    const TargetInstrInfo *TII = MBB.getParent()->getSubtarget().getInstrInfo();
    const int PtrSize = MBB.getParent()->getDataLayout().getPointerSize();
    const MachineInstr &back = MBB.back();
    const bool isTailCall = MBB.back().isCall();

    //If tail call
    //Save RCX RDX, they might be used as parameters
    //If return site
    //Save RAX RDX, they might be used as return value
    unsigned Reg1 = isTailCall ? X86::RCX : X86::RAX;
    
    //If Tail call needs R11,R10
    if(back.readsRegister(X86::R11))
    {
        MachineInstrBuilder saveR11 = BuildMI(MBB, MBBI, DL, TII->get(X86::MOV64mr));
        addRegOffset(saveR11, X86::RSP, false, -16).addReg(X86::R11);
    }
    if(back.readsRegister(X86::R10))
    {
        MachineInstrBuilder saveR10 = BuildMI(MBB, MBBI, DL, TII->get(X86::MOV64mr));
        addRegOffset(saveR10, X86::RSP, false, -24).addReg(X86::R10);
    }
    
    //just pop the shadow stack.
    if(mpk.getValue())
    {
        if(MBB.back().readsRegister(X86::RAX))
        {
            BuildMI(MBB, MBBI, DL, TII->get(X86::PUSH64r), X86::RAX);
        }
        //If tail call
        //Save RCX RDX, they might be used as parameters
        //If return site
        //Save RAX RDX, they might be used as return value
        BuildMI(MBB, MBBI, DL, TII->get(X86::MOV64rr), X86::R10).addReg(Reg1);
        BuildMI(MBB, MBBI, DL, TII->get(X86::PUSH64r), X86::RDX);
        //disable MPK protections
        WRPKRUI(MBB, MBBI, DL, TII, 0);
    }
    //mov r11, gs:[0]
    BuildMI(MBB, MBBI, DL, TII->get(X86::MOV64rm), X86::R11)
    .addReg(0)          //base reg
    .addImm(1)          //scale
    .addReg(0)          //index
    .addImm(0)          //disp
    .addReg(X86::GS);   //segment

    //sub R11, 8
    BuildMI(MBB, MBBI, DL, TII->get(X86::SUB64ri8), X86::R11)
    .addReg(X86::R11)
    .addImm(PtrSize*2);

    //mov gs:[0], R11
    BuildMI(MBB, MBBI, DL, TII->get(X86::MOV64mr))
    .addReg(0)          //base reg
    .addImm(1)          //scale
    .addReg(0)          //index
    .addImm(0)          //disp
    .addReg(X86::GS)   //segment
    .addReg(X86::R11);

    //mov R11, [R11]
    MachineInstrBuilder getRA = BuildMI(MBB, MBBI, DL, TII->get(X86::MOV64rm), X86::R11);
    addDirectMem(getRA, X86::R11);

    if(mpk.getValue())
    {
        //reenable protection
        BuildMI(MBB, MBBI, DL, TII->get(X86::MOV64ri), X86::RAX).addImm(WRITE_PROTECTION);
        BuildMI(MBB, MBBI, DL, TII->get(X86::WRPKRUr));

        //restore RCX and RDX
        BuildMI(MBB, MBBI, DL, TII->get(X86::MOV64rr), Reg1).addReg(X86::R10, RegState::Kill);
        BuildMI(MBB, MBBI, DL, TII->get(X86::POP64r), X86::RDX);
        if(MBB.back().readsRegister(X86::RAX))
        {
            BuildMI(MBB, MBBI, DL, TII->get(X86::POP64r), X86::RAX);
        }
    }
    
    checkRA(MBB, MBBI, TII);
    //Restore R10, R11 if necessary
    if(back.readsRegister(X86::R11))
    {
        MachineInstrBuilder saveR11 = BuildMI(MBB, MBBI, DL, TII->get(X86::MOV64rm), X86::R11);
        addRegOffset(saveR11, X86::RSP, false, -16);
    }
    if(back.readsRegister(X86::R10))
    {
        MachineInstrBuilder saveR10 = BuildMI(MBB, MBBI, DL, TII->get(X86::MOV64rm), X86::R10);
        addRegOffset(saveR10, X86::RSP, false, -24);
    }
}

bool X86ShadowStackSeg::runOnMachineFunction(MachineFunction &MF)
{
    if(!mayWriteMem(MF))return false;
    emitSegmentProlog(MF);
    for (MachineBasicBlock &MBB : MF)
    {
        if (MBB.isReturnBlock())
        {
            emitSegmentEpilog(MBB);
        }
    }
    return true;
}
