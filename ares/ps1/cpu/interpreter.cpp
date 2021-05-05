#define OPCODE pipeline.instruction
#define RD ipu.r[RDn]
#define RT ipu.r[RTn]
#define RS ipu.r[RSn]

#define jp(   id, name, ...) case id: return decoder##name(__VA_ARGS__)
#define op(   id, name, ...) case id: return name(__VA_ARGS__)
#define br(id, name, ...) case id: return name(__VA_ARGS__)

#define SA     (OPCODE >>  6 & 31)
#define RDn    (OPCODE >> 11 & 31)
#define RTn    (OPCODE >> 16 & 31)
#define RSn    (OPCODE >> 21 & 31)
#define IMMi16 s16(OPCODE)
#define IMMu16 u16(OPCODE)
#define IMMu26 (OPCODE & 0x03ff'ffff)

auto CPU::decoderEXECUTE() -> void {
  switch(OPCODE >> 26) {
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
  op(0x30, LWC0, RTn, RS, IMMi16);
  op(0x31, LWC1, RTn, RS, IMMi16);
  op(0x32, LWC2, RTn, RS, IMMi16);
  op(0x33, LWC3, RTn, RS, IMMi16);
  op(0x34, INVALID);
  op(0x35, INVALID);
  op(0x36, INVALID);
  op(0x37, INVALID);
  op(0x38, SWC0, RTn, RS, IMMi16);
  op(0x39, SWC1, RTn, RS, IMMi16);
  op(0x3a, SWC2, RTn, RS, IMMi16);
  op(0x3b, SWC3, RTn, RS, IMMi16);
  op(0x3c, INVALID);
  op(0x3d, INVALID);
  op(0x3e, INVALID);
  op(0x3f, INVALID);
  }
}

auto CPU::decoderSPECIAL() -> void {
  switch(OPCODE & 0x3f) {
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

auto CPU::decoderREGIMM() -> void {
  switch(OPCODE >> 16 & 0x1f) {
  br(0x00, BLTZ, RS, IMMi16);
  br(0x01, BGEZ, RS, IMMi16);
  br(0x02, BLTZ, RS, IMMi16);
  br(0x03, BGEZ, RS, IMMi16);
  br(0x04, BLTZ, RS, IMMi16);
  br(0x05, BGEZ, RS, IMMi16);
  br(0x06, BLTZ, RS, IMMi16);
  br(0x07, BGEZ, RS, IMMi16);
  br(0x08, BLTZ, RS, IMMi16);
  br(0x09, BGEZ, RS, IMMi16);
  br(0x0a, BLTZ, RS, IMMi16);
  br(0x0b, BGEZ, RS, IMMi16);
  br(0x0c, BLTZ, RS, IMMi16);
  br(0x0d, BGEZ, RS, IMMi16);
  br(0x0e, BLTZ, RS, IMMi16);
  br(0x0f, BGEZ, RS, IMMi16);
  br(0x10, BLTZAL, RS, IMMi16);
  br(0x11, BGEZAL, RS, IMMi16);
  br(0x12, BLTZ, RS, IMMi16);
  br(0x13, BGEZ, RS, IMMi16);
  br(0x14, BLTZ, RS, IMMi16);
  br(0x15, BGEZ, RS, IMMi16);
  br(0x16, BLTZ, RS, IMMi16);
  br(0x17, BGEZ, RS, IMMi16);
  br(0x18, BLTZ, RS, IMMi16);
  br(0x19, BGEZ, RS, IMMi16);
  br(0x1a, BLTZ, RS, IMMi16);
  br(0x1b, BGEZ, RS, IMMi16);
  br(0x1c, BLTZ, RS, IMMi16);
  br(0x1d, BGEZ, RS, IMMi16);
  br(0x1e, BLTZ, RS, IMMi16);
  br(0x1f, BGEZ, RS, IMMi16);
  }
}

auto CPU::decoderSCC() -> void {
  switch(OPCODE >> 21 & 0x1f) {
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

  switch(OPCODE & 0x3f) {
  op(0x10, RFE);
  }
}

auto CPU::decoderGTE() -> void {
  switch(OPCODE >> 21 & 0x1f) {
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

  #define LM OPCODE >> 10 & 1
  #define TV OPCODE >> 13 & 3
  #define MV OPCODE >> 15 & 3
  #define MM OPCODE >> 17 & 3
  #define SF OPCODE >> 19 & 1 ? 12 : 0
  switch(OPCODE & 0x3f) {
  op(0x00, RTPS, LM, SF);  //0x01 mirror?
  op(0x01, RTPS, LM, SF);
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

auto CPU::COP1() -> void {
  if(!scc.status.enable.coprocessor1) return exception.coprocessor();
}

auto CPU::COP3() -> void {
  if(!scc.status.enable.coprocessor3) return exception.coprocessor();
}

auto CPU::LWC0(u8 rt, cu32& rs, s16 imm) -> void {
  if(!scc.status.enable.coprocessor0) return exception.coprocessor();
  read<Word>(rs + imm);  //write target unknown
}

auto CPU::LWC1(u8 rt, cu32& rs, s16 imm) -> void {
  if(!scc.status.enable.coprocessor1) return exception.coprocessor();
  read<Word>(rs + imm);  //write target unknown
}

auto CPU::LWC3(u8 rt, cu32& rs, s16 imm) -> void {
  if(!scc.status.enable.coprocessor3) return exception.coprocessor();
  read<Word>(rs + imm);  //write target unknown
}

auto CPU::SWC0(u8 rt, cu32& rs, s16 imm) -> void {
  if(!scc.status.enable.coprocessor0) return exception.coprocessor();
  write<Word>(rs + imm, 0);  //read source unknown
}

auto CPU::SWC1(u8 rt, cu32& rs, s16 imm) -> void {
  if(!scc.status.enable.coprocessor1) return exception.coprocessor();
  write<Word>(rs + imm, 0);  //read source unknown
}

auto CPU::SWC3(u8 rt, cu32& rs, s16 imm) -> void {
  if(!scc.status.enable.coprocessor3) return exception.coprocessor();
  write<Word>(rs + imm, 0);  //read source unknown
}

auto CPU::INVALID() -> void {
  return exception.reservedInstruction();
}

#undef SA
#undef RDn
#undef RTn
#undef RSn
#undef IMMi16
#undef IMMu16
#undef IMMu26

#undef jp
#undef op
#undef br

#undef OPCODE
#undef RD
#undef RT
#undef RS
