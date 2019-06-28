//
//  X86ShadowStackPass.cpp
//  LLVM
//
//  Created by Kelvin Zhang on 1/18/18.
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

extern cl::opt<bool> parallel;

extern cl::opt<bool> mpk;


namespace{
    class X86ShadowStackMem : public MachineFunctionPass{
    public:
        static char ID;
        X86ShadowStackMem() : MachineFunctionPass(ID){}
        StringRef getPassName() const override { 
			return "X86 Shadow Stack Memory Scheme"; }
        bool runOnMachineFunction(MachineFunction &MF) override;
    };
}
char X86ShadowStackMem::ID = 0;

FunctionPass *llvm::createX86ShadowStackMem() { return new X86ShadowStackMem(); }

extern bool mayWriteMem(const MachineFunction &MF);
extern void WRPKRUI(MachineBasicBlock &MBB, const MachineBasicBlock::iterator &MBBI, const DebugLoc &DL, const TargetInstrInfo *TII, int imm);
extern void XORReg(MachineBasicBlock &MBB, const MachineBasicBlock::iterator &MBBI, const DebugLoc &DL, const TargetInstrInfo *TII, unsigned Reg);
extern void checkRA(MachineBasicBlock &MBB, MachineBasicBlock::iterator &MBBI, const TargetInstrInfo *TII);
extern void checkRALBP(MachineBasicBlock &MBB, MachineBasicBlock::iterator &MBBI, const TargetInstrInfo *TII);
extern void checkRAJMP(MachineBasicBlock &MBB, MachineBasicBlock::iterator &MBBI, const TargetInstrInfo *TII);
extern void checkRASilence(MachineBasicBlock &MBB, MachineBasicBlock::iterator &MBBI, const TargetInstrInfo *TII);


static void emitShadowStackPrologue(MachineFunction &MF)
{
    const TargetInstrInfo *TII = MF.getSubtarget().getInstrInfo();
   
    const DebugLoc DL = MF.front().size() == 0 ? DebugLoc() :MF.front().front().getDebugLoc();
    
    //put all the shadow stack code in a separate MBB
    MachineBasicBlock &MBB = MF.front();
    const MachineBasicBlock::iterator &MBBI = MBB.begin();
    //save ecx and edx
    BuildMI(MBB, MBBI, DL, TII->get(X86::MOV64rr), X86::R10).addReg(X86::RCX, RegState::Kill);
    BuildMI(MBB, MBBI, DL, TII->get(X86::MOV64rr), X86::R11).addReg(X86::RDX, RegState::Kill);
    if(mpk.getValue())
    {
        //disable MPK protections
        WRPKRUI(MBB, MBBI, DL, TII, 0);
    }
    //get the return addr
    //mov RAX, [rsp]
    MachineInstrBuilder getRA = BuildMI(MBB, MBBI, DL, TII->get(X86::MOV64rm), X86::RAX);
    addRegOffset(getRA, X86::RSP, false, 0);
    
	//movabs rdx, SHADOW_STACK_PTR
	BuildMI(MBB, MBBI, DL, TII->get(X86::MOV64ri), X86::RDX).addImm((int64_t)SHADOW_STACK_PTR);
	//mov RCX, [RDX];
	MachineInstrBuilder getPtr = BuildMI(MBB, MBBI, DL, TII->get(X86::MOV64rm), X86::RCX);
	addDirectMem(getPtr, X86::RDX);


	//mov [RCX], RAX
	MachineInstrBuilder putRA = BuildMI(MBB, MBBI, DL, TII->get(X86::MOV64mr));
	addRegOffset(putRA, X86::RCX, false, 0);
	putRA.addReg(X86::RAX);
	//mov [RCX+8], RSP
	MachineInstrBuilder putRSP = BuildMI(MBB, MBBI, DL, TII->get(X86::MOV64mr));
	addRegOffset(putRSP, X86::RCX, false, 8).addReg(X86::RSP);
	

	//add [RDX], 8
	MachineInstrBuilder addPtr = BuildMI(MBB, MBBI, DL, TII->get(X86::ADD64mi8));
	addDirectMem(addPtr, X86::RDX);
	addPtr.addImm(16);
    
    if(mpk.getValue())
    {
        XORReg(MBB, MBBI, DL, TII, X86::RCX);
        XORReg(MBB, MBBI, DL, TII, X86::RDX);
        //reenable protection
        BuildMI(MBB, MBBI, DL, TII->get(X86::MOV64ri), X86::RAX).addImm(WRITE_PROTECTION);
        BuildMI(MBB, MBBI, DL, TII->get(X86::WRPKRUr));
    }
    //restore ecx and edx
    BuildMI(MBB, MBBI, DL, TII->get(X86::MOV64rr), X86::RCX).addReg(X86::R10, RegState::Kill);
    BuildMI(MBB, MBBI, DL, TII->get(X86::MOV64rr), X86::RDX).addReg(X86::R11, RegState::Kill);
}

static void emitShadowStackEpilogue(MachineBasicBlock &MBB)
{
    MachineBasicBlock::iterator &MBBI = --MBB.end();
    
    const DebugLoc &DL = MBBI->getDebugLoc();
    const TargetInstrInfo *TII = MBB.getParent()->getSubtarget().getInstrInfo();
    
    const MachineInstr &back = MBB.back();
    //If tail call needs to use R10, R11
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
    
	//save ret value in rax
	//mov r10, rax
	BuildMI(MBB, MBBI, DL, TII->get(X86::MOV64rr), X86::R10).addReg(X86::RAX);
    //save RCX, RDX
    BuildMI(MBB, MBBI, DL, TII->get(X86::PUSH64r)).addReg(X86::RDX);
    if(mpk.getValue())
    {
        BuildMI(MBB, MBBI, DL, TII->get(X86::PUSH64r)).addReg(X86::RCX);
        //disable protection
        WRPKRUI(MBB, MBBI, DL, TII, 0);
    }
	//mov rdx, SHADOW_STACK_PTR
	BuildMI(MBB, MBBI, DL, TII->get(X86::MOV64ri), X86::RDX).addImm((int64_t)SHADOW_STACK_PTR);
	//sub [rdx], 8
	MachineInstrBuilder subPtr = BuildMI(MBB, MBBI, DL, TII->get(X86::SUB64mi8));
	addDirectMem(subPtr, X86::RDX);
	subPtr.addImm(16);

	//r11 now points to top of shadow stack
	//mov r11, [rdx]
	MachineInstrBuilder getPtr = BuildMI(MBB, MBBI, DL, TII->get(X86::MOV64rm), X86::R11);
	addDirectMem(getPtr, X86::RDX);
	//R11 now contains RA
	//mov r11, [r11]
	MachineInstrBuilder getRA = BuildMI(MBB, MBBI, DL, TII->get(X86::MOV64rm), X86::R11);
	addDirectMem(getRA, X86::R11);
    if(mpk.getValue())
    {
        XORReg(MBB, MBBI, DL, TII, X86::RDX);
        //renable protection
        BuildMI(MBB, MBBI, DL, TII->get(X86::MOV64ri), X86::RAX).addImm(WRITE_PROTECTION);
        BuildMI(MBB, MBBI, DL, TII->get(X86::WRPKRUr));
        BuildMI(MBB, MBBI, DL, TII->get(X86::POP64r), X86::RCX);
    }
    //restore RCX, RDX
    BuildMI(MBB, MBBI, DL, TII->get(X86::POP64r), X86::RDX);
    
    
    //restore rax
    BuildMI(MBB, MBBI, DL, TII->get(X86::MOV64rr), X86::RAX).addReg(X86::R10);
    
    //check RA in r11
	checkRA(MBB, MBBI, TII);
    
    //restore R10, R11 if necessary
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


bool X86ShadowStackMem::runOnMachineFunction(MachineFunction &MF) {
    if(!mayWriteMem(MF))return false;
    emitShadowStackPrologue(MF);
    for (MachineBasicBlock &MBB : MF)
    {
        if (MBB.isReturnBlock())
        {
            emitShadowStackEpilogue(MBB);
        }
    }
    return true;
}
void checkRALBP(MachineBasicBlock &MBB, MachineBasicBlock::iterator &MBBI, const TargetInstrInfo *TII)
{
    DebugLoc DL = MBBI->getDebugLoc();
	if(!MBB.back().isCall())
	{
		//pop r10
		BuildMI(MBB, MBBI, DL, TII->get(X86::POP64r), X86::R10);
	}else
	{
		//mov r10, [rsp]
		addDirectMem(
			BuildMI(MBB, MBBI, DL, TII->get(X86::MOV64rm), X86::R10)
			, X86::RSP);
	}

	//xor r11, r10
	BuildMI(MBB, MBBI, DL, TII->get(X86::XOR64rr), X86::R11)
		.addReg(X86::R11)
		.addReg(X86::R10);

	//popcnt r11, r11
	BuildMI(MBB, MBBI, DL, TII->get(X86::POPCNT64rr), X86::R11)
		.addReg(X86::R11);

	//mov gs:[r11], 0
	BuildMI(MBB, MBBI, DL, TII->get(X86::MOV8rm), X86::R11B)
		.addReg(0)          //base
		.addImm(1)          //scale
		.addReg(X86::R11)          //index
		.addImm((int64_t)SHADOW_STACK_SENTINEL+4095)          //disp
		.addReg(0);    //segment


	if(!MBB.back().isCall())
	{
		BuildMI(MBB, MBBI, DL, TII->get(X86::JMP64r), X86::R10);
		//MBB.back().eraseFromParent();
	}
}

void checkRA(MachineBasicBlock &MBB, MachineBasicBlock::iterator &MBBI, const TargetInstrInfo *TII)
{
    DebugLoc DL = MBBI->getDebugLoc();
    //mov r10, rsp
    BuildMI(MBB, MBBI, DL, TII->get(X86::MOV64rr), X86::R10).addReg(X86::RSP);

	//xor r11, [rsp]
	MachineInstrBuilder cmp = BuildMI(MBB, MBBI, DL, TII->get(X86::XOR64rm), X86::R11).addReg(X86::R11);
	addDirectMem(cmp, X86::RSP);

	//mov rsp, 0
	BuildMI(MBB, MBBI, DL, TII->get(X86::MOV64ri), X86::RSP)
		.addImm(0);
	//cmove rsp, r10
	BuildMI(MBB, MBBI, DL, TII->get(X86::CMOVE64rr), X86::RSP)
		.addReg(X86::RSP)
		.addReg(X86::R10);
}

static void checkRANew(MachineBasicBlock &MBB, MachineBasicBlock::iterator &MBBI, const TargetInstrInfo *TII)
{
    DebugLoc DL = MBBI->getDebugLoc();
	addDirectMem(
		BuildMI(MBB, MBBI, DL, TII->get(X86::CMP64rm), X86::R11)
			, X86::RSP);
	BuildMI(MBB, MBBI, DL, TII->get(X86::CMOVNE64rm), X86::R11)
		.addReg(X86::R11)
		.addReg(0)          //base
		.addImm(1)          //scale
		.addReg(0)          //index
		.addImm(0)          //disp
		.addReg(0);			//segment
}
void checkRAJMP(MachineBasicBlock &MBB, MachineBasicBlock::iterator &MBBI, const TargetInstrInfo *TII)
{
    DebugLoc DL = MBBI->getDebugLoc();
	if(!MBB.back().isCall())
	{
		//pop r10
		BuildMI(MBB, MBBI, DL, TII->get(X86::POP64r), X86::R10);
	}else
	{
		//mov r10, [rsp]
		addDirectMem(
			BuildMI(MBB, MBBI, DL, TII->get(X86::MOV64rm), X86::R10)
			, X86::RSP);
	}

    //xor r11, r10
    BuildMI(MBB, MBBI, DL, TII->get(X86::XOR64rr), X86::R11)
		.addReg(X86::R11)
		.addReg(X86::R10);

	//popcnt r11, r11
	BuildMI(MBB, MBBI, DL, TII->get(X86::POPCNT64rr), X86::R11).addReg(X86::R11);
	BuildMI(MBB, MBBI, DL, TII->get(X86::SHL64ri), X86::R11)
		.addReg(X86::R11)
		.addImm(48);

	//or r11, r10
	BuildMI(MBB, MBBI, DL, TII->get(X86::OR64rr), X86::R11)
		.addReg(X86::R11)
		.addReg(X86::R10);
	if(!MBB.back().isCall())
	{
		//jmp r11
		BuildMI(MBB, MBBI, DL, TII->get(X86::JMP64r), X86::R11);
		//MBB.back().eraseFromParent();
	}else
	{
		//no return for tail call, instead just do a read
		//mov r11b, [r11]
		addDirectMem(
			BuildMI(MBB, MBBI, DL, TII->get(X86::MOV8rm), X86::R11B)
			, X86::R11);
	}
}

void checkRASilence(MachineBasicBlock &MBB, MachineBasicBlock::iterator &MBBI, const TargetInstrInfo *TII)
{
    DebugLoc DL = MBBI->getDebugLoc();
	if(!MBB.back().isCall())
	{
		BuildMI(MBB, MBBI, DL, TII->get(X86::LEA64r), X86::RSP)
			.addReg(X86::RSP)   //base
    		.addImm(1)          //scale
    		.addReg(0)          //index
    		.addImm(8)          //disp
    		.addReg(0);			//segment
			//jmp r11
		BuildMI(MBB, MBBI, DL, TII->get(X86::JMP64r), X86::R11);
		//MBB.back().eraseFromParent();
	}else
	{
		MachineInstrBuilder writeRA = BuildMI(MBB, MBBI, DL, TII->get(X86::MOV64mr));
		addDirectMem(writeRA, X86::RSP);
		writeRA.addReg(X86::R11);
	}
}
