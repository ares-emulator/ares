//Motorola 68HC05

#pragma once

namespace ares {

struct M68HC05 {
  virtual auto step(u32 cycles) -> void = 0;
  virtual auto read(n16 address) -> n8 = 0;
  virtual auto write(n16 address, n8 data) -> void = 0;

  //m68hc05.cpp
  auto power() -> void;

  //memory.cpp
  auto load(n16 address) -> n8;
  auto store(n16 address, n8 data) -> void;
  template<typename T = n8> auto fetch() -> T;
  template<typename T = n8> auto push(T) -> void;
  template<typename T = n8> auto pop() -> T;

  //algorithms.cpp
  using f1 = auto (M68HC05::*)(n8) -> n8;
  using f2 = auto (M68HC05::*)(n8, n8) -> n8;
  auto algorithmADC(n8, n8) -> n8;
  auto algorithmADD(n8, n8) -> n8;
  auto algorithmAND(n8, n8) -> n8;
  auto algorithmASR(n8) -> n8;
  auto algorithmBIT(n8, n8) -> n8;
  auto algorithmCLR(n8) -> n8;
  auto algorithmCMP(n8, n8) -> n8;
  auto algorithmCOM(n8) -> n8;
  auto algorithmDEC(n8) -> n8;
  auto algorithmEOR(n8, n8) -> n8;
  auto algorithmINC(n8) -> n8;
  auto algorithmLD (n8, n8) -> n8;
  auto algorithmLSL(n8) -> n8;
  auto algorithmLSR(n8) -> n8;
  auto algorithmNEG(n8) -> n8;
  auto algorithmOR (n8, n8) -> n8;
  auto algorithmROL(n8) -> n8;
  auto algorithmROR(n8) -> n8;
  auto algorithmSBC(n8, n8) -> n8;
  auto algorithmSUB(n8, n8) -> n8;
  auto algorithmTST(n8) -> n8;

  //instruction.cpp
  auto interrupt() -> void;
  auto instruction() -> void;

  //instructions.cpp
  auto instructionBCLR(n3) -> void;
  auto instructionBRA(b1) -> void;
  auto instructionBRCLR(n3) -> void;
  auto instructionBRSET(n3) -> void;
  auto instructionBSET(n3) -> void;
  auto instructionBSR() -> void;
  auto instructionCLR(b1&) -> void;
  auto instructionILL() -> void;
  auto instructionJMPa() -> void;
  auto instructionJMPd() -> void;
  auto instructionJMPi() -> void;
  auto instructionJMPo() -> void;
  auto instructionJMPw() -> void;
  auto instructionJSRa() -> void;
  auto instructionJSRd() -> void;
  auto instructionJSRi() -> void;
  auto instructionJSRo() -> void;
  auto instructionJSRw() -> void;
  auto instructionLDa(f2, n8&) -> void;
  auto instructionLDd(f2, n8&) -> void;
  auto instructionLDi(f2, n8&) -> void;
  auto instructionLDo(f2, n8&) -> void;
  auto instructionLDr(f2, n8&) -> void;
  auto instructionLDw(f2, n8&) -> void;
  auto instructionMUL () -> void;
  auto instructionNOP() -> void;
  auto instructionRMWd(f1) -> void;
  auto instructionRMWi(f1) -> void;
  auto instructionRMWo(f1) -> void;
  auto instructionRMWr(f1, n8&) -> void;
  auto instructionRSP() -> void;
  auto instructionRTI() -> void;
  auto instructionRTS() -> void;
  auto instructionSET(b1&) -> void;
  auto instructionSTa(n8&) -> void;
  auto instructionSTd(n8&) -> void;
  auto instructionSTi(n8&) -> void;
  auto instructionSTo(n8&) -> void;
  auto instructionSTw(n8&) -> void;
  auto instructionSTOP() -> void;
  auto instructionSWI() -> void;
  auto instructionTAX() -> void;
  auto instructionTSTd(f1) -> void;
  auto instructionTSTi(f1) -> void;
  auto instructionTSTo(f1) -> void;
  auto instructionTSTr(f1, n8&) -> void;
  auto instructionTXA() -> void;
  auto instructionWAIT() -> void;

  //serialization.cpp
  auto serialize(serializer&) -> void;

  //disassembler.cpp
  auto disassembleInstruction() -> string;
  auto disassembleContext() -> string;

  n8  A;    //accumulator
  n8  X;    //index register
  n16 PC;   //program counter
  n6  SP;   //stack pointer
  struct {  //condition code register
    operator u8() const {
      return C << 0 | Z << 1 | N << 2 | I << 3 | H << 4 | 0b111 << 5;
    }

    auto& operator=(u8 data) {
      C = data >> 0 & 1;
      Z = data >> 1 & 1;
      N = data >> 2 & 1;
      I = data >> 3 & 1;
      H = data >> 4 & 1;
      return *this;
    }

    b1 C;  //carry
    b1 Z;  //zero
    b1 N;  //negative
    b1 I;  //interrupt mask
    b1 H;  //half-carry
  } CCR;
  n1 IRQ;  //interrupt request
};

}
