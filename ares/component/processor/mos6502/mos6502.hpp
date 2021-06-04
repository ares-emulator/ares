//MOS Technologies MOS6502

#pragma once

namespace ares {

struct MOS6502 {
  n1 BCD = 1;  //set to 0 to disable BCD mode in ADC, SBC instructions

  virtual auto read(n16 addr) -> n8 = 0;
  virtual auto write(n16 addr, n8 data) -> void = 0;
  virtual auto lastCycle() -> void = 0;
  virtual auto nmi(n16& vector) -> void = 0;
  virtual auto readDebugger(n16 addr) -> n8 { return 0; }

  //mos6502.cpp
  auto power() -> void;

  //memory.cpp
  auto idle() -> void;
  auto idlePageCrossed(n16, n16) -> void;
  auto idlePageAlways(n16, n16) -> void;
  auto opcode() -> n8;
  auto operand() -> n8;
  auto load(n8 addr) -> n8;
  auto store(n8 addr, n8 data) -> void;
  auto push(n8 data) -> void;
  auto pull() -> n8;

  //algorithms.cpp
  using fp = auto (MOS6502::*)(n8) -> n8;
  auto algorithmADC(n8) -> n8;
  auto algorithmAND(n8) -> n8;
  auto algorithmASL(n8) -> n8;
  auto algorithmBIT(n8) -> n8;
  auto algorithmCMP(n8) -> n8;
  auto algorithmCPX(n8) -> n8;
  auto algorithmCPY(n8) -> n8;
  auto algorithmDEC(n8) -> n8;
  auto algorithmEOR(n8) -> n8;
  auto algorithmINC(n8) -> n8;
  auto algorithmLD (n8) -> n8;
  auto algorithmLSR(n8) -> n8;
  auto algorithmORA(n8) -> n8;
  auto algorithmROL(n8) -> n8;
  auto algorithmROR(n8) -> n8;
  auto algorithmSBC(n8) -> n8;

  //instruction.cpp
  auto interrupt() -> void;
  auto instruction() -> void;

  //instructions.cpp
  auto instructionAbsoluteModify(fp alu) -> void;
  auto instructionAbsoluteModify(fp alu, n8 index) -> void;
  auto instructionAbsoluteRead(fp alu, n8& data) -> void;
  auto instructionAbsoluteRead(fp alu, n8& data, n8 index) -> void;
  auto instructionAbsoluteWrite(n8& data) -> void;
  auto instructionAbsoluteWrite(n8& data, n8 index) -> void;
  auto instructionBranch(bool take) -> void;
  auto instructionBreak() -> void;
  auto instructionCallAbsolute() -> void;
  auto instructionClear(bool& flag) -> void;
  auto instructionImmediate(fp alu, n8& data) -> void;
  auto instructionImplied(fp alu, n8& data) -> void;
  auto instructionIndirectXRead(fp alu, n8& data) -> void;
  auto instructionIndirectXWrite(n8& data) -> void;
  auto instructionIndirectYRead(fp alu, n8& data) -> void;
  auto instructionIndirectYWrite(n8& data) -> void;
  auto instructionJumpAbsolute() -> void;
  auto instructionJumpIndirect() -> void;
  auto instructionNoOperation() -> void;
  auto instructionPull(n8& data) -> void;
  auto instructionPullP() -> void;
  auto instructionPush(n8& data) -> void;
  auto instructionPushP() -> void;
  auto instructionReturnInterrupt() -> void;
  auto instructionReturnSubroutine() -> void;
  auto instructionSet(bool& flag) -> void;
  auto instructionTransfer(n8& source, n8& target, bool flag) -> void;
  auto instructionZeroPageModify(fp alu) -> void;
  auto instructionZeroPageModify(fp alu, n8 index) -> void;
  auto instructionZeroPageRead(fp alu, n8& data) -> void;
  auto instructionZeroPageRead(fp alu, n8& data, n8 index) -> void;
  auto instructionZeroPageWrite(n8& data) -> void;
  auto instructionZeroPageWrite(n8& data, n8 index) -> void;

  //serialization.cpp
  auto serialize(serializer&) -> void;

  //disassembler.cpp
  noinline auto disassembleInstruction(maybe<n16> pc = {}) -> string;
  noinline auto disassembleContext() -> string;

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
  n8  MDR;
};

}
