#pragma once

//Zilog Z80

namespace ares {

struct Z80 {
  struct Bus {
    virtual auto read(n16 address) -> n8 = 0;
    virtual auto write(n16 address, n8 data) -> void = 0;

    virtual auto in(n16 address) -> n8 = 0;
    virtual auto out(n16 address, n8 data) -> void = 0;
  };

  virtual auto step(u32 clocks) -> void = 0;
  virtual auto synchronizing() const -> bool = 0;

  //CMOS: out (c) writes 0x00
  //NMOS: out (c) writes 0xff; if an interrupt fires during "ld a,i" or "ld a,r", PF is cleared
  enum class MOSFET : u32 { CMOS, NMOS };

  //z80.cpp
  auto power(MOSFET = MOSFET::NMOS) -> void;
  auto reset() -> void;

  auto irq(n8 extbus = 0xff) -> bool;
  auto nmi() -> bool;
  auto parity(n8) const -> bool;

  //memory.cpp
  auto wait(u32 clocks = 1) -> void;
  auto opcode() -> n8;
  auto operand() -> n8;
  auto operands() -> n16;
  auto push(n16) -> void;
  auto pop() -> n16;
  auto displace(n16&, u32 wclocks = 5) -> n16;
  auto read(n16 address) -> n8;
  auto write(n16 address, n8 data) -> void;
  auto in(n16 address) -> n8;
  auto out(n16 address, n8 data) -> void;

  //instruction.cpp
  auto instruction() -> void;
  auto instruction(n8 code) -> void;
  auto instructionCB(n8 code) -> void;
  auto instructionCBd(n16 address, n8 code) -> void;
  auto instructionED(n8 code) -> void;

  //algorithms.cpp
  auto ADD(n8, n8, bool = false) -> n8;
  auto AND(n8, n8) -> n8;
  auto BIT(n3, n8) -> n8;
  auto CP (n8, n8) -> void;
  auto DEC(n8) -> n8;
  auto IN (n8) -> n8;
  auto INC(n8) -> n8;
  auto OR (n8, n8) -> n8;
  auto RES(n3, n8) -> n8;
  auto RL (n8) -> n8;
  auto RLC(n8) -> n8;
  auto RR (n8) -> n8;
  auto RRC(n8) -> n8;
  auto SET(n3, n8) -> n8;
  auto SLA(n8) -> n8;
  auto SLL(n8) -> n8;
  auto SRA(n8) -> n8;
  auto SRL(n8) -> n8;
  auto SUB(n8, n8, bool = false) -> n8;
  auto XOR(n8, n8) -> n8;

  //instructions.cpp
  auto instructionADC_a_irr(n16&) -> void;
  auto instructionADC_a_n() -> void;
  auto instructionADC_a_r(n8&) -> void;
  auto instructionADC_hl_rr(n16&) -> void;
  auto instructionADD_a_irr(n16&) -> void;
  auto instructionADD_a_n() -> void;
  auto instructionADD_a_r(n8&) -> void;
  auto instructionADD_hl_rr(n16&) -> void;
  auto instructionAND_a_irr(n16&) -> void;
  auto instructionAND_a_n() -> void;
  auto instructionAND_a_r(n8&) -> void;
  auto instructionBIT_o_irr(n3, n16&) -> void;
  auto instructionBIT_o_irr_r(n3, n16&, n8&) -> void;
  auto instructionBIT_o_r(n3, n8&) -> void;
  auto instructionCALL_c_nn(bool c) -> void;
  auto instructionCALL_nn() -> void;
  auto instructionCCF() -> void;
  auto instructionCP_a_irr(n16& x) -> void;
  auto instructionCP_a_n() -> void;
  auto instructionCP_a_r(n8& x) -> void;
  auto instructionCPD() -> void;
  auto instructionCPDR() -> void;
  auto instructionCPI() -> void;
  auto instructionCPIR() -> void;
  auto instructionCPL() -> void;
  auto instructionDAA() -> void;
  auto instructionDEC_irr(n16&) -> void;
  auto instructionDEC_r(n8&) -> void;
  auto instructionDEC_rr(n16&) -> void;
  auto instructionDI() -> void;
  auto instructionDJNZ_e() -> void;
  auto instructionEI() -> void;
  auto instructionEX_irr_rr(n16&, n16&) -> void;
  auto instructionEX_rr_rr(n16&, n16&) -> void;
  auto instructionEXX() -> void;
  auto instructionHALT() -> void;
  auto instructionIM_o(n2) -> void;
  auto instructionIN_a_in() -> void;
  auto instructionIN_r_ic(n8&) -> void;
  auto instructionIN_ic() -> void;
  auto instructionINC_irr(n16&) -> void;
  auto instructionINC_r(n8&) -> void;
  auto instructionINC_rr(n16&) -> void;
  auto instructionIND() -> void;
  auto instructionINDR() -> void;
  auto instructionINI() -> void;
  auto instructionINIR() -> void;
  auto instructionJP_c_nn(bool) -> void;
  auto instructionJP_rr(n16&) -> void;
  auto instructionJR_c_e(bool) -> void;
  auto instructionLD_a_inn() -> void;
  auto instructionLD_a_irr(n16& x) -> void;
  auto instructionLD_inn_a() -> void;
  auto instructionLD_inn_rr(n16&) -> void;
  auto instructionLD_irr_a(n16&) -> void;
  auto instructionLD_irr_n(n16&) -> void;
  auto instructionLD_irr_r(n16&, n8&) -> void;
  auto instructionLD_r_n(n8&) -> void;
  auto instructionLD_r_irr(n8&, n16&) -> void;
  auto instructionLD_r_r(n8&, n8&) -> void;
  auto instructionLD_r_r1(n8&, n8&) -> void;
  auto instructionLD_r_r2(n8&, n8&) -> void;
  auto instructionLD_rr_inn(n16&) -> void;
  auto instructionLD_rr_nn(n16&) -> void;
  auto instructionLD_sp_rr(n16&) -> void;
  auto instructionLDD() -> void;
  auto instructionLDDR() -> void;
  auto instructionLDI() -> void;
  auto instructionLDIR() -> void;
  auto instructionNEG() -> void;
  auto instructionNOP() -> void;
  auto instructionOR_a_irr(n16&) -> void;
  auto instructionOR_a_n() -> void;
  auto instructionOR_a_r(n8&) -> void;
  auto instructionOTDR() -> void;
  auto instructionOTIR() -> void;
  auto instructionOUT_ic_r(n8&) -> void;
  auto instructionOUT_ic() -> void;
  auto instructionOUT_in_a() -> void;
  auto instructionOUTD() -> void;
  auto instructionOUTI() -> void;
  auto instructionPOP_rr(n16&) -> void;
  auto instructionPUSH_rr(n16&) -> void;
  auto instructionRES_o_irr(n3, n16&) -> void;
  auto instructionRES_o_irr_r(n3, n16&, n8&) -> void;
  auto instructionRES_o_r(n3, n8&) -> void;
  auto instructionRET() -> void;
  auto instructionRET_c(bool c) -> void;
  auto instructionRETI() -> void;
  auto instructionRETN() -> void;
  auto instructionRL_irr(n16&) -> void;
  auto instructionRL_irr_r(n16&, n8&) -> void;
  auto instructionRL_r(n8&) -> void;
  auto instructionRLA() -> void;
  auto instructionRLC_irr(n16&) -> void;
  auto instructionRLC_irr_r(n16&, n8&) -> void;
  auto instructionRLC_r(n8&) -> void;
  auto instructionRLCA() -> void;
  auto instructionRLD() -> void;
  auto instructionRR_irr(n16&) -> void;
  auto instructionRR_irr_r(n16&, n8&) -> void;
  auto instructionRR_r(n8&) -> void;
  auto instructionRRA() -> void;
  auto instructionRRC_irr(n16&) -> void;
  auto instructionRRC_irr_r(n16&, n8&) -> void;
  auto instructionRRC_r(n8&) -> void;
  auto instructionRRCA() -> void;
  auto instructionRRD() -> void;
  auto instructionRST_o(n3) -> void;
  auto instructionSBC_a_irr(n16&) -> void;
  auto instructionSBC_a_n() -> void;
  auto instructionSBC_a_r(n8&) -> void;
  auto instructionSBC_hl_rr(n16&) -> void;
  auto instructionSCF() -> void;
  auto instructionSET_o_irr(n3, n16&) -> void;
  auto instructionSET_o_irr_r(n3, n16&, n8&) -> void;
  auto instructionSET_o_r(n3, n8&) -> void;
  auto instructionSLA_irr(n16&) -> void;
  auto instructionSLA_irr_r(n16&, n8&) -> void;
  auto instructionSLA_r(n8&) -> void;
  auto instructionSLL_irr(n16&) -> void;
  auto instructionSLL_irr_r(n16&, n8&) -> void;
  auto instructionSLL_r(n8&) -> void;
  auto instructionSRA_irr(n16&) -> void;
  auto instructionSRA_irr_r(n16&, n8&) -> void;
  auto instructionSRA_r(n8&) -> void;
  auto instructionSRL_irr(n16&) -> void;
  auto instructionSRL_irr_r(n16&, n8&) -> void;
  auto instructionSRL_r(n8&) -> void;
  auto instructionSUB_a_irr(n16&) -> void;
  auto instructionSUB_a_n() -> void;
  auto instructionSUB_a_r(n8&) -> void;
  auto instructionXOR_a_irr(n16&) -> void;
  auto instructionXOR_a_n() -> void;
  auto instructionXOR_a_r(n8&) -> void;

  //serialization.cpp
  auto serialize(serializer&) -> void;

  //disassembler.cpp
  noinline auto disassembleInstruction(maybe<n16> pc = {}) -> string;
  noinline auto disassembleContext() -> string;

  auto disassemble(n16 pc, n8 prefix, n8 code) -> string;
  auto disassembleCB(n16 pc, n8 prefix, n8 code) -> string;
  auto disassembleCBd(n16 pc, n8 prefix, i8 d, n8 code) -> string;
  auto disassembleED(n16 pc, n8 prefix, n8 code) -> string;

  MOSFET mosfet = MOSFET::NMOS;
  enum class Prefix : u32 { hl, ix, iy } prefix = Prefix::hl;

  union Pair {
    Pair() : word(0) {}
    n16 word;
    struct Byte { n8 order_msb2(hi, lo); } byte;
  };

  Pair af, af_;
  Pair bc, bc_;
  Pair de, de_;
  Pair hl, hl_;
  Pair ix;
  Pair iy;
  Pair ir;
  Pair wz;

  n16 SP;
  n16 PC;
  b1  EI;    //"ei" executed last
  b1  P;     //"ld a,i" or "ld a,r" executed last
  b1  Q;     //opcode that updated flag registers executed last
  b1  HALT;  //"halt" instruction executed
  b1  IFF1;  //interrupt flip-flop 1
  b1  IFF2;  //interrupt flip-flop 2
  n2  IM;    //interrupt mode (0-2)

  Bus* bus = nullptr;
};

}
