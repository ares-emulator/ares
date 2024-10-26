#pragma once

//Intel 8080

namespace ares {

struct I8080 {
  struct Bus {
    virtual auto read(n16 address) -> n8 = 0;
    virtual auto write(n16 address, n8 data) -> void = 0;

    virtual auto in(n16 address) -> n8 = 0;
    virtual auto out(n16 address, n8 data) -> void = 0;
  };

  virtual auto step(u32 clocks) -> void = 0;
  virtual auto synchronizing() const -> bool = 0;

  //i8080.cpp
  auto power() -> void;
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
  auto read(n16 address) -> n8;
  auto write(n16 address, n8 data) -> void;
  auto in(n16 address) -> n8;
  auto out(n16 address, n8 data) -> void;

  //instruction.cpp
  auto instruction() -> void;
  auto instruction(n8 code) -> void;

  //algorithms.cpp
  auto ADD(n8, n8, bool = false) -> n8;
  auto AND(n8, n8) -> n8;
  auto CP (n8, n8) -> void;
  auto DEC(n8) -> n8;
  auto INC(n8) -> n8;
  auto OR (n8, n8) -> n8;
  auto SUB(n8, n8, bool = false) -> n8;
  auto XOR(n8, n8) -> n8;

  //instructions.cpp
  auto instructionADC_a_irr(n16&) -> void;
  auto instructionADC_a_n() -> void;
  auto instructionADC_a_r(n8&) -> void;
  auto instructionADD_a_irr(n16&) -> void;
  auto instructionADD_a_n() -> void;
  auto instructionADD_a_r(n8&) -> void;
  auto instructionADD_hl_rr(n16&) -> void;
  auto instructionAND_a_irr(n16&) -> void;
  auto instructionAND_a_n() -> void;
  auto instructionAND_a_r(n8&) -> void;
  auto instructionCALL_c_nn(bool c) -> void;
  auto instructionCCF() -> void;
  auto instructionCP_a_irr(n16& x) -> void;
  auto instructionCP_a_n() -> void;
  auto instructionCP_a_r(n8& x) -> void;
  auto instructionCPL() -> void;
  auto instructionDAA() -> void;
  auto instructionDEC_irr(n16&) -> void;
  auto instructionDEC_r(n8&) -> void;
  auto instructionDEC_rr(n16&) -> void;
  auto instructionDI() -> void;
  auto instructionEI() -> void;
  auto instructionEX_irr_rr(n16&, n16&) -> void;
  auto instructionEX_rr_rr(n16&, n16&) -> void;
  auto instructionHALT() -> void;
  auto instructionIN_a_in() -> void;
  auto instructionINC_irr(n16&) -> void;
  auto instructionINC_r(n8&) -> void;
  auto instructionINC_rr(n16&) -> void;
  auto instructionJP_c_nn(bool) -> void;
  auto instructionJP_rr(n16&) -> void;
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
  auto instructionLD_rr_inn(n16&) -> void;
  auto instructionLD_rr_nn(n16&) -> void;
  auto instructionLD_sp_rr(n16&) -> void;
  auto instructionNOP() -> void;
  auto instructionOR_a_irr(n16&) -> void;
  auto instructionOR_a_n() -> void;
  auto instructionOR_a_r(n8&) -> void;
  auto instructionOUT_in_a() -> void;
  auto instructionPOP_rr(n16&) -> void;
  auto instructionPUSH_rr(n16&) -> void;
  auto instructionRET() -> void;
  auto instructionRET_c(bool c) -> void;
  auto instructionRLA() -> void;
  auto instructionRLCA() -> void;
  auto instructionRRA() -> void;
  auto instructionRRCA() -> void;
  auto instructionRST_o(n3) -> void;
  auto instructionSBC_a_irr(n16&) -> void;
  auto instructionSBC_a_n() -> void;
  auto instructionSBC_a_r(n8&) -> void;
  auto instructionSCF() -> void;
  auto instructionSUB_a_irr(n16&) -> void;
  auto instructionSUB_a_n() -> void;
  auto instructionSUB_a_r(n8&) -> void;
  auto instructionXOR_a_irr(n16&) -> void;
  auto instructionXOR_a_n() -> void;
  auto instructionXOR_a_r(n8&) -> void;

  //serialization.cpp
  auto serialize(serializer&) -> void;

  //disassembler.cpp
  NALL_NOINLINE auto disassembleInstruction(maybe<n16> pc = {}) -> string;
  NALL_NOINLINE auto disassembleContext() -> string;

  auto disassemble(n16 pc, n8 prefix, n8 code) -> string;

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
  b1  INTE;  //interrupt enable

  Bus* bus = nullptr;
};

}
