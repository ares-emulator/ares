#define OP pipeline.instruction
#define RD ipu.r[RDn]
#define RT ipu.r[RTn]
#define RS ipu.r[RSn]
#define VD vpu.r[VDn]
#define VS vpu.r[VSn]
#define VT vpu.r[VTn]

#define jp(id, name, ...) case id: return decoder##name(__VA_ARGS__)
#define op(id, flags, name, ...) case id: pipeline flags; return name(__VA_ARGS__)
#define br(id, flags, name, ...) case id: pipeline flags; return name(__VA_ARGS__)
#define vu(id, flags, name, ...) case id: pipeline flags; \
  switch(E) { \
  case 0x0: return name<0x0>(__VA_ARGS__); \
  case 0x1: return name<0x1>(__VA_ARGS__); \
  case 0x2: return name<0x2>(__VA_ARGS__); \
  case 0x3: return name<0x3>(__VA_ARGS__); \
  case 0x4: return name<0x4>(__VA_ARGS__); \
  case 0x5: return name<0x5>(__VA_ARGS__); \
  case 0x6: return name<0x6>(__VA_ARGS__); \
  case 0x7: return name<0x7>(__VA_ARGS__); \
  case 0x8: return name<0x8>(__VA_ARGS__); \
  case 0x9: return name<0x9>(__VA_ARGS__); \
  case 0xa: return name<0xa>(__VA_ARGS__); \
  case 0xb: return name<0xb>(__VA_ARGS__); \
  case 0xc: return name<0xc>(__VA_ARGS__); \
  case 0xd: return name<0xd>(__VA_ARGS__); \
  case 0xe: return name<0xe>(__VA_ARGS__); \
  case 0xf: return name<0xf>(__VA_ARGS__); \
  } \
  unreachable;

#define SA     (OP >>  6 & 31)
#define RDn    (OP >> 11 & 31)
#define RTn    (OP >> 16 & 31)
#define RSn    (OP >> 21 & 31)
#define VDn    (OP >>  6 & 31)
#define VSn    (OP >> 11 & 31)
#define VTn    (OP >> 16 & 31)
#define IMMi16 s16(OP)
#define IMMu16 u16(OP)
#define IMMu26 (OP & 0x03ff'ffff)

#define R .regRead
#define W .regWrite
#define L .load()
#define S .store()

auto RSP::decoderEXECUTE() -> void {
  switch(OP >> 26) {
  jp(0x00, SPECIAL);
  jp(0x01, REGIMM);
  br(0x02,                , J, IMMu26);
  br(0x03,                , JAL, IMMu26);
  br(0x04, R(RSn) R(RTn)  , BEQ, RS, RT, IMMi16);
  br(0x05, R(RSn) R(RTn)  , BNE, RS, RT, IMMi16);
  br(0x06, R(RSn)         , BLEZ, RS, IMMi16);
  br(0x07, R(RSn)         , BGTZ, RS, IMMi16);
  op(0x08, R(RSn)         , ADDIU, RT, RS, IMMi16);  //ADDI
  op(0x09, R(RSn)         , ADDIU, RT, RS, IMMi16);
  op(0x0a, R(RSn)         , SLTI, RT, RS, IMMi16);
  op(0x0b, R(RSn)         , SLTIU, RT, RS, IMMi16);
  op(0x0c, R(RSn)         , ANDI, RT, RS, IMMu16);
  op(0x0d, R(RSn)         , ORI, RT, RS, IMMu16);
  op(0x0e, R(RSn)         , XORI, RT, RS, IMMu16);
  op(0x0f,                , LUI, RT, IMMu16);
  jp(0x10, SCC);
  op(0x11,                , INVALID);  //COP1
  jp(0x12, VU);
  op(0x13,                , INVALID);  //COP3
  op(0x14,                , INVALID);  //BEQL
  op(0x15,                , INVALID);  //BNEL
  op(0x16,                , INVALID);  //BLEZL
  op(0x17,                , INVALID);  //BGTZL
  op(0x18,                , INVALID);  //DADDI
  op(0x19,                , INVALID);  //DADDIU
  op(0x1a,                , INVALID);  //LDL
  op(0x1b,                , INVALID);  //LDR
  op(0x1c,                , INVALID);
  op(0x1d,                , INVALID);
  op(0x1e,                , INVALID);
  op(0x1f,                , INVALID);
  op(0x20, R(RSn) W(RTn) L, LB, RT, RS, IMMi16);
  op(0x21, R(RSn) W(RTn) L, LH, RT, RS, IMMi16);
  op(0x22,                , INVALID);  //LWL
  op(0x23, R(RSn) W(RTn) L, LW, RT, RS, IMMi16);
  op(0x24, R(RSn) W(RTn) L, LBU, RT, RS, IMMi16);
  op(0x25, R(RSn) W(RTn) L, LHU, RT, RS, IMMi16);
  op(0x26,                , INVALID);  //LWR
  op(0x27, R(RSn) W(RTn) L, LWU, RT, RS, IMMi16);
  op(0x28, R(RTn) R(RSn) S, SB, RT, RS, IMMi16);
  op(0x29, R(RTn) R(RSn) S, SH, RT, RS, IMMi16);
  op(0x2a,                , INVALID);  //SWL
  op(0x2b, R(RTn) R(RSn) S, SW, RT, RS, IMMi16);
  op(0x2c,                , INVALID);  //SDL
  op(0x2d,                , INVALID);  //SDR
  op(0x2e,                , INVALID);  //SWR
  op(0x2f,                , INVALID);  //CACHE
  op(0x30,                , INVALID);  //LL
  op(0x31,                , INVALID);  //LWC1
  jp(0x32, LWC2);
  op(0x33,                , INVALID);  //LWC3
  op(0x34,                , INVALID);  //LLD
  op(0x35,                , INVALID);  //LDC1
  op(0x36,                , INVALID);  //LDC2
  op(0x37,                , INVALID);  //LD
  op(0x38,                , INVALID);  //SC
  op(0x39,                , INVALID);  //SWC1
  jp(0x3a, SWC2);
  op(0x3b,                , INVALID);  //SWC3
  op(0x3c,                , INVALID);  //SCD
  op(0x3d,                , INVALID);  //SDC1
  op(0x3e,                , INVALID);  //SDC2
  op(0x3f,                , INVALID);  //SD
  }
}

auto RSP::decoderSPECIAL() -> void {
  switch(OP & 0x3f) {
  op(0x00, R(RTn)       , SLL, RD, RT, SA);
  op(0x01,              , INVALID);
  op(0x02, R(RTn)       , SRL, RD, RT, SA);
  op(0x03, R(RTn)       , SRA, RD, RT, SA);
  op(0x04, R(RTn) R(RSn), SLLV, RD, RT, RS);
  op(0x05,              , INVALID);
  op(0x06, R(RTn) R(RSn), SRLV, RD, RT, RS);
  op(0x07, R(RTn) R(RSn), SRAV, RD, RT, RS);
  br(0x08, R(RSn)       , JR, RS);
  br(0x09, R(RSn)       , JALR, RD, RS);
  op(0x0a,              , INVALID);
  op(0x0b,              , INVALID);
  op(0x0c,              , INVALID);  //SYSCALL
  br(0x0d,              , BREAK);
  op(0x0e,              , INVALID);
  op(0x0f,              , INVALID);  //SYNC
  op(0x10,              , INVALID);  //MFHI
  op(0x11,              , INVALID);  //MTHI
  op(0x12,              , INVALID);  //MFLO
  op(0x13,              , INVALID);  //MTLO
  op(0x14,              , INVALID);  //DSLLV
  op(0x15,              , INVALID);
  op(0x16,              , INVALID);  //DSRLV
  op(0x17,              , INVALID);  //DSRAV
  op(0x18,              , INVALID);  //MULT
  op(0x19,              , INVALID);  //MULTU
  op(0x1a,              , INVALID);  //DIV
  op(0x1b,              , INVALID);  //DIVU
  op(0x1c,              , INVALID);  //DMULT
  op(0x1d,              , INVALID);  //DMULTU
  op(0x1e,              , INVALID);  //DDIV
  op(0x1f,              , INVALID);  //DDIVU
  op(0x20, R(RSn) R(RTn), ADDU, RD, RS, RT);  //ADD
  op(0x21, R(RSn) R(RTn), ADDU, RD, RS, RT);
  op(0x22, R(RSn) R(RTn), SUBU, RD, RS, RT);  //SUB
  op(0x23, R(RSn) R(RTn), SUBU, RD, RS, RT);
  op(0x24, R(RSn) R(RTn), AND, RD, RS, RT);
  op(0x25, R(RSn) R(RTn), OR, RD, RS, RT);
  op(0x26, R(RSn) R(RTn), XOR, RD, RS, RT);
  op(0x27, R(RSn) R(RTn), NOR, RD, RS, RT);
  op(0x28,              , INVALID);
  op(0x29,              , INVALID);
  op(0x2a, R(RSn) R(RTn), SLT, RD, RS, RT);
  op(0x2b, R(RSn) R(RTn), SLTU, RD, RS, RT);
  op(0x2c,              , INVALID);  //DADD
  op(0x2d,              , INVALID);  //DADDU
  op(0x2e,              , INVALID);  //DSUB
  op(0x2f,              , INVALID);  //DSUBU
  op(0x30,              , INVALID);  //TGE
  op(0x31,              , INVALID);  //TGEU
  op(0x32,              , INVALID);  //TLT
  op(0x33,              , INVALID);  //TLTU
  op(0x34,              , INVALID);  //TEQ
  op(0x35,              , INVALID);
  op(0x36,              , INVALID);  //TNE
  op(0x37,              , INVALID);
  op(0x38,              , INVALID);  //DSLL
  op(0x39,              , INVALID);
  op(0x3a,              , INVALID);  //DSRL
  op(0x3b,              , INVALID);  //DSRA
  op(0x3c,              , INVALID);  //DSLL32
  op(0x3d,              , INVALID);
  op(0x3e,              , INVALID);  //DSRL32
  op(0x3f,              , INVALID);  //DSRA32
  }
}

auto RSP::decoderREGIMM() -> void {
  switch(OP >> 16 & 0x1f) {
  br(0x00, R(RSn), BLTZ, RS, IMMi16);
  br(0x01, R(RSn), BGEZ, RS, IMMi16);
  op(0x02,       , INVALID);  //BLTZL
  op(0x03,       , INVALID);  //BGEZL
  op(0x04,       , INVALID);
  op(0x05,       , INVALID);
  op(0x06,       , INVALID);
  op(0x07,       , INVALID);
  op(0x08,       , INVALID);  //TGEI
  op(0x09,       , INVALID);  //TGEIU
  op(0x0a,       , INVALID);  //TLTI
  op(0x0b,       , INVALID);  //TLTIU
  op(0x0c,       , INVALID);  //TEQI
  op(0x0d,       , INVALID);
  op(0x0e,       , INVALID);  //TNEI
  op(0x0f,       , INVALID);
  br(0x10, R(RSn), BLTZAL, RS, IMMi16);
  br(0x11, R(RSn), BGEZAL, RS, IMMi16);
  op(0x12,       , INVALID);  //BLTZALL
  op(0x13,       , INVALID);  //BGEZALL
  op(0x14,       , INVALID);
  op(0x15,       , INVALID);
  op(0x16,       , INVALID);
  op(0x17,       , INVALID);
  op(0x18,       , INVALID);
  op(0x19,       , INVALID);
  op(0x1a,       , INVALID);
  op(0x1b,       , INVALID);
  op(0x1c,       , INVALID);
  op(0x1d,       , INVALID);
  op(0x1e,       , INVALID);
  op(0x1f,       , INVALID);
  }
}

auto RSP::decoderSCC() -> void {
  switch(OP >> 21 & 0x1f) {
  op(0x00, W(RTn) L S, MFC0, RT, RDn);
  op(0x01,           , INVALID);  //DMFC0
  op(0x02,           , INVALID);  //CFC0
  op(0x03,           , INVALID);
  op(0x04, R(RTn) L S, MTC0, RT, RDn);
  op(0x05,           , INVALID);  //DMTC0
  op(0x06,           , INVALID);  //CTC0
  op(0x07,           , INVALID);
  op(0x08,           , INVALID);  //BC0
  op(0x09,           , INVALID);
  op(0x0a,           , INVALID);
  op(0x0b,           , INVALID);
  op(0x0c,           , INVALID);
  op(0x0d,           , INVALID);
  op(0x0e,           , INVALID);
  op(0x0f,           , INVALID);
  }
}

auto RSP::decoderVU() -> void {
  #define E (OP >> 7 & 15)
  switch(OP >> 21 & 0x1f) {
  vu(0x00, W(RTn) L S, MFC2, RT, VS);
  op(0x01,           , INVALID);  //DMFC2
  op(0x02, W(RTn) L S, CFC2, RT, RDn);
  op(0x03,           , INVALID);
  vu(0x04, R(RTn) L S, MTC2, RT, VS);
  op(0x05,           , INVALID);  //DMTC2
  op(0x06, R(RTn) L S, CTC2, RT, RDn);
  op(0x07,           , INVALID);
  op(0x08,           , INVALID);  //BC2
  op(0x09,           , INVALID);
  op(0x0a,           , INVALID);
  op(0x0b,           , INVALID);
  op(0x0c,           , INVALID);
  op(0x0d,           , INVALID);
  op(0x0e,           , INVALID);
  op(0x0f,           , INVALID);
  }
  #undef E

  #define E  (OP >> 21 & 15)
  #define DE (OP >> 11 &  7)
  switch(OP & 0x3f) {
  vu(0x00, , VMULF, VD, VS, VT);
  vu(0x01, , VMULU, VD, VS, VT);
  vu(0x02, , VRNDP, VD, VSn, VT);
  vu(0x03, , VMULQ, VD, VS, VT);
  vu(0x04, , VMUDL, VD, VS, VT);
  vu(0x05, , VMUDM, VD, VS, VT);
  vu(0x06, , VMUDN, VD, VS, VT);
  vu(0x07, , VMUDH, VD, VS, VT);
  vu(0x08, , VMACF, VD, VS, VT);
  vu(0x09, , VMACU, VD, VS, VT);
  vu(0x0a, , VRNDN, VD, VSn, VT);
  op(0x0b, , VMACQ, VD);
  vu(0x0c, , VMADL, VD, VS, VT);
  vu(0x0d, , VMADM, VD, VS, VT);
  vu(0x0e, , VMADN, VD, VS, VT);
  vu(0x0f, , VMADH, VD, VS, VT);
  vu(0x10, , VADD, VD, VS, VT);
  vu(0x11, , VSUB, VD, VS, VT);
  vu(0x12, , VZERO, VD, VS, VT); //VSUT
  vu(0x13, , VABS, VD, VS, VT);
  vu(0x14, , VADDC, VD, VS, VT);
  vu(0x15, , VSUBC, VD, VS, VT);
  vu(0x16, , VZERO, VD, VS, VT); //VADDB
  vu(0x17, , VZERO, VD, VS, VT); //VSUBB
  vu(0x18, , VZERO, VD, VS, VT); //VACCB
  vu(0x19, , VZERO, VD, VS, VT); //VSUCB
  vu(0x1a, , VZERO, VD, VS, VT); //VSAD
  vu(0x1b, , VZERO, VD, VS, VT); //VSAC
  vu(0x1c, , VZERO, VD, VS, VT); //VSUM
  vu(0x1d, , VSAR, VD, VS);
  vu(0x1e, , VZERO, VD, VS, VT);
  vu(0x1f, , VZERO, VD, VS, VT);
  vu(0x20, , VLT, VD, VS, VT);
  vu(0x21, , VEQ, VD, VS, VT);
  vu(0x22, , VNE, VD, VS, VT);
  vu(0x23, , VGE, VD, VS, VT);
  vu(0x24, , VCL, VD, VS, VT);
  vu(0x25, , VCH, VD, VS, VT);
  vu(0x26, , VCR, VD, VS, VT);
  vu(0x27, , VMRG, VD, VS, VT);
  vu(0x28, , VAND, VD, VS, VT);
  vu(0x29, , VNAND, VD, VS, VT);
  vu(0x2a, , VOR, VD, VS, VT);
  vu(0x2b, , VNOR, VD, VS, VT);
  vu(0x2c, , VXOR, VD, VS, VT);
  vu(0x2d, , VNXOR, VD, VS, VT);
  vu(0x2e, , VZERO, VD, VS, VT);
  vu(0x2f, , VZERO, VD, VS, VT);
  vu(0x30, , VRCP, VD, DE, VT);
  vu(0x31, , VRCPL, VD, DE, VT);
  vu(0x32, , VRCPH, VD, DE, VT);
  vu(0x33, , VMOV, VD, DE, VT);
  vu(0x34, , VRSQ, VD, DE, VT);
  vu(0x35, , VRSQL, VD, DE, VT);
  vu(0x36, , VRSQH, VD, DE, VT);
  op(0x37, , VNOP);
  vu(0x38, , VZERO, VD, VS, VT); //VEXTT
  vu(0x39, , VZERO, VD, VS, VT); //VEXTQ
  vu(0x3a, , VZERO, VD, VS, VT); //VEXTN
  vu(0x3b, , VZERO, VD, VS, VT);
  vu(0x3c, , VZERO, VD, VS, VT); //VINST
  vu(0x3d, , VZERO, VD, VS, VT); //VINSQ
  vu(0x3e, , VZERO, VD, VS, VT); //VINSN
  op(0x3f, , VNOP); //VNULL
  }
  #undef E
  #undef DE
}

auto RSP::decoderLWC2() -> void {
  #define E     (OP >> 7 & 15)
  #define IMMi7 i7(OP)
  switch(OP >> 11 & 0x1f) {
  vu(0x00, R(RSn) L, LBV, VT, RS, IMMi7);
  vu(0x01, R(RSn) L, LSV, VT, RS, IMMi7);
  vu(0x02, R(RSn) L, LLV, VT, RS, IMMi7);
  vu(0x03, R(RSn) L, LDV, VT, RS, IMMi7);
  vu(0x04, R(RSn) L, LQV, VT, RS, IMMi7);
  vu(0x05, R(RSn) L, LRV, VT, RS, IMMi7);
  vu(0x06, R(RSn) L, LPV, VT, RS, IMMi7);
  vu(0x07, R(RSn) L, LUV, VT, RS, IMMi7);
  vu(0x08, R(RSn) L, LHV, VT, RS, IMMi7);
  vu(0x09, R(RSn) L, LFV, VT, RS, IMMi7);
//vu(0x0a, R(RSn) L, LWV, VT, RS, IMMi7);  //not present on N64 RSP
  vu(0x0b, R(RSn) L, LTV, VTn, RS, IMMi7);
  }
  #undef E
  #undef IMMi7
}

auto RSP::decoderSWC2() -> void {
  #define E     (OP >> 7 & 15)
  #define IMMi7 i7(OP)
  switch(OP >> 11 & 0x1f) {
  vu(0x00, R(RSn) S, SBV, VT, RS, IMMi7);
  vu(0x01, R(RSn) S, SSV, VT, RS, IMMi7);
  vu(0x02, R(RSn) S, SLV, VT, RS, IMMi7);
  vu(0x03, R(RSn) S, SDV, VT, RS, IMMi7);
  vu(0x04, R(RSn) S, SQV, VT, RS, IMMi7);
  vu(0x05, R(RSn) S, SRV, VT, RS, IMMi7);
  vu(0x06, R(RSn) S, SPV, VT, RS, IMMi7);
  vu(0x07, R(RSn) S, SUV, VT, RS, IMMi7);
  vu(0x08, R(RSn) S, SHV, VT, RS, IMMi7);
  vu(0x09, R(RSn) S, SFV, VT, RS, IMMi7);
  vu(0x0a, R(RSn) S, SWV, VT, RS, IMMi7);
  vu(0x0b, R(RSn) S, STV, VTn, RS, IMMi7);
  }
  #undef E
  #undef IMMi7
}

auto RSP::INVALID() -> void {
}

#undef L
#undef S
#undef R
#undef W

#undef SA
#undef RDn
#undef RTn
#undef RSn
#undef VDn
#undef VSn
#undef VTn
#undef IMMi16
#undef IMMu16
#undef IMMu26

#undef jp
#undef op
#undef br

#undef OP
#undef RD
#undef RT
#undef RS
#undef VD
#undef VS
#undef VT
