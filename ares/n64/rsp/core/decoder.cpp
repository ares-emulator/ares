#define OP     pipeline.instruction
#define SA     (OP >>  6 & 31)
#define RDn    (OP >> 11 & 31)
#define RTn    (OP >> 16 & 31)
#define RSn    (OP >> 21 & 31)
#define VDn    (OP >>  6 & 31)
#define VSn    (OP >> 11 & 31)
#define VTn    (OP >> 16 & 31)
#define RD     GPR[RDn]
#define RT     GPR[RTn]
#define RS     GPR[RSn]
#define VD     VPR[VDn]
#define VS     VPR[VSn]
#define VT     VPR[VTn]
#define IMMi16 i16(OP)
#define IMMu16 u16(OP)
#define IMMu26 (OP & 0x03ff'ffff)

auto RSP::instructionEXECUTE() -> void {
  switch(OP >> 26) {
  case 0x00: return instructionSPECIAL();
  case 0x01: return instructionREGIMM();
  case 0x02: return instructionJ(IMMu26);
  case 0x03: return instructionJAL(IMMu26);
  case 0x04: return instructionBEQ(RS, RT, IMMi16);
  case 0x05: return instructionBNE(RS, RT, IMMi16);
  case 0x06: return instructionBLEZ(RS, IMMi16);
  case 0x07: return instructionBGTZ(RS, IMMi16);
  case 0x08: return instructionADDIU(RT, RS, IMMi16);  //ADDI
  case 0x09: return instructionADDIU(RT, RS, IMMi16);
  case 0x0a: return instructionSLTI(RT, RS, IMMi16);
  case 0x0b: return instructionSLTIU(RT, RS, IMMi16);
  case 0x0c: return instructionANDI(RT, RS, IMMu16);
  case 0x0d: return instructionORI(RT, RS, IMMu16);
  case 0x0e: return instructionXORI(RT, RS, IMMu16);
  case 0x0f: return instructionLUI(RT, IMMu16);
  case 0x10: return instructionCOP0();
  case 0x11: break;  //COP1
  case 0x12: return instructionCOP2();
  case 0x13: break;  //COP3
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
  case 0x20: return instructionLB(RT, RS, IMMi16);
  case 0x21: return instructionLH(RT, RS, IMMi16);
  case 0x22: break;  //LWL
  case 0x23: return instructionLW(RT, RS, IMMi16);
  case 0x24: return instructionLBU(RT, RS, IMMi16);
  case 0x25: return instructionLHU(RT, RS, IMMi16);
  case 0x26: break;  //LWR
  case 0x27: break;  //LWU
  case 0x28: return instructionSB(RT, RS, IMMi16);
  case 0x29: return instructionSH(RT, RS, IMMi16);
  case 0x2a: break;  //SWL
  case 0x2b: return instructionSW(RT, RS, IMMi16);
  case 0x2c: break;  //SDL
  case 0x2d: break;  //SDR
  case 0x2e: break;  //SWR
  case 0x2f: return instructionCACHE(OP >> 16 & 3, OP >> 18 & 3);
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
}

auto RSP::instructionSPECIAL() -> void {
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
  case 0x0c: break;  //SYSCALL
  case 0x0d: return instructionBREAK();
  case 0x0e: break;
  case 0x0f: break;  //SYNC
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
  case 0x20: return instructionADDU(RD, RS, RT);  //ADD
  case 0x21: return instructionADDU(RD, RS, RT);
  case 0x22: return instructionSUBU(RD, RS, RT);  //SUB
  case 0x23: return instructionSUBU(RD, RS, RT);
  case 0x24: return instructionAND(RD, RS, RT);
  case 0x25: return instructionOR(RD, RS, RT);
  case 0x26: return instructionXOR(RD, RS, RT);
  case 0x27: return instructionNOR(RD, RS, RT);
  case 0x28: break;
  case 0x29: break;
  case 0x2a: return instructionSLT(RD, RS, RT);
  case 0x2b: return instructionSLTU(RD, RS, RT);
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
}

auto RSP::instructionREGIMM() -> void {
  switch(OP >> 16 & 0x1f) {
  case 0x00: return instructionBLTZ(RS, IMMi16);
  case 0x01: return instructionBGEZ(RS, IMMi16);
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
  case 0x10: return instructionBLTZAL(RS, IMMi16);
  case 0x11: return instructionBGEZAL(RS, IMMi16);
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
}

auto RSP::instructionCOP0() -> void {
  switch(OP >> 21 & 31) {
  case 0x00: return instructionMFC0(RT, RDn);
  case 0x01: return;  //DMFC0
  case 0x02: return;  //CFC0
  case 0x03: return;
  case 0x04: return instructionMTC0(RT, RDn);
  case 0x05: return;  //DMTC0
  case 0x06: return;  //CTC0
  case 0x07: return;
  case 0x08: return;  //BC0
  case 0x09: return;
  case 0x0a: return;
  case 0x0b: return;
  case 0x0c: return;
  case 0x0d: return;
  case 0x0e: return;
  case 0x0f: return;
  }
}

auto RSP::instructionCOP2() -> void {
  #define E (OP >> 7 & 15)
  switch(OP >> 21 & 31) {
  case 0x00: return instructionMFC2(RT, VS, E);
  case 0x01: return;  //DMFC2
  case 0x02: return instructionCFC2(RT, RDn);
  case 0x03: return;
  case 0x04: return instructionMTC2(RT, VS, E);
  case 0x05: return;  //DMTC2
  case 0x06: return instructionCTC2(RT, RDn);
  case 0x07: return;
  case 0x08: return;  //BC2
  case 0x09: return;
  case 0x0a: return;
  case 0x0b: return;
  case 0x0c: return;
  case 0x0d: return;
  case 0x0e: return;
  case 0x0f: return;
  }
  #undef E

  #define E  (OP >> 21 & 15)
  #define DE (OP >> 11 & 31)
  switch(OP & 0x3f) {
  case 0x00: return instructionVMULF(0, VD, VS, VT, E);
  case 0x01: return instructionVMULF(1, VD, VS, VT, E);
  case 0x02: return instructionVRND(1, VD, VSn, VT, E);
  case 0x03: return instructionVMULQ(VD, VS, VT, E);
  case 0x04: return instructionVMUDL(VD, VS, VT, E);
  case 0x05: return instructionVMUDM(VD, VS, VT, E);
  case 0x06: return instructionVMUDN(VD, VS, VT, E);
  case 0x07: return instructionVMUDH(VD, VS, VT, E);
  case 0x08: return instructionVMACF(0, VD, VS, VT, E);
  case 0x09: return instructionVMACF(1, VD, VS, VT, E);
  case 0x0a: return instructionVRND(0, VD, VSn, VT, E);
  case 0x0b: return instructionVMACQ(VD);
  case 0x0c: return instructionVMADL(VD, VS, VT, E);
  case 0x0d: return instructionVMADM(VD, VS, VT, E);
  case 0x0e: return instructionVMADN(VD, VS, VT, E);
  case 0x0f: return instructionVMADH(VD, VS, VT, E);
  case 0x10: return instructionVADD(VD, VS, VT, E);
  case 0x11: return instructionVSUB(VD, VS, VT, E);
  case 0x12: break;
  case 0x13: return instructionVABS(VD, VS, VT, E);
  case 0x14: return instructionVADDC(VD, VS, VT, E);
  case 0x15: return instructionVSUBC(VD, VS, VT, E);
  case 0x16: break;
  case 0x17: break;
  case 0x18: break;
  case 0x19: break;
  case 0x1a: break;
  case 0x1b: break;
  case 0x1c: break;
  case 0x1d: return instructionVSAR(VD, VS, E);
  case 0x1e: break;
  case 0x1f: break;
  case 0x20: return instructionVLT(VD, VS, VT, E);
  case 0x21: return instructionVEQ(VD, VS, VT, E);
  case 0x22: return instructionVNE(VD, VS, VT, E);
  case 0x23: return instructionVGE(VD, VS, VT, E);
  case 0x24: return instructionVCL(VD, VS, VT, E);
  case 0x25: return instructionVCH(VD, VS, VT, E);
  case 0x26: return instructionVCR(VD, VS, VT, E);
  case 0x27: return instructionVMRG(VD, VS, VT, E);
  case 0x28: return instructionVAND(VD, VS, VT, E);
  case 0x29: return instructionVNAND(VD, VS, VT, E);
  case 0x2a: return instructionVOR(VD, VS, VT, E);
  case 0x2b: return instructionVNOR(VD, VS, VT, E);
  case 0x2c: return instructionVXOR(VD, VS, VT, E);
  case 0x2d: return instructionVNXOR(VD, VS, VT, E);
  case 0x2e: break;
  case 0x2f: break;
  case 0x30: return instructionVRCP(0, VD, DE, VT, E);
  case 0x31: return instructionVRCP(1, VD, DE, VT, E);
  case 0x32: return instructionVRCPH(VD, DE, VT, E);
  case 0x33: return instructionVMOV(VD, DE, VT, E);
  case 0x34: return instructionVRSQ(0, VD, DE, VT, E);
  case 0x35: return instructionVRSQ(1, VD, DE, VT, E);
  case 0x36: return instructionVRSQH(VD, DE, VT, E);
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
  #undef E
  #undef DE
}

#define E     (OP >> 7 & 15)
#define IMMi7 int7(OP)

auto RSP::instructionLWC2() -> void {
  switch(OP >> 11 & 31) {
  case 0x00: return instructionLBV(VT, E, RS, IMMi7);
  case 0x01: return instructionLSV(VT, E, RS, IMMi7);
  case 0x02: return instructionLLV(VT, E, RS, IMMi7);
  case 0x03: return instructionLDV(VT, E, RS, IMMi7);
  case 0x04: return instructionLQV(VT, E, RS, IMMi7);
  case 0x05: return instructionLRV(VT, E, RS, IMMi7);
  case 0x06: return instructionLPV(VT, E, RS, IMMi7);
  case 0x07: return instructionLUV(VT, E, RS, IMMi7);
  case 0x08: return instructionLHV(VT, E, RS, IMMi7);
  case 0x09: return instructionLFV(VT, E, RS, IMMi7);
//case 0x0a: return instructionLWV(VT, E, RS, IMMi7);  //not present on N64 RSP
  case 0x0b: return instructionLTV(VTn, E, RS, IMMi7);
  }
}

auto RSP::instructionSWC2() -> void {
  switch(OP >> 11 & 31) {
  case 0x00: return instructionSBV(VT, E, RS, IMMi7);
  case 0x01: return instructionSSV(VT, E, RS, IMMi7);
  case 0x02: return instructionSLV(VT, E, RS, IMMi7);
  case 0x03: return instructionSDV(VT, E, RS, IMMi7);
  case 0x04: return instructionSQV(VT, E, RS, IMMi7);
  case 0x05: return instructionSRV(VT, E, RS, IMMi7);
  case 0x06: return instructionSPV(VT, E, RS, IMMi7);
  case 0x07: return instructionSUV(VT, E, RS, IMMi7);
  case 0x08: return instructionSHV(VT, E, RS, IMMi7);
  case 0x09: return instructionSFV(VT, E, RS, IMMi7);
  case 0x0a: return instructionSWV(VT, E, RS, IMMi7);
  case 0x0b: return instructionSTV(VTn, E, RS, IMMi7);
  }
}

#undef E
#undef IMMi7

#undef OP
#undef SA
#undef RDn
#undef RTn
#undef RSn
#undef VDn
#undef VSn
#undef VTn
#undef RD
#undef RT
#undef RS
#undef VD
#undef VS
#undef VT
#undef IMMi16
#undef IMMu16
#undef IMMu26
