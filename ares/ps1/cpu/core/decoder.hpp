//shares instruction decoding between interpreter and dynamic recompiler

#define SA     (OP >>  6 & 31)
#define RDn    (OP >> 11 & 31)
#define RTn    (OP >> 16 & 31)
#define RSn    (OP >> 21 & 31)
#define IMMi16 i16(OP)
#define IMMu16 u16(OP)
#define IMMu26 (OP & 0x03ff'ffff)

#ifdef DECODER_EXECUTE
{
  switch(OP >> 26) {
  jp(0x00, SPECIAL);
  jp(0x01, REGIMM);
  br(0x02, J, IMMu26);
  br(0x03, JAL, IMMu26);
  br(0x04, BEQ, RS, RT, IMMi16);
  br(0x05, BNE, RS, RT, IMMi16);
  br(0x06, BLEZ, RS, IMMi16);
  br(0x07, BGTZ, RS, IMMi16);
  op(0x08, ADDI, RT, RS, IMMi16);
  op(0x09, ADDIU, RT, RS, IMMi16);
  op(0x0a, SLTI, RT, RS, IMMi16);
  op(0x0b, SLTIU, RT, RS, IMMi16);
  op(0x0c, ANDI, RT, RS, IMMu16);
  op(0x0d, ORI, RT, RS, IMMu16);
  op(0x0e, XORI, RT, RS, IMMu16);
  op(0x0f, LUI, RT, IMMu16);
  jp(0x10, SCC);
  op(0x11, COP1);
  jp(0x12, GTE);
  op(0x13, COP3);
  op(0x14, INVALID);
  op(0x15, INVALID);
  op(0x16, INVALID);
  op(0x17, INVALID);
  op(0x18, INVALID);
  op(0x19, INVALID);
  op(0x1a, INVALID);
  op(0x1b, INVALID);
  op(0x1c, INVALID);
  op(0x1d, INVALID);
  op(0x1e, INVALID);
  op(0x1f, INVALID);
  op(0x20, LB, RT, RS, IMMi16);
  op(0x21, LH, RT, RS, IMMi16);
  op(0x22, LWL, RT, RS, IMMi16);
  op(0x23, LW, RT, RS, IMMi16);
  op(0x24, LBU, RT, RS, IMMi16);
  op(0x25, LHU, RT, RS, IMMi16);
  op(0x26, LWR, RT, RS, IMMi16);
  op(0x27, INVALID);
  op(0x28, SB, RT, RS, IMMi16);
  op(0x29, SH, RT, RS, IMMi16);
  op(0x2a, SWL, RT, RS, IMMi16);
  op(0x2b, SW, RT, RS, IMMi16);
  op(0x2c, INVALID);
  op(0x2d, INVALID);
  op(0x2e, SWR, RT, RS, IMMi16);
  op(0x2f, INVALID);
  op(0x30, INVALID);
  op(0x31, INVALID);
  op(0x32, LWC2, RTn, RS, IMMi16);
  op(0x33, INVALID);
  op(0x34, INVALID);
  op(0x35, INVALID);
  op(0x36, INVALID);
  op(0x37, INVALID);
  op(0x38, INVALID);
  op(0x39, INVALID);
  op(0x3a, SWC2, RTn, RS, IMMi16);
  op(0x3b, INVALID);
  op(0x3c, INVALID);
  op(0x3d, INVALID);
  op(0x3e, INVALID);
  op(0x3f, INVALID);
  }
}
#undef DECODER_EXECUTE
#endif

#ifdef DECODER_SPECIAL
{
  switch(OP & 0x3f) {
  op(0x00, SLL, RD, RT, SA);
  op(0x01, INVALID);
  op(0x02, SRL, RD, RT, SA);
  op(0x03, SRA, RD, RT, SA);
  op(0x04, SLLV, RD, RT, RS);
  op(0x05, INVALID);
  op(0x06, SRLV, RD, RT, RS);
  op(0x07, SRAV, RD, RT, RS);
  br(0x08, JR, RS);
  br(0x09, JALR, RD, RS);
  op(0x0a, INVALID);
  op(0x0b, INVALID);
  br(0x0c, SYSCALL);
  br(0x0d, BREAK);
  op(0x0e, INVALID);
  op(0x0f, INVALID);
  op(0x10, MFHI, RD);
  op(0x11, MTHI, RS);
  op(0x12, MFLO, RD);
  op(0x13, MTLO, RS);
  op(0x14, INVALID);
  op(0x15, INVALID);
  op(0x16, INVALID);
  op(0x17, INVALID);
  op(0x18, MULT, RS, RT);
  op(0x19, MULTU, RS, RT);
  op(0x1a, DIV, RS, RT);
  op(0x1b, DIVU, RS, RT);
  op(0x1c, INVALID);
  op(0x1d, INVALID);
  op(0x1e, INVALID);
  op(0x1f, INVALID);
  op(0x20, ADD, RD, RS, RT);
  op(0x21, ADDU, RD, RS, RT);
  op(0x22, SUB, RD, RS, RT);
  op(0x23, SUBU, RD, RS, RT);
  op(0x24, AND, RD, RS, RT);
  op(0x25, OR, RD, RS, RT);
  op(0x26, XOR, RD, RS, RT);
  op(0x27, NOR, RD, RS, RT);
  op(0x28, INVALID);
  op(0x29, INVALID);
  op(0x2a, SLT, RD, RS, RT);
  op(0x2b, SLTU, RD, RS, RT);
  op(0x2c, INVALID);
  op(0x2d, INVALID);
  op(0x2e, INVALID);
  op(0x2f, INVALID);
  op(0x30, INVALID);
  op(0x31, INVALID);
  op(0x32, INVALID);
  op(0x33, INVALID);
  op(0x34, INVALID);
  op(0x35, INVALID);
  op(0x36, INVALID);
  op(0x37, INVALID);
  op(0x38, INVALID);
  op(0x39, INVALID);
  op(0x3a, INVALID);
  op(0x3b, INVALID);
  op(0x3c, INVALID);
  op(0x3d, INVALID);
  op(0x3e, INVALID);
  op(0x3f, INVALID);
  }
}
#undef DECODER_SPECIAL
#endif

#ifdef DECODER_REGIMM
{
  switch(OP >> 16 & 0x1f) {
  br(0x00, BLTZ, RS, IMMi16);
  br(0x01, BGEZ, RS, IMMi16);
  op(0x02, INVALID);
  op(0x03, INVALID);
  op(0x04, INVALID);
  op(0x05, INVALID);
  op(0x06, INVALID);
  op(0x07, INVALID);
  op(0x08, INVALID);
  op(0x09, INVALID);
  op(0x0a, INVALID);
  op(0x0b, INVALID);
  op(0x0c, INVALID);
  op(0x0d, INVALID);
  op(0x0e, INVALID);
  op(0x0f, INVALID);
  br(0x10, BLTZAL, RS, IMMi16);
  br(0x11, BGEZAL, RS, IMMi16);
  op(0x12, INVALID);
  op(0x13, INVALID);
  op(0x14, INVALID);
  op(0x15, INVALID);
  op(0x16, INVALID);
  op(0x17, INVALID);
  op(0x18, INVALID);
  op(0x19, INVALID);
  op(0x1a, INVALID);
  op(0x1b, INVALID);
  op(0x1c, INVALID);
  op(0x1d, INVALID);
  op(0x1e, INVALID);
  op(0x1f, INVALID);
  }
}
#undef DECODER_REGIMM
#endif

#ifdef DECODER_SCC
{
  switch(OP >> 21 & 0x1f) {
  op(0x00, MFC0, RT, RDn);
  op(0x01, INVALID);
  op(0x02, INVALID);
  op(0x03, INVALID);
  op(0x04, MTC0, RT, RDn);
  op(0x05, INVALID);
  op(0x06, INVALID);
  op(0x07, INVALID);
  op(0x08, INVALID);
  op(0x09, INVALID);
  op(0x0a, INVALID);
  op(0x0b, INVALID);
  op(0x0c, INVALID);
  op(0x0d, INVALID);
  op(0x0e, INVALID);
  op(0x0f, INVALID);
  }

  switch(OP & 0x3f) {
  op(0x10, RFE);
  }
}
#undef DECODER_SCC
#endif

#ifdef DECODER_GTE
{
  switch(OP >> 21 & 0x1f) {
  op(0x00, MFC2, RT, RDn);
  op(0x01, INVALID);
  op(0x02, CFC2, RT, RDn);
  op(0x03, INVALID);
  op(0x04, MTC2, RT, RDn);
  op(0x05, INVALID);
  op(0x06, CTC2, RT, RDn);
  op(0x07, INVALID);
  op(0x08, INVALID);
  op(0x09, INVALID);
  op(0x0a, INVALID);
  op(0x0b, INVALID);
  op(0x0c, INVALID);
  op(0x0d, INVALID);
  op(0x0e, INVALID);
  op(0x0f, INVALID);
  }

  #define LM OP >> 10 & 1
  #define TV OP >> 13 & 3
  #define MV OP >> 15 & 3
  #define MM OP >> 17 & 3
  #define SF OP >> 19 & 1 ? 12 : 0
  switch(OP & 0x3f) {
  op(0x00, RTPS, LM, SF);
  op(0x01, RTPS, LM, SF);  //0x00 mirror?
  op(0x06, NCLIP);
  op(0x0c, OP, LM, SF);
  op(0x10, DPCS, LM, SF);
  op(0x11, INTPL, LM, SF);
  op(0x12, MVMVA, LM, TV, MV, MM, SF);
  op(0x13, NCDS, LM, SF);
  op(0x14, CDP, LM, SF);
  op(0x16, NCDT, LM, SF);
  op(0x1a, DCPL, LM, SF);  //0x29 mirror?
  op(0x1b, NCCS, LM, SF);
  op(0x1c, CC, LM, SF);
  op(0x1e, NCS, LM, SF);
  op(0x20, NCT, LM, SF);
  op(0x28, SQR, LM, SF);
  op(0x29, DCPL, LM, SF);
  op(0x2a, DPCT, LM, SF);
  op(0x2d, AVSZ3);
  op(0x2e, AVSZ4);
  op(0x30, RTPT, LM, SF);
  op(0x3d, GPF, LM, SF);
  op(0x3e, GPL, LM, SF);
  op(0x3f, NCCT, LM, SF);
  }
  #undef LM
  #undef TV
  #undef MV
  #undef MM
  #undef SF
}
#undef DECODER_GTE
#endif

#undef SA
#undef RDn
#undef RTn
#undef RSn
#undef IMMi16
#undef IMMu16
#undef IMMu26
