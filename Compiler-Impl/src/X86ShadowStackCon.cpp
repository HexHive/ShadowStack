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

cl::opt<bool>
    parallel("x86-parallel-ss",
	cl::desc("Use a parallel shadow stack instead of minimal"),
	cl::init(false), 
	cl::NotHidden);

cl::opt<std::string>
    epilog("x86-ss-epilog",
	cl::desc("Use this type of epilog"),
	cl::init("cmp"), 
	cl::NotHidden);

cl::opt<bool>
mpk("x86-mpk",
          cl::desc("Use mpk to protect shadow stack"),
          cl::init(false),
          cl::NotHidden);


namespace{
    class X86ShadowStackCon : public MachineFunctionPass{
    public:
        static char ID;
        X86ShadowStackCon() : MachineFunctionPass(ID){}
        StringRef getPassName() const override { return "X86 Shadow Stack Con"; }
        bool runOnMachineFunction(MachineFunction &MF) override;
    };
}
char X86ShadowStackCon::ID = 0;
extern void checkRA(MachineBasicBlock &MBB, MachineBasicBlock::iterator &MBBI, const TargetInstrInfo *TII);

FunctionPass *llvm::createX86ShadowStackCon() { return new X86ShadowStackCon(); }


void XORReg(MachineBasicBlock &MBB, const MachineBasicBlock::iterator &MBBI, const DebugLoc &DL, const TargetInstrInfo *TII, unsigned Reg)
{
    BuildMI(MBB, MBBI, DL, TII->get(X86::MOV32ri), Reg).addImm(0);
}

void WRPKRUI(MachineBasicBlock &MBB, const MachineBasicBlock::iterator &MBBI, const DebugLoc &DL, const TargetInstrInfo *TII, int imm)
{
    BuildMI(MBB, MBBI, DL, TII->get(X86::MOV32ri), X86::EAX).addImm(imm);
    XORReg(MBB, MBBI, DL, TII, X86::ECX);
    XORReg(MBB, MBBI, DL, TII, X86::EDX);
    BuildMI(MBB, MBBI, DL, TII->get(X86::WRPKRUr));
}

static void emitShadowStackPrologue(MachineFunction &MF)
{
    const TargetInstrInfo *TII = MF.getSubtarget().getInstrInfo();
    
    const DebugLoc DL = MF.front().size() == 0 ? DebugLoc() :MF.front().front().getDebugLoc();
    
    //put all the shadow stack code in a separate MBB
    MachineBasicBlock &MBB = MF.front();
    const MachineBasicBlock::iterator &MBBI = MBB.begin();
    if(mpk.getValue())
    {
        //save ecx and edx
        BuildMI(MBB, MBBI, DL, TII->get(X86::MOV64rr), X86::R10).addReg(X86::RCX, RegState::Kill);
        BuildMI(MBB, MBBI, DL, TII->get(X86::MOV64rr), X86::R11).addReg(X86::RDX, RegState::Kill);
        
        //disable MPK protections
        WRPKRUI(MBB, MBBI, DL, TII, 0);
    }
    //get the return addr
    //mov RAX, [rsp]
    MachineInstrBuilder getRA = BuildMI(MBB, MBBI, DL, TII->get(X86::MOV64rm), X86::RAX);
    addRegOffset(getRA, X86::RSP, false, 0);

    //store return addr
    //mov [rsp+SHADOW_STACK_OFFSET], vReg1
    MachineInstrBuilder putRA = BuildMI(MBB, MBBI, DL, TII->get(X86::MOV64mr));
    addRegOffset(putRA, X86::RSP, false, SHADOW_STACK_OFFSET).addReg(X86::RAX, RegState::Kill);
    if(mpk.getValue())
    {
        //reenable protection
        BuildMI(MBB, MBBI, DL, TII->get(X86::MOV64ri), X86::RAX).addImm(WRITE_PROTECTION);
        BuildMI(MBB, MBBI, DL, TII->get(X86::WRPKRUr));
        
        //restore ecx and edx
        BuildMI(MBB, MBBI, DL, TII->get(X86::MOV64rr), X86::RCX).addReg(X86::R10, RegState::Kill);
        BuildMI(MBB, MBBI, DL, TII->get(X86::MOV64rr), X86::RDX).addReg(X86::R11, RegState::Kill);
    }
}

static void emitShadowStackEpilogue(MachineBasicBlock &MBB)
{
    MachineBasicBlock::iterator &MBBI = --MBB.end();
    
    const DebugLoc &DL = MBBI->getDebugLoc();
    const TargetInstrInfo *TII = MBB.getParent()->getSubtarget().getInstrInfo();
    
    const MachineInstr &back = MBB.back();
    
	bool saveR10 = back.readsRegister(X86::R10);
	bool saveR11 = back.readsRegister(X86::R11);

    //If tail call needs R10, R11
    if(saveR11)
    {
        MachineInstrBuilder saveR11 = BuildMI(MBB, MBBI, DL, TII->get(X86::MOV64mr));
		addRegOffset(saveR11, X86::RSP, false, -16).addReg(X86::R11);
		//BuildMI(MBB, MBBI, DL, TII->get(X86::MOV64rr), X86::RAX).addReg(X86::R11);
    }
    if(saveR10)
    {
        MachineInstrBuilder saveR10 = BuildMI(MBB, MBBI, DL, TII->get(X86::MOV64mr));
        addRegOffset(saveR10, X86::RSP, false, -24).addReg(X86::R10);
		//BuildMI(MBB, MBBI, DL, TII->get(X86::MOV64rr), X86::RDX).addReg(X86::R10);
    }
    
    //mov r11, [rsp+SHADOW_STACK_OFFSET]
    MachineInstrBuilder getRA = BuildMI(MBB, MBBI, DL, TII->get(X86::MOV64rm), X86::R11);
    addRegOffset(getRA, X86::RSP, false, SHADOW_STACK_OFFSET);

    //check RA in r11
    checkRA(MBB, MBBI, TII);
    
    //restore R11, R10 if necessary
    if(saveR11)
    {
        MachineInstrBuilder saveR11 = BuildMI(MBB, MBBI, DL, TII->get(X86::MOV64rm), X86::R11);
        addRegOffset(saveR11, X86::RSP, false, -16);
		//BuildMI(MBB, MBBI, DL, TII->get(X86::MOV64rr), X86::R11).addReg(X86::RAX);
    }
    if(saveR10)
    {
        MachineInstrBuilder saveR10 = BuildMI(MBB, MBBI, DL, TII->get(X86::MOV64rm), X86::R10);
        addRegOffset(saveR10, X86::RSP, false, -24);
		//BuildMI(MBB, MBBI, DL, TII->get(X86::MOV64rr), X86::R10).addReg(X86::RDX);
    }
}

static bool isPush(unsigned opcode)
{
	switch(opcode)
	{
		case X86::PUSH64i32:
		case X86::PUSHi32:
		case X86::PUSH64i8:
		case X86::PUSH32i8:
		case X86::PUSH64rmm:
		case X86::PUSH32rmm:
		case X86::PUSH16rmm:
		case X86::PUSH16r:
		case X86::PUSH32r:
		case X86::PUSH64r:
			return true;
		default:
			return false;
	}
}

static bool isGlobalVarAccess(const MachineInstr &MI)
{
    for(MachineMemOperand *MMO : MI.memoperands())
    {
        if(MMO->isStore())
        {
            if(MMO->getValue() && isa<GlobalValue>(MMO->getValue()))
            {
                return true;
            }
        }
    }
    return false;
}

bool mayWriteMem(const MachineFunction &MF)
{
	for(const MachineBasicBlock &MBB : MF)
	{
		for(const MachineInstr &MI : MBB)
		{
			//if this is a push instr, ignore it
			if(isPush(MI.getOpcode()))continue;
			if(MI.mayStore())
			{
				if(!isGlobalVarAccess(MI))return true;
			}
		}
	}
	return false;
}

bool X86ShadowStackCon::runOnMachineFunction(MachineFunction &MF) {
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
