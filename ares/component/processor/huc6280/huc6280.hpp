//Hudson Soft HuC6280

#pragma once

namespace ares {

struct HuC6280 {
  virtual auto step(u32 clocks) -> void = 0;
  virtual auto read(n8 bank, n13 address) -> n8 = 0;
  virtual auto write(n8 bank, n13 address, n8 data) -> void = 0;
  virtual auto store(n2 address, n8 data) -> void = 0;
  virtual auto lastCycle() -> void = 0;

  //huc6280.cpp
  auto power() -> void;

  //memory.cpp
  auto load8(n8) -> n8;
  auto load16(n16) -> n8;
  auto store8(n8, n8) -> void;
  auto store16(n16, n8) -> void;

  auto idle() -> void;
  auto opcode() -> n8;
  auto operand() -> n8;

  auto push(n8) -> void;
  auto pull() -> n8;

  //instructions.cpp
  using fp = auto (HuC6280::*)(n8) -> n8;
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
  auto algorithmTRB(n8) -> n8;
  auto algorithmTSB(n8) -> n8;

  using bp = auto (HuC6280::*)(n16&, n16&, bool) -> void;
  auto algorithmTAI(n16&, n16&, bool) -> void;
  auto algorithmTDD(n16&, n16&, bool) -> void;
  auto algorithmTIA(n16&, n16&, bool) -> void;
  auto algorithmTII(n16&, n16&, bool) -> void;
  auto algorithmTIN(n16&, n16&, bool) -> void;

  //instruction.cpp
  auto interrupt(n16 vector) -> void;
  auto instruction() -> void;

  //instructions.cpp
  auto instructionAbsoluteModify(fp, n8 = 0) -> void;
  auto instructionAbsoluteRead(fp, n8&, n8 = 0) -> void;
  auto instructionAbsoluteReadMemory(fp, n8 = 0) -> void;
  auto instructionAbsoluteWrite(n8, n8 = 0) -> void;
  auto instructionBlockMove(bp) -> void;
  auto instructionBranch(bool) -> void;
  auto instructionBranchIfBitReset(n3) -> void;
  auto instructionBranchIfBitSet(n3) -> void;
  auto instructionBranchSubroutine() -> void;
  auto instructionBreak() -> void;
  auto instructionCallAbsolute() -> void;
  auto instructionChangeSpeedLow() -> void;
  auto instructionChangeSpeedHigh() -> void;
  auto instructionClear(n8&) -> void;
  auto instructionClear(bool&) -> void;
  auto instructionImmediate(fp, n8&) -> void;
  auto instructionImmediateMemory(fp) -> void;
  auto instructionImplied(fp, n8&) -> void;
  auto instructionIndirectRead(fp, n8&, n8 = 0) -> void;
  auto instructionIndirectReadMemory(fp, n8 = 0) -> void;
  auto instructionIndirectWrite(n8, n8 = 0) -> void;
  auto instructionIndirectYRead(fp, n8&) -> void;
  auto instructionIndirectYReadMemory(fp) -> void;
  auto instructionIndirectYWrite(n8) -> void;
  auto instructionJumpAbsolute() -> void;
  auto instructionJumpIndirect(n8 = 0) -> void;
  auto instructionNoOperation() -> void;
  auto instructionPull(n8&) -> void;
  auto instructionPullP() -> void;
  auto instructionPush(n8) -> void;
  auto instructionResetMemoryBit(n3) -> void;
  auto instructionReturnInterrupt() -> void;
  auto instructionReturnSubroutine() -> void;
  auto instructionSet(bool&) -> void;
  auto instructionSetMemoryBit(n3) -> void;
  auto instructionStoreImplied(n2) -> void;
  auto instructionSwap(n8&, n8&) -> void;
  auto instructionTestAbsolute(n8 = 0) -> void;
  auto instructionTestZeroPage(n8 = 0) -> void;
  auto instructionTransfer(n8&, n8&) -> void;
  auto instructionTransferAccumulatorToMPR() -> void;
  auto instructionTransferMPRToAccumulator() -> void;
  auto instructionTransferXS() -> void;
  auto instructionZeroPageModify(fp, n8 = 0) -> void;
  auto instructionZeroPageRead(fp, n8&, n8 = 0) -> void;
  auto instructionZeroPageReadMemory(fp, n8 = 0) -> void;
  auto instructionZeroPageWrite(n8, n8 = 0) -> void;

  //disassembler.cpp
  auto disassembleInstruction() -> string;
  auto disassembleContext() -> string;

  //serialization.cpp
  auto serialize(serializer&) -> void;

  struct Flags {
    bool c;  //carry
    bool z;  //zero
    bool i;  //interrupt disable
    bool d;  //decimal mode
    bool b;  //break
    bool t;  //memory operation
    bool v;  //overflow
    bool n;  //negative

    operator n8() const {
      return c << 0 | z << 1 | i << 2 | d << 3 | b << 4 | t << 5 | v << 6 | n << 7;
    }

    auto operator()() const -> n8 {
      return operator n8();
    }

    auto& operator=(n8 data) {
      c = data.bit(0);
      z = data.bit(1);
      i = data.bit(2);
      d = data.bit(3);
      b = data.bit(4);
      t = data.bit(5);
      v = data.bit(6);
      n = data.bit(7);
      return *this;
    }
  };

  struct Registers {
    n8  a;
    n8  x;
    n8  y;
    n8  s;
    n16 pc;
    n8  mpr[8];
    n8  mpl;  //MPR latch
    n8  cs;   //code speed (3 = fast, 12 = slow)
    Flags p;
  } r;

  bool blockMove;
  Random random;
};

}
