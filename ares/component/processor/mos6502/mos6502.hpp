//MOS Technologies MOS6502

#pragma once

namespace ares {

struct MOS6502 {
  n1 BCD = 1;  //set to 0 to disable BCD mode in ADC, SBC instructions

  virtual auto read(n16 addr) -> n8 = 0;
  virtual auto write(n16 addr, n8 data) -> void = 0;

  virtual auto lastCycle() -> void {}
  virtual auto cancelNmi() -> void {}
  virtual auto delayIrq() -> void {}
  virtual auto irqPending() -> bool { return false; }
  virtual auto nmi(n16& vector) -> void {}
  virtual auto readDebugger(n16 addr) -> n8 { return 0; }

  //mos6502.cpp
  auto power(bool reset = false) -> void;

  //memory.cpp
  auto idle() -> void;
  auto idleZeroPage(n8) -> void;
  auto idlePageCrossed(n16, n16) -> void;
  auto idlePageAlways(n16, n16) -> void;
  auto opcode() -> n8;
  auto operand() -> n8;
  auto load(n8 addr) -> n8;
  auto store(n8 addr, n8 data) -> void;
  auto push(n8 data) -> void;
  auto pull() -> n8;

  //addresses.cpp
  using addr = auto (MOS6502::*)() -> n16;
  auto addressAbsolute() -> n16;
  auto addressAbsoluteJMP() -> n16;
  auto addressAbsoluteXRead() -> n16;
  auto addressAbsoluteXWrite() -> n16;
  auto addressAbsoluteYRead() -> n16;
  auto addressAbsoluteYWrite() -> n16;
  auto addressImmediate() -> n16;
  auto addressImplied() -> n16;
  auto addressIndirect() -> n16;
  auto addressIndirectX() -> n16;
  auto addressIndirectYRead() -> n16;
  auto addressIndirectYWrite() -> n16;
  auto addressRelative() -> n16;
  auto addressZeroPage() -> n16;
  auto addressZeroPageX() -> n16;
  auto addressZeroPageY() -> n16;

  //algorithms.cpp
  using algorithm = auto (MOS6502::*)() -> void;

  // official algorithms
  auto algorithmADC() -> void;
  auto algorithmAND() -> void;
  auto algorithmASLA() -> void;
  auto algorithmASLM() -> void;
  auto algorithmBCC() -> void;
  auto algorithmBCS() -> void;
  auto algorithmBEQ() -> void;
  auto algorithmBIT() -> void;
  auto algorithmBMI() -> void;
  auto algorithmBNE() -> void;
  auto algorithmBPL() -> void;
  auto algorithmBRK() -> void;
  auto algorithmBVC() -> void;
  auto algorithmBVS() -> void;
  auto algorithmBranch(bool take) -> void;
  auto algorithmCLC() -> void;
  auto algorithmCLD() -> void;
  auto algorithmCLI() -> void;
  auto algorithmCLV() -> void;
  auto algorithmCMP() -> void;
  auto algorithmCPX() -> void;
  auto algorithmCPY() -> void;
  auto algorithmDEC() -> void;
  auto algorithmDEX() -> void;
  auto algorithmDEY() -> void;
  auto algorithmEOR() -> void;
  auto algorithmINC() -> void;
  auto algorithmINX() -> void;
  auto algorithmINY() -> void;
  auto algorithmJMP() -> void;
  auto algorithmJSR() -> void;
  auto algorithmNOP() -> void;
  auto algorithmLDA() -> void;
  auto algorithmLDX() -> void;
  auto algorithmLDY() -> void;
  auto algorithmLSRA() -> void;
  auto algorithmLSRM() -> void;
  auto algorithmORA() -> void;
  auto algorithmPHA() -> void;
  auto algorithmPHP() -> void;
  auto algorithmPLA() -> void;
  auto algorithmPLP() -> void;
  auto algorithmROLA() -> void;
  auto algorithmROLM() -> void;
  auto algorithmRORA() -> void;
  auto algorithmRORM() -> void;
  auto algorithmRTI() -> void;
  auto algorithmRTS() -> void;
  auto algorithmSBC() -> void;
  auto algorithmSEC() -> void;
  auto algorithmSED() -> void;
  auto algorithmSEI() -> void;
  auto algorithmSTA() -> void;
  auto algorithmSTX() -> void;
  auto algorithmSTY() -> void;
  auto algorithmTAX() -> void;
  auto algorithmTAY() -> void;
  auto algorithmTSX() -> void;
  auto algorithmTXA() -> void;
  auto algorithmTXS() -> void;
  auto algorithmTYA() -> void;

  // unofficial algorithms
  auto algorithmAAC() -> void;
  auto algorithmAAX() -> void;
  auto algorithmANE() -> void;
  auto algorithmARR() -> void;
  auto algorithmASR() -> void;
  auto algorithmATX() -> void;
  auto algorithmAXS() -> void;
  auto algorithmDCP() -> void;
  auto algorithmISC() -> void;
  auto algorithmLAS() -> void;
  auto algorithmLAX() -> void;
  auto algorithmRLA() -> void;
  auto algorithmRRA() -> void;
  auto algorithmSLO() -> void;
  auto algorithmSRE() -> void;
  auto algorithmSXA() -> void;
  auto algorithmSYA() -> void;
  auto algorithmTAS() -> void;

  // freeze algorithms: need reset
  auto algorithmJAM() -> void;

  //instruction.cpp
  auto interrupt() -> void;
  auto reset() -> void;
  auto instruction() -> void;

  //instructions.cpp
  auto instructionNone(addr mode, algorithm alg) -> void;
  auto instructionLoad(addr mode, algorithm alg) -> void;
  auto instructionStore(addr mode, algorithm alg) -> void;
  auto instructionModify(addr mode, algorithm alg) -> void;

  //serialization.cpp
  auto serialize(serializer&) -> void;

  //disassembler.cpp
  NALL_NOINLINE auto disassembleInstruction(maybe<n16> pc = {}) -> string;
  NALL_NOINLINE auto disassembleContext() -> string;

  struct PR {
    bool c;  //carry
    bool z;  //zero
    bool i;  //interrupt disable
    bool d;  //decimal mode
    bool v;  //overflow
    bool n;  //negative

    operator u8() const {
      return c << 0 | z << 1 | i << 2 | d << 3 | v << 6 | n << 7;
    }

    auto& operator=(u8 data) {
      c = data >> 0 & 1;
      z = data >> 1 & 1;
      i = data >> 2 & 1;
      d = data >> 3 & 1;
      v = data >> 6 & 1;
      n = data >> 7 & 1;
      return *this;
    }
  };

  n8  A;
  n8  X;
  n8  Y;
  n8  S;
  PR  P;
  n16 PC;
  n16 MAR;
  n8  MDR;
  n1  resetting = 0;
};

}
