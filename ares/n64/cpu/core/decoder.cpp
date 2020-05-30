#define OP     pipeline.instruction
#define SA     (OP >>  6 & 31)
#define RDn    (OP >> 11 & 31)
#define RTn    (OP >> 16 & 31)
#define RSn    (OP >> 21 & 31)
#define RD     GPR[RDn]
#define RT     GPR[RTn]
#define RS     GPR[RSn]
#define FD     (OP >>  6 & 31)
#define FS     (OP >> 11 & 31)
#define FT     (OP >> 16 & 31)
#define IMMi16 i16(OP)
#define IMMu16 u16(OP)
#define IMMu26 (OP & 0x03ff'ffff)

auto CPU::instructionEXECUTE() -> void {
  switch(OP >> 26) {
  case 0x00: return instructionSPECIAL();
  case 0x01: return instructionREGIMM();
  case 0x02: return instructionJ(IMMu26);
  case 0x03: return instructionJAL(IMMu26);
  case 0x04: return instructionBEQ(RS, RT, IMMi16);
  case 0x05: return instructionBNE(RS, RT, IMMi16);
  case 0x06: return instructionBLEZ(RS, IMMi16);
  case 0x07: return instructionBGTZ(RS, IMMi16);
  case 0x08: return instructionADDI(RT, RS, IMMi16);
  case 0x09: return instructionADDIU(RT, RS, IMMi16);
  case 0x0a: return instructionSLTI(RT, RS, IMMi16);
  case 0x0b: return instructionSLTIU(RT, RS, IMMi16);
  case 0x0c: return instructionANDI(RT, RS, IMMu16);
  case 0x0d: return instructionORI(RT, RS, IMMu16);
  case 0x0e: return instructionXORI(RT, RS, IMMu16);
  case 0x0f: return instructionLUI(RT, IMMu16);
  case 0x10: return instructionCOP0();
  case 0x11: return instructionCOP1();
  case 0x12: return exception.coprocessor2();  //COP2
  case 0x13: return exception.coprocessor3();  //COP3
  case 0x14: return instructionBEQL(RS, RT, IMMi16);
  case 0x15: return instructionBNEL(RS, RT, IMMi16);
  case 0x16: return instructionBLEZL(RS, IMMi16);
  case 0x17: return instructionBGTZL(RS, IMMi16);
  case 0x18: return instructionDADDI(RT, RS, IMMi16);
  case 0x19: return instructionDADDIU(RT, RS, IMMi16);
  case 0x1a: return instructionLDL(RT, RS, IMMi16);
  case 0x1b: return instructionLDR(RT, RS, IMMi16);
  case 0x1c: break;
  case 0x1d: break;
  case 0x1e: break;
  case 0x1f: break;
  case 0x20: return instructionLB(RT, RS, IMMi16);
  case 0x21: return instructionLH(RT, RS, IMMi16);
  case 0x22: return instructionLWL(RT, RS, IMMi16);
  case 0x23: return instructionLW(RT, RS, IMMi16);
  case 0x24: return instructionLBU(RT, RS, IMMi16);
  case 0x25: return instructionLHU(RT, RS, IMMi16);
  case 0x26: return instructionLWR(RT, RS, IMMi16);
  case 0x27: return instructionLWU(RT, RS, IMMi16);
  case 0x28: return instructionSB(RT, RS, IMMi16);
  case 0x29: return instructionSH(RT, RS, IMMi16);
  case 0x2a: return instructionSWL(RT, RS, IMMi16);
  case 0x2b: return instructionSW(RT, RS, IMMi16);
  case 0x2c: return instructionSDL(RT, RS, IMMi16);
  case 0x2d: return instructionSDR(RT, RS, IMMi16);
  case 0x2e: return instructionSWR(RT, RS, IMMi16);
  case 0x2f: return instructionCACHE(OP >> 16 & 3, OP >> 18 & 3);
  case 0x30: return instructionLL(RT, RS, IMMi16);
  case 0x31: return instructionLWC1(FT, RS, IMMi16);
  case 0x32: return exception.coprocessor2();  //LWC2
  case 0x33: return exception.coprocessor3();  //LWC3
  case 0x34: return instructionLLD(RT, RS, IMMi16);
  case 0x35: return instructionLDC1(FT, RS, IMMi16);
  case 0x36: return exception.coprocessor2();  //LDC2
  case 0x37: return instructionLD(RT, RS, IMMi16);
  case 0x38: return instructionSC(RT, RS, IMMi16);
  case 0x39: return instructionSWC1(FT, RS, IMMi16);
  case 0x3a: return exception.coprocessor2();  //SWC2
  case 0x3b: return exception.coprocessor3();  //SWC3
  case 0x3c: return instructionSCD(RT, RS, IMMi16);
  case 0x3d: return instructionSDC1(FT, RS, IMMi16);
  case 0x3e: return exception.coprocessor2();  //SDC2
  case 0x3f: return instructionSD(RT, RS, IMMi16);
  }

  exception.reservedInstruction();
}

auto CPU::instructionSPECIAL() -> void {
  switch(OP & 0x3f) {
  case 0x00: return instructionSLL(RD, RT, SA);
  case 0x01: break;
  case 0x02: return instructionSRL(RD, RT, SA);
  case 0x03: return instructionSRA(RD, RT, SA);
  case 0x04: return instructionSLLV(RD, RT, RS);
  case 0x05: break;
  case 0x06: return instructionSRLV(RD, RT, RS);
  case 0x07: return instructionSRAV(RD, RT, RS);
  case 0x08: return instructionJR(RS);
  case 0x09: return instructionJALR(RD, RS);
  case 0x0a: break;
  case 0x0b: break;
  case 0x0c: return instructionSYSCALL();
  case 0x0d: return instructionBREAK();
  case 0x0e: break;
  case 0x0f: return;  //SYNC
  case 0x10: return instructionMFHI(RD);
  case 0x11: return instructionMTHI(RS);
  case 0x12: return instructionMFLO(RD);
  case 0x13: return instructionMTLO(RS);
  case 0x14: return instructionDSLLV(RD, RT, RS);
  case 0x15: break;
  case 0x16: return instructionDSRLV(RD, RT, RS);
  case 0x17: return instructionDSRAV(RD, RT, RS);
  case 0x18: return instructionMULT(RS, RT);
  case 0x19: return instructionMULTU(RS, RT);
  case 0x1a: return instructionDIV(RS, RT);
  case 0x1b: return instructionDIVU(RS, RT);
  case 0x1c: return instructionDMULT(RS, RT);
  case 0x1d: return instructionDMULTU(RS, RT);
  case 0x1e: return instructionDDIV(RS, RT);
  case 0x1f: return instructionDDIVU(RS, RT);
  case 0x20: return instructionADD(RD, RS, RT);
  case 0x21: return instructionADDU(RD, RS, RT);
  case 0x22: return instructionSUB(RD, RS, RT);
  case 0x23: return instructionSUBU(RD, RS, RT);
  case 0x24: return instructionAND(RD, RS, RT);
  case 0x25: return instructionOR(RD, RS, RT);
  case 0x26: return instructionXOR(RD, RS, RT);
  case 0x27: return instructionNOR(RD, RS, RT);
  case 0x28: break;
  case 0x29: break;
  case 0x2a: return instructionSLT(RD, RS, RT);
  case 0x2b: return instructionSLTU(RD, RS, RT);
  case 0x2c: return instructionDADD(RD, RS, RT);
  case 0x2d: return instructionDADDU(RD, RS, RT);
  case 0x2e: return instructionDSUB(RD, RS, RT);
  case 0x2f: return instructionDSUBU(RD, RS, RT);
  case 0x30: return instructionTGE(RS, RT);
  case 0x31: return instructionTGEU(RS, RT);
  case 0x32: return instructionTLT(RS, RT);
  case 0x33: return instructionTLTU(RS, RT);
  case 0x34: return instructionTEQ(RS, RT);
  case 0x35: break;
  case 0x36: return instructionTNE(RS, RT);
  case 0x37: break;
  case 0x38: return instructionDSLL(RD, RT, SA);
  case 0x39: break;
  case 0x3a: return instructionDSRL(RD, RT, SA);
  case 0x3b: return instructionDSRA(RD, RT, SA);
  case 0x3c: return instructionDSLL(RD, RT, SA + 32);
  case 0x3d: break;
  case 0x3e: return instructionDSRL(RD, RT, SA + 32);
  case 0x3f: return instructionDSRA(RD, RT, SA + 32);
  }

  exception.reservedInstruction();
}

auto CPU::instructionREGIMM() -> void {
  switch(OP >> 16 & 0x1f) {
  case 0x00: return instructionBLTZ(RS, IMMi16);
  case 0x01: return instructionBGEZ(RS, IMMi16);
  case 0x02: return instructionBLTZL(RS, IMMi16);
  case 0x03: return instructionBGEZL(RS, IMMi16);
  case 0x04: break;
  case 0x05: break;
  case 0x06: break;
  case 0x07: break;
  case 0x08: return instructionTGEI(RS, IMMi16);
  case 0x09: return instructionTGEIU(RS, IMMi16);
  case 0x0a: return instructionTLTI(RS, IMMi16);
  case 0x0b: return instructionTLTIU(RS, IMMi16);
  case 0x0c: return instructionTEQI(RS, IMMi16);
  case 0x0d: break;
  case 0x0e: return instructionTNEI(RS, IMMi16);
  case 0x0f: break;
  case 0x10: return instructionBLTZAL(RS, IMMi16);
  case 0x11: return instructionBGEZAL(RS, IMMi16);
  case 0x12: return instructionBLTZALL(RS, IMMi16);
  case 0x13: return instructionBGEZALL(RS, IMMi16);
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

  exception.reservedInstruction();
}

auto CPU::instructionCOP0() -> void {
  if(!scc.status.enable.coprocessor0) {
    if(context.mode != Context::Mode::Kernel) return exception.coprocessor0();
  }

  switch(OP >> 21 & 0x1f) {
  case 0x00: return instructionMFC0(RT, RDn);
  case 0x01: return instructionDMFC0(RT, RDn);
  case 0x02: return exception.reservedInstruction();  //CFC0
  case 0x03: return exception.reservedInstruction();
  case 0x04: return instructionMTC0(RT, RDn);
  case 0x05: return instructionDMTC0(RT, RDn);
  case 0x06: return exception.reservedInstruction();  //CTC0
  case 0x07: return exception.reservedInstruction();
  case 0x08: return exception.reservedInstruction();  //BC0
  case 0x09: return exception.reservedInstruction();
  case 0x0a: return exception.reservedInstruction();
  case 0x0b: return exception.reservedInstruction();
  case 0x0c: return exception.reservedInstruction();
  case 0x0d: return exception.reservedInstruction();
  case 0x0e: return exception.reservedInstruction();
  case 0x0f: return exception.reservedInstruction();
  }

  switch(OP & 0x3f) {
  case 0x01: return instructionTLBR();
  case 0x02: return instructionTLBWI();
  case 0x06: return instructionTLBWR();
  case 0x08: return instructionTLBP();
  case 0x18: return instructionERET();
  }

//undefined instructions do not throw a reserved instruction exception
}

auto CPU::instructionCOP1() -> void {
  if(!scc.status.enable.coprocessor1) {
    return exception.coprocessor1();
  }

  switch(OP >> 21 & 0x1f) {
  case 0x00: return instructionMFC1(RT, FS);
  case 0x01: return instructionDMFC1(RT, FS);
  case 0x02: return instructionCFC1(RT, RDn);
  case 0x03: return exception.reservedInstruction();
  case 0x04: return instructionMTC1(RT, FS);
  case 0x05: return instructionDMTC1(RT, FS);
  case 0x06: return instructionCTC1(RT, RDn);
  case 0x07: return exception.reservedInstruction();
  case 0x08: return instructionBC1(OP >> 16 & 1, OP >> 17 & 1, IMMi16);
  case 0x09: return exception.reservedInstruction();
  case 0x0a: return exception.reservedInstruction();
  case 0x0b: return exception.reservedInstruction();
  case 0x0c: return exception.reservedInstruction();
  case 0x0d: return exception.reservedInstruction();
  case 0x0e: return exception.reservedInstruction();
  case 0x0f: return exception.reservedInstruction();
  }

  if((OP >> 21 & 31) == 16)
  switch(OP & 0x3f) {
  case 0x00: return instructionFADD_S(FD, FS, FT);
  case 0x01: return instructionFSUB_S(FD, FS, FT);
  case 0x02: return instructionFMUL_S(FD, FS, FT);
  case 0x03: return instructionFDIV_S(FD, FS, FT);
  case 0x04: return instructionFSQRT_S(FD, FS);
  case 0x05: return instructionFABS_S(FD, FS);
  case 0x06: return instructionFMOV_S(FD, FS);
  case 0x07: return instructionFNEG_S(FD, FS);
  case 0x08: return instructionFROUND_L_S(FD, FS);
  case 0x09: return instructionFTRUNC_L_S(FD, FS);
  case 0x0a: return instructionFCEIL_L_S(FD, FS);
  case 0x0b: return instructionFFLOOR_L_S(FD, FS);
  case 0x0c: return instructionFROUND_W_S(FD, FS);
  case 0x0d: return instructionFTRUNC_W_S(FD, FS);
  case 0x0e: return instructionFCEIL_W_S(FD, FS);
  case 0x0f: return instructionFFLOOR_W_S(FD, FS);
  case 0x21: return instructionFCVT_D_S(FD, FS);
  case 0x24: return instructionFCVT_W_S(FD, FS);
  case 0x25: return instructionFCVT_L_S(FD, FS);
  case 0x30: case 0x38: return instructionFCF();
  case 0x31: case 0x39: return instructionFCUN();
  case 0x32: case 0x3a: return instructionFCEQ_S(FS, FT);
  case 0x33: case 0x3b: return instructionFCUEQ_S(FS, FT);
  case 0x34: case 0x3c: return instructionFCOLT_S(FS, FT);
  case 0x35: case 0x3d: return instructionFCULT_S(FS, FT);
  case 0x36: case 0x3e: return instructionFCOLE_S(FS, FT);
  case 0x37: case 0x3f: return instructionFCULE_S(FS, FT);
  }

  if((OP >> 21 & 31) == 17)
  switch(OP & 0x3f) {
  case 0x00: return instructionFADD_D(FD, FS, FT);
  case 0x01: return instructionFSUB_D(FD, FS, FT);
  case 0x02: return instructionFMUL_D(FD, FS, FT);
  case 0x03: return instructionFDIV_D(FD, FS, FT);
  case 0x04: return instructionFSQRT_D(FD, FS);
  case 0x05: return instructionFABS_D(FD, FS);
  case 0x06: return instructionFMOV_D(FD, FS);
  case 0x07: return instructionFNEG_D(FD, FS);
  case 0x08: return instructionFROUND_L_D(FD, FS);
  case 0x09: return instructionFTRUNC_L_D(FD, FS);
  case 0x0a: return instructionFCEIL_L_D(FD, FS);
  case 0x0b: return instructionFFLOOR_L_D(FD, FS);
  case 0x0c: return instructionFROUND_W_D(FD, FS);
  case 0x0d: return instructionFTRUNC_W_D(FD, FS);
  case 0x0e: return instructionFCEIL_W_D(FD, FS);
  case 0x0f: return instructionFFLOOR_W_D(FD, FS);
  case 0x20: return instructionFCVT_S_D(FD, FS);
  case 0x24: return instructionFCVT_W_D(FD, FS);
  case 0x25: return instructionFCVT_L_D(FD, FS);
  case 0x30: case 0x38: return instructionFCF();
  case 0x31: case 0x39: return instructionFCUN();
  case 0x32: case 0x3a: return instructionFCEQ_D(FS, FT);
  case 0x33: case 0x3b: return instructionFCUEQ_D(FS, FT);
  case 0x34: case 0x3c: return instructionFCOLT_D(FS, FT);
  case 0x35: case 0x3d: return instructionFCULT_D(FS, FT);
  case 0x36: case 0x3e: return instructionFCOLE_D(FS, FT);
  case 0x37: case 0x3f: return instructionFCULE_D(FS, FT);
  }

  if((OP >> 21 & 31) == 20)
  switch(OP & 0x3f) {
  case 0x20: return instructionFCVT_S_W(FD, FS);
  case 0x21: return instructionFCVT_D_W(FD, FS);
  }

  if((OP >> 21 & 31) == 21)
  switch(OP & 0x3f) {
  case 0x20: return instructionFCVT_S_L(FD, FS);
  case 0x21: return instructionFCVT_D_L(FD, FS);
  }

//undefined instructions do not throw a reserved instruction exception
}

#undef OP
#undef SA
#undef RDn
#undef RTn
#undef RSn
#undef RD
#undef RT
#undef RS
#undef FD
#undef FS
#undef FT
#undef IMMi16
#undef IMMu16
#undef IMMu26
