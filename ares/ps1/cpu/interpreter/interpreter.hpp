//struct CPU {
  //interpreter/ipu.cpp
  auto ADD(u32& rd, cu32& rs, cu32& rt) -> void;
  auto ADDI(u32& rt, cu32& rs, s16 imm) -> void;
  auto ADDIU(u32& rt, cu32& rs, s16 imm) -> void;
  auto ADDU(u32& rd, cu32& rs, cu32& rt) -> void;
  auto AND(u32& rd, cu32& rs, cu32& rt) -> void;
  auto ANDI(u32& rt, cu32& rs, u16 imm) -> void;
  auto BEQ(cu32& rs, cu32& rt, s16 imm) -> void;
  auto BGEZ(cs32& rs, s16 imm) -> void;
  auto BGEZAL(cs32& rs, s16 imm) -> void;
  auto BGTZ(cs32& rs, s16 imm) -> void;
  auto BLEZ(cs32& rs, s16 imm) -> void;
  auto BLTZ(cs32& rs, s16 imm) -> void;
  auto BLTZAL(cs32& rs, s16 imm) -> void;
  auto BNE(cu32& rs, cu32& rt, s16 imm) -> void;
  auto BREAK() -> void;
  auto DIV(cs32& rs, cs32& rt) -> void;
  auto DIVU(cu32& rs, cu32& rt) -> void;
  auto J(u32 imm) -> void;
  auto JAL(u32 imm) -> void;
  auto JALR(u32& rd, cu32& rs) -> void;
  auto JR(cu32& rs) -> void;
  auto LB(u32& rt, cu32& rs, s16 imm) -> void;
  auto LBU(u32& rt, cu32& rs, s16 imm) -> void;
  auto LH(u32& rt, cu32& rs, s16 imm) -> void;
  auto LHU(u32& rt, cu32& rs, s16 imm) -> void;
  auto LUI(u32& rt, u16 imm) -> void;
  auto LW(u32& rt, cu32& rs, s16 imm) -> void;
  auto LWL(u32& rt, cu32& rs, s16 imm) -> void;
  auto LWR(u32& rt, cu32& rs, s16 imm) -> void;
  auto MFHI(u32& rd) -> void;
  auto MFLO(u32& rd) -> void;
  auto MTHI(cu32& rs) -> void;
  auto MTLO(cu32& rs) -> void;
  auto MULT(cs32& rs, cs32& rt) -> void;
  auto MULTU(cu32& rs, cu32& rt) -> void;
  auto NOR(u32& rd, cu32& rs, cu32& rt) -> void;
  auto OR(u32& rd, cu32& rs, cu32& rt) -> void;
  auto ORI(u32& rt, cu32& rs, u16 imm) -> void;
  auto SB(cu32& rt, cu32& rs, s16 imm) -> void;
  auto SH(cu32& rt, cu32& rs, s16 imm) -> void;
  auto SLL(u32& rd, cu32& rt, u8 sa) -> void;
  auto SLLV(u32& rd, cu32& rt, cu32& rs) -> void;
  auto SLT(u32& rd, cs32& rs, cs32& rt) -> void;
  auto SLTI(u32& rt, cs32& rs, s16 imm) -> void;
  auto SLTIU(u32& rt, cu32& rs, s16 imm) -> void;
  auto SLTU(u32& rd, cu32& rs, cu32& rt) -> void;
  auto SRA(u32& rd, cs32& rt, u8 sa) -> void;
  auto SRAV(u32& rd, cs32& rt, cu32& rs) -> void;
  auto SRL(u32& rd, cu32& rt, u8 sa) -> void;
  auto SRLV(u32& rd, cu32& rt, cu32& rs) -> void;
  auto SUB(u32& rd, cu32& rs, cu32& rt) -> void;
  auto SUBU(u32& rd, cu32& rs, cu32& rt) -> void;
  auto SW(cu32& rt, cu32& rs, s16 imm) -> void;
  auto SWL(cu32& rt, cu32& rs, s16 imm) -> void;
  auto SWR(cu32& rt, cu32& rs, s16 imm) -> void;
  auto SYSCALL() -> void;
  auto XOR(u32& rd, cu32& rs, cu32& rt) -> void;
  auto XORI(u32& rt, cu32& rs, u16 imm) -> void;

  //interpreter/scc.cpp
  auto MFC0(u32& rt, u8 rd) -> void;
  auto MTC0(cu32& rt, u8 rd) -> void;
  auto RFE() -> void;

  //interpreter/gte.cpp
  auto CFC2(u32& rt, u8 rd) -> void;
  auto CTC2(cu32& rt, u8 rd) -> void;
  auto LWC2(u8 rt, cu32& rs, s16 imm) -> void;
  auto MFC2(u32& rt, u8 rd) -> void;
  auto MTC2(cu32& rt, u8 rd) -> void;
  auto SWC2(u8 rt, cu32& rs, s16 imm) -> void;

  //interpreter/interpreter.cpp
  auto decoderEXECUTE() -> void;
  auto decoderSPECIAL() -> void;
  auto decoderREGIMM() -> void;
  auto decoderSCC() -> void;
  auto decoderGTE() -> void;

  auto COP1() -> void;
  auto COP3() -> void;
  auto LWC0(u8 rt, cu32& rs, s16 imm) -> void;
  auto LWC1(u8 rt, cu32& rs, s16 imm) -> void;
  auto LWC3(u8 rt, cu32& rs, s16 imm) -> void;
  auto SWC0(u8 rt, cu32& rs, s16 imm) -> void;
  auto SWC1(u8 rt, cu32& rs, s16 imm) -> void;
  auto SWC3(u8 rt, cu32& rs, s16 imm) -> void;
  auto INVALID() -> void;
//};
