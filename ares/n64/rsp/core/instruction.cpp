auto RSP::exception(uint type) -> void {
}

auto RSP::instruction() -> void {
  pipeline.address = PC;
  pipeline.instruction = imem.readWord(pipeline.address);
  if(IP) {
    PC = IP();
    IP = nothing;
  } else {
    PC += 4;
  }
//instructionDEBUG();
  instructionEXECUTE();
  GPR[0].u64 = 0;
}

auto RSP::instructionDEBUG() -> void {
}

auto RSP::instructionEXECUTE() -> void {
  switch(OP >> 26) {
  case 0x00: return instructionSPECIAL();
  case 0x01: return instructionREGIMM();
  case 0x02: return instructionJ();
  case 0x03: return instructionJAL();
  case 0x04: return instructionB(RS.u64 == RT.u64);  //BEQ
  case 0x05: return instructionB(RS.u64 != RT.u64);  //BNE
  case 0x06: return instructionB(RS.i64 <= 0);       //BLEZ
  case 0x07: return instructionB(RS.i64 >  0);       //BGTZ
  case 0x08: return instructionADDIU();  //ADDI
  case 0x09: return instructionADDIU();
  case 0x0a: return instructionSLTIU();  //SLTI
  case 0x0b: return instructionSLTIU();
  case 0x0c: return instructionANDI();
  case 0x0d: return instructionORI();
  case 0x0e: return instructionXORI();
  case 0x0f: return instructionLUI();
  case 0x10: return instructionCOP0();
  case 0x11: return instructionCOP1();
  case 0x12: return instructionCOP2();
  case 0x13: return instructionCOP3();
  case 0x14: break;  //BEQL
  case 0x15: break;  //BNEL
  case 0x16: break;  //BLEZL
  case 0x17: break;  //BGTZL
  case 0x18: break;  //DADDI
  case 0x19: break;  //DADDIU
  case 0x1a: break;  //LDL
  case 0x1b: break;  //LDR
  case 0x1c: break;
  case 0x1d: break;
  case 0x1e: break;
  case 0x1f: break;
  case 0x20: return instructionLB();
  case 0x21: return instructionLH();
  case 0x22: break;  //LWL
  case 0x23: return instructionLW();
  case 0x24: return instructionLBU();
  case 0x25: return instructionLHU();
  case 0x26: break;  //LWR
  case 0x27: break;  //LWU
  case 0x28: return instructionSB();
  case 0x29: return instructionSH();
  case 0x2a: break;  //SWL
  case 0x2b: return instructionSW();
  case 0x2c: break;  //SDL
  case 0x2d: break;  //SDR
  case 0x2e: break;  //SWR
  case 0x2f: return instructionCACHE();
  case 0x30: break;  //LL
  case 0x31: break;  //LWC1
  case 0x32: return instructionLWC2();
  case 0x33: break;  //LWC3
  case 0x34: break;  //LLD
  case 0x35: break;  //LDC1
  case 0x36: break;  //LDC2
  case 0x37: break;  //LD
  case 0x38: break;  //SC
  case 0x39: break;  //SWC1
  case 0x3a: return instructionSWC2();
  case 0x3b: break;  //SWC3
  case 0x3c: break;  //SCD
  case 0x3d: break;  //SDC1
  case 0x3e: break;  //SDC2
  case 0x3f: break;  //SD
  }
  exception(InvalidInstruction);
}

auto RSP::instructionSPECIAL() -> void {
  switch(OP & 0x3f) {
  case 0x00: return instructionSLL();
  case 0x01: break;
  case 0x02: return instructionSRL();
  case 0x03: return instructionSRA();
  case 0x04: return instructionSLLV();
  case 0x05: break;
  case 0x06: return instructionSRLV();
  case 0x07: return instructionSRAV();
  case 0x08: return instructionJR();
  case 0x09: return instructionJALR();
  case 0x0a: break;
  case 0x0b: break;
  case 0x0c: break;  //SYSCALL
  case 0x0d: return instructionBREAK();
  case 0x0e: break;
  case 0x0f: return; //SYNC
  case 0x10: break;  //MFHI
  case 0x11: break;  //MTHI
  case 0x12: break;  //MFLO
  case 0x13: break;  //MTLO
  case 0x14: break;  //DSLLV
  case 0x15: break;
  case 0x16: break;  //DSRLV
  case 0x17: break;  //DSRAV
  case 0x18: break;  //MULT
  case 0x19: break;  //MULTU
  case 0x1a: break;  //DIV
  case 0x1b: break;  //DIVU
  case 0x1c: break;  //DMULT
  case 0x1d: break;  //DMULTU
  case 0x1e: break;  //DDIV
  case 0x1f: break;  //DDIVU
  case 0x20: return instructionADDU();  //ADD
  case 0x21: return instructionADDU();
  case 0x22: return instructionSUBU();  //SUB
  case 0x23: return instructionSUBU();
  case 0x24: return instructionAND();
  case 0x25: return instructionOR();
  case 0x26: return instructionXOR();
  case 0x27: return instructionNOR();
  case 0x28: break;
  case 0x29: break;
  case 0x2a: return instructionSLTU();  //SLT
  case 0x2b: return instructionSLTU();
  case 0x2c: break;  //DADD
  case 0x2d: break;  //DADDU
  case 0x2e: break;  //DSUB
  case 0x2f: break;  //DSUBU
  case 0x30: break;  //TGE
  case 0x31: break;  //TGEU
  case 0x32: break;  //TLT
  case 0x33: break;  //TLTU
  case 0x34: break;  //TEQ
  case 0x35: break;
  case 0x36: break;  //TNE
  case 0x37: break;
  case 0x38: break;  //DSLL
  case 0x39: break;
  case 0x3a: break;  //DSRL
  case 0x3b: break;  //DSRA
  case 0x3c: break;  //DSLL32
  case 0x3d: break;
  case 0x3e: break;  //DSRL32
  case 0x3f: break;  //DSRA32
  }
  exception(InvalidInstruction);
}

auto RSP::instructionREGIMM() -> void {
  switch(OP >> 16 & 0x1f) {
  case 0x00: return instructionB(RS.i64 <  0);  //BLTZ
  case 0x01: return instructionB(RS.i64 >= 0);  //BGEZ
  case 0x02: break;  //BLTZL
  case 0x03: break;  //BGEZL
  case 0x04: break;
  case 0x05: break;
  case 0x06: break;
  case 0x07: break;
  case 0x08: break;  //TGEI
  case 0x09: break;  //TGEIU
  case 0x0a: break;  //TLTI
  case 0x0b: break;  //TLTIU
  case 0x0c: break;  //TEQI
  case 0x0d: break;
  case 0x0e: break;  //TNEI
  case 0x0f: break;
  case 0x10: break;  //BLTZAL
  case 0x11: break;  //BGEZAL
  case 0x12: break;  //BLTZALL
  case 0x13: break;  //BGEZALL
  case 0x14: break;
  case 0x15: break;
  case 0x16: break;
  case 0x17: break;
  case 0x18: break;
  case 0x19: break;
  case 0x1a: break;
  case 0x1b: break;
  case 0x1c: break;
  case 0x1d: break;
  case 0x1e: break;
  case 0x1f: break;
  }
  exception(InvalidInstruction);
}

auto RSP::instructionCOP0() -> void {
//if(!STATUS_COP0) return exception(CoprocessorUnusable);
  switch(OP >> 21 & 31) {
  case 0x00: return instructionMFC0();
  case 0x01: break;  //DMFC0
  case 0x02: return instructionCFC0();
  case 0x04: return instructionMTC0();
  case 0x05: break;  //DMTC0
  case 0x06: return instructionCTC0();
  case 0x08: break;  //BC0
  }
  exception(InvalidInstruction);
}

auto RSP::instructionCOP1() -> void {
//if(!STATUS_COP1) return exception(CoprocessorUnusable);
  exception(InvalidInstruction);
}

auto RSP::instructionCOP2() -> void {
//if(!STATUS_COP2) return exception(CoprocessorUnusable);
  switch(OP >> 21 & 31) {
  case 0x00: return instructionMFC2();
  case 0x01: break;  //DMFC2
  case 0x02: return instructionCFC2();
  case 0x04: return instructionMTC2();
  case 0x05: break;  //DMTC2
  case 0x06: return instructionCTC2();
  case 0x08: break;  //BC2
  }
  if(!(OP >> 25 & 1)) return exception(InvalidInstruction);
  switch(OP & 0x3f) {
  case 0x00: return instructionVMULF(0);
  case 0x01: return instructionVMULF(1);
  case 0x02: return instructionVRND(1);
  case 0x03: return instructionVMULQ();
  case 0x04: return instructionVMUDL();
  case 0x05: return instructionVMUDM();
  case 0x06: return instructionVMUDN();
  case 0x07: return instructionVMUDH();
  case 0x08: return instructionVMACF(0);
  case 0x09: return instructionVMACF(1);
  case 0x0a: return instructionVRND(0);
  case 0x0b: return instructionVMACQ();
  case 0x0c: return instructionVMADL();
  case 0x0d: return instructionVMADM();
  case 0x0e: return instructionVMADN();
  case 0x0f: return instructionVMADH();
  case 0x10: return instructionVADD();
  case 0x11: return instructionVSUB();
  case 0x12: break;
  case 0x13: return instructionVABS();
  case 0x14: return instructionVADDC();
  case 0x15: return instructionVSUBC();
  case 0x16: break;
  case 0x17: break;
  case 0x18: break;
  case 0x19: break;
  case 0x1a: break;
  case 0x1b: break;
  case 0x1c: break;
  case 0x1d: return instructionVSAR();
  case 0x1e: break;
  case 0x1f: break;
  case 0x20: return instructionVLT();
  case 0x21: return instructionVEQ();
  case 0x22: return instructionVNE();
  case 0x23: return instructionVGE();
  case 0x24: return instructionVCL();
  case 0x25: return instructionVCH();
  case 0x26: return instructionVCR();
  case 0x27: return instructionVMRG();
  case 0x28: return instructionVAND();
  case 0x29: return instructionVNAND();
  case 0x2a: return instructionVOR();
  case 0x2b: return instructionVNOR();
  case 0x2c: return instructionVXOR();
  case 0x2d: return instructionVNXOR();
  case 0x2e: break;
  case 0x2f: break;
  case 0x30: return instructionVRCP(0);
  case 0x31: return instructionVRCP(1);
  case 0x32: return instructionVRCPH();
  case 0x33: return instructionVMOV();
  case 0x34: return instructionVRSQ(0);
  case 0x35: return instructionVRSQ(1);
  case 0x36: return instructionVRSQH();
  case 0x37: return instructionVNOP();
  case 0x38: break;
  case 0x39: break;
  case 0x3a: break;
  case 0x3b: break;
  case 0x3c: break;
  case 0x3d: break;
  case 0x3e: break;
  case 0x3f: break;
  }
  exception(InvalidInstruction);
}

auto RSP::instructionCOP3() -> void {
//if(!STATUS_COP3) return exception(CoprocessorUnusable);
  exception(InvalidInstruction);
}
