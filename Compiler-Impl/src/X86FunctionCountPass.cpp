#include "X86InstrBuilder.h"
#include "X86InstrInfo.h"
#include "X86MachineFunctionInfo.h"
#include "X86Subtarget.h"
#include "X86TargetMachine.h"
#include "llvm/ADT/SmallSet.h"


using namespace llvm;


namespace{
	class FunctionCountPass : public MachineFunctionPass{
	public:
		static char ID;
		FunctionCountPass(): MachineFunctionPass(ID){}
        StringRef getPassName() const override { return "X86 Funciton Count Pass"; }
        bool runOnMachineFunction(MachineFunction &MF) override;
	}; 
}


char FunctionCountPass::ID = 0;

FunctionPass *llvm::createX86FunctionCountPass() { return new FunctionCountPass(); }


bool FunctionCountPass::runOnMachineFunction(MachineFunction &MF) {
	MachineBasicBlock &MBB = MF.front();
	MachineBasicBlock::iterator MBBI = MBB.begin();
	const DebugLoc DL = MBBI->getDebugLoc();
    const TargetInstrInfo *TII = MF.getSubtarget().getInstrInfo();
	BuildMI(MBB, MBBI, DL, TII->get(X86::INC64m)) 
		.addReg(0)          //base
    	.addImm(1)          //scale
    	.addReg(0)          //index
    	.addImm(24)          //disp
    	.addReg(X86::GS);    //segment

	return true;
}
