//
//  X86MPXProtect.cpp
//  LLVMDemangle
//
//  Created by Kelvin Zhang on 2/14/18.
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

namespace{
    class X86ShadowStackMPX : public MachineFunctionPass{
    public:
        static char ID;
        X86ShadowStackMPX() : MachineFunctionPass(ID){}
        StringRef getPassName() const override { return "X86 MPX"; }
        bool runOnMachineFunction(MachineFunction &MF) override;
    };
}

char X86ShadowStackMPX::ID = 0;

FunctionPass *llvm::createX86ShadowStackMPX() { return new X86ShadowStackMPX(); }

bool X86ShadowStackMPX::runOnMachineFunction(MachineFunction &MF)
{
    const int64_t ROTATION_OFFSET = (0xFFFFFFFFFFFFFFFFL-(size_t)SHADOW_STACK_PTR + PRACTICAL_STACK_SIZE);
//#ifdef nonono
      /*NHB CFIXX*/
      const TargetInstrInfo *TII = MF.getSubtarget().getInstrInfo();
      //const TargetLowering *TLI = MF.getSubtarget().getTargetLowering();
      const TargetRegisterInfo *TRI = MF.getSubtarget().getRegisterInfo();
      for(auto &MBB : MF){
        for(auto &MI : MBB) {
          if(MI.mayStore()) {
            switch(MI.getOpcode()){
              case X86::MOV8mi:
              case X86::MOV16mi:
              case X86::MOV32mi:
              case X86::MOV8mr:
              case X86::MOV16mr:
              case X86::MOV32mr:
              case X86::MOV64mr:
              case X86::MOV64mi32:
              case X86::CMP8mi8:
              case X86::CMP8mi:
              case X86::CMP16mi8:
              case X86::CMP32mi8:
              case X86::CMP32mi:
              case X86::CMP32mr:
              case X86::CMP64mi32:
              case X86::CMP64mi8:
              case X86::CMP64mr:
              case X86::MOVSSmr:
              case X86::MOVSDmr:
              case X86::TEST32mi:
              case X86::TEST16mi:
              case X86::TEST8mi:
              case X86::MOVAPDmr:
              case X86::INC8m:
              case X86::INC16m:
              case X86::INC32m:
              case X86::INC64m:
              case X86::DEC8m:
              case X86::DEC16m:
              case X86::DEC32m:
              case X86::DEC64m:
              //case X86::JMP16m:
              //case X86::JMP32m:
              //case X86::JMP64m:
              case X86::ADD64mi8:
              case X86::ADD64mi32:
              case X86::ADD64mr:
              case X86::ADD32mi8:
              case X86::ADD32mi:
              case X86::ADD32mr:
              case X86::ADD16mi8:
              case X86::ADD16mi:
              case X86::ADD16mr:
              case X86::ADD8mi8:
              case X86::ADD8mr:
              case X86::MOVUPSmr:
              case X86::MOVUPDmr:
              case X86::MOVAPSmr:
              case X86::MOVDQUmr:
              case X86::MOVDQAmr:
              case X86::MOVPQI2QImr:
              case X86::MOVPDI2DImr:
              case X86::IDIV64m:
              case X86::IDIV32m:
              case X86::IDIV16m:
              case X86::IDIV8m:
              case X86::DIV64m:
              case X86::DIV32m:
              case X86::DIV16m:
              case X86::DIV8m:
              case X86::OR8mi:
              case X86::OR16mi:
              case X86::OR32mi:
              case X86::OR16mi8:
              case X86::OR32mi8:
              case X86::OR64mi32:
              case X86::OR8mr:
              case X86::OR16mr:
              case X86::OR32mr:
              case X86::OR64mr:
              case X86::XOR8mi:
              case X86::XOR16mi:
              case X86::XOR32mi:
              case X86::XOR16mi8:
              case X86::XOR32mi8:
              case X86::XOR8mr:
              case X86::XOR16mr:
              case X86::XOR32mr:
              case X86::XOR64mr:
              case X86::SETBEm:
              case X86::SETEm:
              case X86::SETGm:
              case X86::SETGEm:
              case X86::SETLm:
              case X86::SETLEm:
              case X86::SETNEm:
              case X86::AND8mr:
              case X86::AND16mr:
              case X86::AND32mr:
              case X86::AND64mr:
              case X86::AND8mi:
              case X86::AND16mi:
              case X86::AND32mi:
              case X86::AND8mi8:
              case X86::AND16mi8:
              case X86::AND32mi8:
              case X86::AND64mi8:
              case X86::AND64mi32:
              case X86::SUB8mr:
              case X86::SUB16mr:
              case X86::SUB32mr:
              case X86::SUB64mr:
              //case X86::CALL64m:
              //case X86::CALL32m:
              case X86::ROL64mi:
              case X86::ROL32mi:
              case X86::ROL16mi:
              case X86::ROR64mi:
              case X86::ROR32mi:
              case X86::ROR16mi:
              case X86::SHR64mi:
              case X86::SHR64mCL:
              case X86::SHR32mi:
              case X86::SHR16mi:
              case X86::SHL64mi:
              case X86::SHL32mi:
              case X86::SHL16mi:
              case X86::SHR64m1:
              case X86::SHR32m1:
              case X86::SHR16m1:
              case X86::SHL64m1:
              case X86::SHL32m1:
              case X86::SHL16m1:
              case X86::ADC64mi8:
              case X86::ADC32mi8:
              case X86::ADC32mi:
              case X86::ADC16mi:
              case X86::ADC8mi:
              case X86::SBB32mi:
              case X86::SBB32mi8:
              case X86::SBB16mi:
              case X86::SBB16mi8:
              case X86::MOV8mr_NOREX:
              //case X86::TAILJMPm64:
              case X86::SHLD64mri8:
              case X86::SHLD32mri8:
              case X86::SHLD16mri8:
              case X86::NOT64m:
              case X86::NOT32m:
              case X86::NOT16m:
              case X86::NOT8m:
              case X86::NEG64m:
              case X86::NEG32m:
              case X86::NEG16m:
              case X86::NEG8m:
              case X86::OR64mi8:
              case X86::OR8mi8:
              case X86::MOVLPDmr:
              case X86::MOVLPSmr:
              case X86::ST_FP80m:
              case X86::ST_FP64m:
              case X86::ST_FP32m:
              case X86::LD_F80m:
              case X86::LD_F64m:
              case X86::LD_F32m:
              case X86::SAR64mi:
              case X86::SAR32mi:
              case X86::SAR32m1:
              case X86::SAR64mCL:
              case X86::SHL64mCL:
              case X86::SETAEm:
              case X86::SETBm:
              case X86::MOVHPDmr:
              case X86::SETAm:
              case X86::ILD_F64m:
              case X86::MOVSDto64mr:
              case X86::MOVSS2DImr:
              case X86::MUL_F64m:
              case X86::MUL_F32m:
              case X86::LOCK_INC64m:
              case X86::LOCK_INC32m:
              case X86::LCMPXCHG64:
                {
                  const MCInstrDesc &leaOp = TII->get(X86::LEA64r);
                  //const MCInstrDesc &shrOp = TII->get(X86::SHR64ri);
                  const MCInstrDesc &bndcuOp = TII->get(X86::BNDCU64rr);
                  const MCInstrDesc &movOp = TII->get(X86::MOV64ri);
                  unsigned vReg1 = MF.getRegInfo().createVirtualRegister(TII->getRegClass(bndcuOp, 1, TRI, MF)); 
                  unsigned vReg2 = MF.getRegInfo().createVirtualRegister(TII->getRegClass(bndcuOp, 1, TRI, MF)); 
                  unsigned vReg3 = MF.getRegInfo().createVirtualRegister(TII->getRegClass(bndcuOp, 1, TRI, MF)); 
                  
                  //TODO: May need to change kill flags on my new instructions
                  MachineInstrBuilder lea = BuildMI(MBB, MI, MI.getDebugLoc(), leaOp, vReg1); 
                  lea->addOperand(MI.getOperand(0));
                  lea->addOperand(MI.getOperand(1));
                  lea->addOperand(MI.getOperand(2));
                  lea->addOperand(MI.getOperand(3));
                  lea->addOperand(MI.getOperand(4));

                  MachineInstrBuilder movabs = BuildMI(MBB, MI, MI.getDebugLoc(), movOp, vReg2); 
                  movabs.addImm(ROTATION_OFFSET);
  
                  MachineInstrBuilder lea2 = BuildMI(MBB, MI, MI.getDebugLoc(), leaOp, vReg3);
                  lea2.addReg(vReg1);
                  lea2.addImm(1);
                  lea2.addReg(vReg2);
                  lea2.addImm(0);
                  lea2.addReg(0);

                  MachineInstrBuilder bndcu = BuildMI(MBB, MI, MI.getDebugLoc(), bndcuOp);
                  bndcu.addReg(X86::BND0);
                  bndcu.addReg(vReg3); //, RegState::Kill);
                  /*
                  bndcu.addImm(1);
                  bndcu.addReg(0);
                  bndcu.addImm(0xff80fd6a);
                  bndcu.addReg(0);
                  */
                  break;
                }
              case X86::MOV8rm:
              case X86::MOV16rm:
              case X86::MOV32rm:
              case X86::MOV64rm:
              case X86::CMP64rm:
              case X86::CMP32rm:
              case X86::MOVSSrm:
              case X86::MOVSDrm:
              case X86::MOVSX64rm8:
              case X86::MOVSX64rm16:
              case X86::MOVSX64rm32:
              case X86::MOVSX32rm8:
              case X86::MOVSX32rm16:
              case X86::MOVZX32rm8:
              case X86::MOVZX32rm16:
              case X86::CVTSI2SDrm:
              case X86::CVTSI2SSrm:
              //case X86::CVTSS2SIrm:
              case X86::CVTTSS2SI64rm:
              case X86::CVTTSS2SIrm:
              case X86::CVTPS2PDrm:
              case X86::UCOMISSrm:
              case X86::UCOMISDrm:
              case X86::MOVAPDrm:
              case X86::MOVAPSrm:
              case X86::MOVUPSrm:
              case X86::MOVUPDrm:
              case X86::MOVDQUrm:
              case X86::MOVDQArm:
              case X86::MOVQI2PQIrm:
              case X86::IMUL64rmi32:
              case X86::IMUL64rmi8:
              case X86::IMUL32rmi8:
              case X86::IMUL16rmi8:
              case X86::IMUL32rmi:
              case X86::IMUL16rmi:
              case X86::MOVDI2PDIrm:
              case X86::TEST8mr:
              case X86::TEST16mr:
              case X86::TEST32mr:
              case X86::TEST64mr:
              case X86::PSHUFDmi:
              case X86::CVTTSD2SIrm:
              case X86::CVTTSD2SI64rm:
              case X86::PSHUFLWmi:
//              case X86::MOVZQI2PQIrm:
              case X86::CVTPD2PSrm:
              case X86::MOVDI2SSrm:
              case X86::MOV64toSDrm:
                {
                  const MCInstrDesc &leaOp = TII->get(X86::LEA64r);
                  //const MCInstrDesc &shrOp = TII->get(X86::SHR64ri);
                  const MCInstrDesc &bndcuOp = TII->get(X86::BNDCU64rr);
                  const MCInstrDesc &movOp = TII->get(X86::MOV64ri);
                  unsigned vReg1 = MF.getRegInfo().createVirtualRegister(TII->getRegClass(bndcuOp, 1, TRI, MF)); 
                  unsigned vReg2 = MF.getRegInfo().createVirtualRegister(TII->getRegClass(bndcuOp, 1, TRI, MF)); 
                  unsigned vReg3 = MF.getRegInfo().createVirtualRegister(TII->getRegClass(bndcuOp, 1, TRI, MF)); 
                  
                  //TODO: May need to change kill flags on my new instructions
                  MachineInstrBuilder lea = BuildMI(MBB, MI, MI.getDebugLoc(), leaOp, vReg1); 
                  lea->addOperand(MI.getOperand(1));
                  lea->addOperand(MI.getOperand(2));
                  lea->addOperand(MI.getOperand(3));
                  lea->addOperand(MI.getOperand(4));
                  lea->addOperand(MI.getOperand(5));

                  MachineInstrBuilder movabs = BuildMI(MBB, MI, MI.getDebugLoc(), movOp, vReg2); 
                  movabs.addImm(ROTATION_OFFSET);
  
                  MachineInstrBuilder lea2 = BuildMI(MBB, MI, MI.getDebugLoc(), leaOp, vReg3);
                  lea2.addReg(vReg1);
                  lea2.addImm(1);
                  lea2.addReg(vReg2);
                  lea2.addImm(0);
                  lea2.addReg(0);

                  MachineInstrBuilder bndcu = BuildMI(MBB, MI, MI.getDebugLoc(), bndcuOp);
                  bndcu.addReg(X86::BND0);
                  bndcu.addReg(vReg3); //, RegState::Kill);
                  /*
                  bndcu.addImm(1);
                  bndcu.addReg(0);
                  bndcu.addImm(0); //xff80fd6a);
                  bndcu.addReg(0);
                  */
                  break;
                }
                case X86::DIVSSrm:
              case X86::DIVSDrm:
              case X86::SUBSSrm:
              case X86::SUBSDrm:
              case X86::SUB64rm:
              case X86::SUB32rm:
              case X86::SUB16rm:
              case X86::SUB8rm:
              case X86::MULSSrm:
              case X86::MULSDrm:
              case X86::AND64rm:
              case X86::AND32rm:
              case X86::AND16rm:
              case X86::AND8rm:
              case X86::PADDQrm:
              case X86::LXADD64:
              case X86::LXADD32:
              case X86::ADD64rm:
              case X86::ADD32rm:
              case X86::ADD16rm:
              case X86::ADD8rm:
              case X86::ADDSSrm:
              case X86::ADDSDrm:
//              case X86::FvXORPDrm:
//              case X86::FvXORPSrm:
//              case X86::FvANDPSrm:
              case X86::OR64rm:
              case X86::OR32rm:
              case X86::OR16rm:
              case X86::OR8rm:
              case X86::PANDrm:
              case X86::PANDNrm:
              case X86::PXORrm:
              case X86::PORrm:
              case X86::MINSSrm:
              case X86::MINSDrm:
              case X86::MAXSSrm:
              case X86::MAXSDrm:
              case X86::IMUL64rm:
              case X86::IMUL32rm:
              case X86::IMUL16rm:
              case X86::UNPCKLPDrm:
              case X86::CMOVS64rm:
              case X86::CMOVS32rm:
              case X86::CMOVS16rm:
              case X86::CMOVE64rm:
              case X86::CMOVE32rm:
              case X86::CMOVE16rm:
              case X86::CMOVNE64rm:
              case X86::CMOVNE32rm:
              case X86::CMOVNE16rm:
              case X86::CMOVGE64rm:
              case X86::CMOVGE32rm:
              case X86::CMOVGE16rm:
              case X86::CMOVG64rm:
              case X86::CMOVG32rm:
              case X86::CMOVG16rm:
              case X86::CMOVLE64rm:
              case X86::CMOVLE32rm:
              case X86::CMOVLE16rm:
              case X86::CMOVL64rm:
              case X86::CMOVL32rm:
              case X86::CMOVL16rm:
              case X86::CMOVB64rm:
              case X86::CMOVB32rm:
              case X86::CMOVB16rm:
              case X86::CMOVBE64rm:
              case X86::CMOVBE32rm:
              case X86::CMOVBE16rm:
              case X86::XOR64rm:
              case X86::XOR32rm:
              case X86::XOR16rm:
              case X86::XOR8rm:
              case X86::PUNPCKLQDQrm:
              case X86::PINSRWrm:
              case X86::ORPSrm:
              case X86::PCMPEQDrm:
              case X86::PCMPEQQrm:
              case X86::PUNPCKLDQrm:
              case X86::SUBPDrm:
              case X86::CMOVAE64rm:
              case X86::MOVHPDrm:
              case X86::CMOVNS32rm:
              case X86::CMPSDrm:
              case X86::ANDPSrm:
              case X86::ANDPDrm:
              case X86::PMULUDQrm:
//              case X86::FvANDPDrm:
              case X86::CMOVA64rm:
              case X86::CMOVA32rm:
              case X86::CMOVA16rm:
              case X86::MULPDrm:
              case X86::ADDPDrm:
              case X86::PADDDrm:
              case X86::CMPSSrm:
              case X86::SHUFPDrmi:
              case X86::DIVPDrm:
              case X86::ADDPSrm:
              case X86::ANDNPDrm:
              case X86::MOVLPDrm:
              case X86::UNPCKLPSrm:
              case X86::UNPCKHPDrm:
              case X86::XORPDrm:
              case X86::CMPPDrmi:
              case X86::MULPSrm:
              case X86::XCHG64rm: 
                {
                  const MCInstrDesc &leaOp = TII->get(X86::LEA64r);
                  //const MCInstrDesc &shrOp = TII->get(X86::SHR64ri);
                  const MCInstrDesc &bndcuOp = TII->get(X86::BNDCU64rr);
                  const MCInstrDesc &movOp = TII->get(X86::MOV64ri);
                  unsigned vReg1 = MF.getRegInfo().createVirtualRegister(TII->getRegClass(bndcuOp, 1, TRI, MF)); 
                  unsigned vReg2 = MF.getRegInfo().createVirtualRegister(TII->getRegClass(bndcuOp, 1, TRI, MF)); 
                  unsigned vReg3 = MF.getRegInfo().createVirtualRegister(TII->getRegClass(bndcuOp, 1, TRI, MF)); 
                  
                  //TODO: May need to change kill flags on my new instructions
                  MachineInstrBuilder lea = BuildMI(MBB, MI, MI.getDebugLoc(), leaOp, vReg1); 
                  lea->addOperand(MI.getOperand(2));
                  lea->addOperand(MI.getOperand(3));
                  lea->addOperand(MI.getOperand(4));
                  lea->addOperand(MI.getOperand(5));
                  lea->addOperand(MI.getOperand(6));

                  MachineInstrBuilder movabs = BuildMI(MBB, MI, MI.getDebugLoc(), movOp, vReg2); 
                  movabs.addImm(ROTATION_OFFSET);
  
                  MachineInstrBuilder lea2 = BuildMI(MBB, MI, MI.getDebugLoc(), leaOp, vReg3);
                  lea2.addReg(vReg1);
                  lea2.addImm(1);
                  lea2.addReg(vReg2);
                  lea2.addImm(0);
                  lea2.addReg(0);

                  MachineInstrBuilder bndcu = BuildMI(MBB, MI, MI.getDebugLoc(), bndcuOp);
                  bndcu.addReg(X86::BND0);
                  bndcu.addReg(vReg3); //, RegState::Kill);
                  /*
                  bndcu.addImm(1);
                  bndcu.addReg(0);
                  bndcu.addImm(0); //xff80fd6a);
                  bndcu.addReg(0);
                  */
                  break;
                }

              default:
                break;
            }
          }
        }
      }
//#endif
      return true;
}
