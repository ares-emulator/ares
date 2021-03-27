#pragma once

//Samsung SSP1601

namespace ares {

struct SSP1601 {
  virtual auto read(u16 address) -> u16 = 0;
  virtual auto readEXT(n3 r) -> u16 = 0;
  virtual auto writeEXT(n3 r, u16 v) -> void = 0;

  //ssp1601.cpp
  auto power() -> void;

  //algorithms.cpp
  auto test(u16 op) -> bool;

  auto add(u32) -> void;
  auto sub(u32) -> void;
  auto cmp(u32) -> void;
  auto and(u32) -> void;
  auto or (u32) -> void;
  auto eor(u32) -> void;
  auto shr() -> void;
  auto shl() -> void;
  auto neg() -> void;
  auto abs() -> void;

  //registers.cpp
  auto readGR(n4 r) -> n16;
  auto writeGR(n4 r, u16 v) -> void;
  auto updateP() -> n32;

  //memory.cpp
  auto fetch() -> u16;
  auto readPR1(u16 op) -> u16;
  auto readPR1(n2 r, n1 bank, n2 mode) -> u16;
  auto writePR1(u16 op, u16 v) -> void;
  auto readPR2(u16 op) -> u16;

  //instructions.cpp
  auto interrupt() -> u16;
  auto instruction() -> void;

  //serialization.cpp
  auto serialize(serializer&) -> void;

  //disassembler.cpp
  auto disassembleInstruction() -> string;
  auto disassembleInstruction(u16 pc, u16 op, u16 imm) -> string;
  auto disassembleContext() -> string;

  enum : u32 {
    SSP_R0, SSP_X, SSP_Y, SSP_A,
    SSP_ST, SSP_STACK, SSP_PC, SSP_P,
  };

  n16 RAM[512];
  n16 FRAME[6];
  n16 X;
  n16 Y;
  n32 A;
  n16 ST;
  n16 STACK;
  n16 PC;
  n32 P;
  n8  R[8];
  n4  IRQ;
};

}
