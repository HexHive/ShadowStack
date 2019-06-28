//
//  X86ShadowStackReg.cpp
//  LLVMX86CodeGen
//
//  Created by Kelvin Zhang on 2/8/18.
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
#include "ShadowStack/ShadowStackFeatures.h"
#include "ShadowStack/ShadowStackConstants.h"
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <iterator>


using namespace llvm;
extern cl::opt<bool> parallel;
extern cl::opt<std::string> epilog;

namespace{
    class X86ShadowStackReg : public MachineFunctionPass{
    public:
        static char ID;
        X86ShadowStackReg() : MachineFunctionPass(ID){}
        StringRef getPassName() const override { return "X86 Shadow Stack Register Scheme"; }
        bool runOnMachineFunction(MachineFunction &MF) override;
    };
}

char X86ShadowStackReg::ID = 0;

FunctionPass *llvm::createX86ShadowStackReg() { return new X86ShadowStackReg(); }

extern bool mayWriteMem(const MachineFunction &MF);
extern void WRPKRUI(MachineBasicBlock &MBB, const MachineBasicBlock::iterator &MBBI, const DebugLoc &DL, const TargetInstrInfo *TII, int imm);
extern void checkRA(MachineBasicBlock &MBB, MachineBasicBlock::iterator &MBBI, const TargetInstrInfo *TII);
extern void checkRALBP(MachineBasicBlock &MBB, MachineBasicBlock::iterator &MBBI, const TargetInstrInfo *TII);
extern void checkRAJMP(MachineBasicBlock &MBB, MachineBasicBlock::iterator &MBBI, const TargetInstrInfo *TII);
extern void checkRASilence(MachineBasicBlock &MBB, MachineBasicBlock::iterator &MBBI, const TargetInstrInfo *TII);

static void emitMinimalRegisterProlog(MachineBasicBlock &MBB, const MachineBasicBlock::iterator &MBBI, const DebugLoc DL, const TargetInstrInfo *TII)
{
    //get the return addr
    //mov RAX, [rsp]
    MachineInstrBuilder getRA = BuildMI(MBB, MBBI, DL, TII->get(X86::MOV64rm), X86::RAX);
    addDirectMem(getRA, X86::RSP);
    
    //store return addr
    //mov [r15], RAX
    MachineInstrBuilder putRA = BuildMI(MBB, MBBI, DL, TII->get(X86::MOV64mr));
    addDirectMem(putRA, X86::R15);
    putRA.addReg(X86::RAX);

	//mov [r15+8], rsp
    MachineInstrBuilder putRSP = BuildMI(MBB, MBBI, DL, TII->get(X86::MOV64mr));
    addRegOffset(putRSP, X86::R15, false, 8);
    putRSP.addReg(X86::RSP);

	//lea r15, [r15 + 16]
	BuildMI(MBB, MBBI, DL, TII->get(X86::LEA64r), X86::R15)
		.addReg(X86::R15)   //base
    	.addImm(1)          //scale
    	.addReg(0)          //index
    	.addImm(16)          //disp
    	.addReg(0);			//segment
}

static void emitParallelRegisterProlog(MachineBasicBlock &MBB, const MachineBasicBlock::iterator &MBBI, const DebugLoc DL, const TargetInstrInfo *TII)
{
	
    //get the return addr
    //mov RAX, [rsp]
    MachineInstrBuilder getRA = BuildMI(MBB, MBBI, DL, TII->get(X86::MOV64rm), X86::RAX);
    addDirectMem(getRA, X86::RSP);
    
    //store return addr
    //mov [rsp+r15], RAX
    MachineInstrBuilder putRA = BuildMI(MBB, MBBI, DL, TII->get(X86::MOV64mr));
    addRegReg(putRA, X86::RSP, false, X86::R15, false);
    putRA.addReg(X86::RAX);
}

static void emitRegisterProlog(MachineFunction &MF)
{
    const TargetInstrInfo *TII = MF.getSubtarget().getInstrInfo();
    const DebugLoc DL = MF.front().size() == 0 ? DebugLoc() :MF.front().front().getDebugLoc();
    MachineBasicBlock &MBB = MF.front();
    const MachineBasicBlock::iterator &MBBI = MBB.begin();
    
#ifdef SHADOW_STACK_MPK
        //save ecx and edx
        BuildMI(MBB, MBBI, DL, TII->get(X86::MOV64rr), X86::R10).addReg(X86::RCX, RegState::Kill);
        BuildMI(MBB, MBBI, DL, TII->get(X86::MOV64rr), X86::R11).addReg(X86::RDX, RegState::Kill);
        
        //disable MPK protections
        WRPKRUI(MBB, MBBI, DL, TII, 0);
#endif
		
#ifndef SHADOW_STACK_PARR
		emitMinimalRegisterProlog(MBB, MBBI, DL, TII);
#else
		emitParallelRegisterProlog(MBB, MBBI, DL, TII);
#endif

#ifdef SHADOW_STACK_MPK
        //reenable protection
        BuildMI(MBB, MBBI, DL, TII->get(X86::MOV64ri), X86::RAX).addImm(WRITE_PROTECTION);
        BuildMI(MBB, MBBI, DL, TII->get(X86::WRPKRUr));
        
        //restore ecx and edx
        BuildMI(MBB, MBBI, DL, TII->get(X86::MOV64rr), X86::RCX).addReg(X86::R10, RegState::Kill);
        BuildMI(MBB, MBBI, DL, TII->get(X86::MOV64rr), X86::RDX).addReg(X86::R11, RegState::Kill);
#endif
}

//read shadow RA into R11
static void emitParallelRegisterEpilog(MachineBasicBlock &MBB, const MachineBasicBlock::iterator &MBBI, const DebugLoc DL, const TargetInstrInfo *TII)
{
    //mov r11, [rsp+r15]
    MachineInstrBuilder getRA = BuildMI(MBB, MBBI, DL, TII->get(X86::MOV64rm), X86::R11);
    addRegReg(getRA, X86::RSP, false, X86::R15, false);
}

//read shadow RA into r11
static void emitMinimalRegisterEpilog(MachineBasicBlock &MBB, const MachineBasicBlock::iterator &MBBI, const DebugLoc DL, const TargetInstrInfo *TII)
{
    //mov r11, [r15-8]
    MachineInstrBuilder getRA = BuildMI(MBB, MBBI, DL, TII->get(X86::MOV64rm), X86::R11);
    addRegOffset(getRA, X86::R15, false, -16);
	//lea r15, [r15-16]
	BuildMI(MBB, MBBI, DL, TII->get(X86::LEA64r), X86::R15)
		.addReg(X86::R15)   //base
    	.addImm(1)          //scale
    	.addReg(0)          //index
    	.addImm(-16)          //disp
    	.addReg(0);			//segment
}
static void emitRegisterEpilog(MachineBasicBlock &MBB)
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
#ifndef SHADOW_STACK_PARR 
		emitMinimalRegisterEpilog(MBB, MBBI, DL, TII);
#else
		emitParallelRegisterEpilog(MBB, MBBI, DL, TII);
#endif
    
	//checkRAJMP(MBB, MBBI, TII);
	checkRALBP(MBB, MBBI, TII);
	//checkRA(MBB, MBBI, TII);
    
    //Restore R10, R11
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

static bool isGlobalConstructor(const MachineFunction &MF)
{
    const Module *MOD = MF.getFunction().getParent();
    GlobalVariable *ctors = MOD->getGlobalVariable("llvm.global_ctors");
    if(ctors == NULL)return false;
    ConstantArray *CA = dyn_cast<ConstantArray>(ctors->getInitializer());
    if(CA)
    {
        for(Use &OP : CA->operands())
        {
            if(isa<ConstantAggregateZero>(OP))continue;
            ConstantStruct *CS = cast<ConstantStruct>(OP);
            if(Function *F = dyn_cast<Function>(CS->getOperand(1)))
            {
                if(F->getName() == MF.getFunction().getName())return true;
            }
        }
    }
	ctors = MOD->getGlobalVariable("llvm.global_dtors");
    if(ctors == NULL)return false;
    CA = dyn_cast<ConstantArray>(ctors->getInitializer());
    if(CA)
    {
        for(Use &OP : CA->operands())
        {
            if(isa<ConstantAggregateZero>(OP))continue;
            ConstantStruct *CS = cast<ConstantStruct>(OP);
            if(Function *F = dyn_cast<Function>(CS->getOperand(1)))
            {
                if(F->getName() == MF.getFunction().getName())return true;
            }
        }
    }
    return false;
}

static void loadR15(MachineBasicBlock &MBB, const MachineBasicBlock::iterator &MBBI, 
		const DebugLoc &DL, const TargetInstrInfo *TII)
{
#ifndef SHADOW_STACK_PARR 
		//read gs:[16], contains top of shadow stack
		//mov r15, gs:[0]
		BuildMI(MBB, MBBI, DL, TII->get(X86::MOV64rm), X86::R15)
			.addReg(0)          //base
    		.addImm(1)          //scale
    		.addReg(0)          //index
    		.addImm(16)          //disp
    		.addReg(X86::GS);    //segment
#else
		//read gs:[0], contains offset of shadow stack
		//mov r15, gs:[0]
		BuildMI(MBB, MBBI, DL, TII->get(X86::MOV64rm), X86::R15)
			.addReg(0)          //base
    		.addImm(1)          //scale
    		.addReg(0)          //index
    		.addImm(0)          //disp
    		.addReg(X86::GS);    //segment
#endif
}

bool X86ShadowStackReg::runOnMachineFunction(MachineFunction &MF)
{
    const bool unsafe = mayWriteMem(MF);
    if (unsafe)
        emitRegisterProlog(MF);
	
	if(isGlobalConstructor(MF))
	{
        const TargetInstrInfo *TII = MF.getSubtarget().getInstrInfo();
        const DebugLoc DL = MF.front().front().getDebugLoc();
        MachineBasicBlock &MBB = MF.front();
        const MachineBasicBlock::iterator &MBBI = MBB.begin();
        loadR15(MBB, MBBI, DL, TII);
		return true;
	}

    if(MF.getFunction().getName() == "main")
    {
        const TargetInstrInfo *TII = MF.getSubtarget().getInstrInfo();
        const DebugLoc DL = MF.front().front().getDebugLoc();
        MachineBasicBlock &MBB = MF.front();
        const MachineBasicBlock::iterator &MBBI = MBB.begin();
        loadR15(MBB, MBBI, DL, TII);
    }
	
	if(!unsafe)return true;

    for (MachineBasicBlock &MBB : MF)
    {
        if (MBB.isReturnBlock())
        {
            emitRegisterEpilog(MBB);
        }
    }
    return true;
}
