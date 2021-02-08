#pragma once

namespace ares {

struct SPC700 {
  virtual auto idle() -> void = 0;
  virtual auto read(n16 address) -> n8 = 0;
  virtual auto write(n16 address, n8 data) -> void = 0;
  virtual auto synchronizing() const -> bool = 0;

  virtual auto readDisassembler(n16 address) -> n8 { return 0; }

  //spc700.cpp
  auto power() -> void;

  //memory.cpp
  auto fetch() -> n8;
  auto load(n8 address) -> n8;
  auto store(n8 address, n8 data) -> void;
  auto pull() -> n8;
  auto push(n8 data) -> void;

  //instruction.cpp
  auto instruction() -> void;

  //algorithms.cpp
  auto algorithmADC(n8, n8) -> n8;
  auto algorithmAND(n8, n8) -> n8;
  auto algorithmASL(n8) -> n8;
  auto algorithmCMP(n8, n8) -> n8;
  auto algorithmDEC(n8) -> n8;
  auto algorithmEOR(n8, n8) -> n8;
  auto algorithmINC(n8) -> n8;
  auto algorithmLD (n8, n8) -> n8;
  auto algorithmLSR(n8) -> n8;
  auto algorithmOR (n8, n8) -> n8;
  auto algorithmROL(n8) -> n8;
  auto algorithmROR(n8) -> n8;
  auto algorithmSBC(n8, n8) -> n8;
  auto algorithmADW(n16, n16) -> n16;
  auto algorithmCPW(n16, n16) -> n16;
  auto algorithmLDW(n16, n16) -> n16;
  auto algorithmSBW(n16, n16) -> n16;

  //instructions.cpp
  using fps = auto (SPC700::*)(n8) -> n8;
  using fpb = auto (SPC700::*)(n8, n8) -> n8;
  using fpw = auto (SPC700::*)(n16, n16) -> n16;

  auto instructionAbsoluteBitModify(n3) -> void;
  auto instructionAbsoluteBitSet(n3, bool) -> void;
  auto instructionAbsoluteRead(fpb, n8&) -> void;
  auto instructionAbsoluteModify(fps) -> void;
  auto instructionAbsoluteWrite(n8&) -> void;
  auto instructionAbsoluteIndexedRead(fpb, n8&) -> void;
  auto instructionAbsoluteIndexedWrite(n8&) -> void;
  auto instructionBranch(bool) -> void;
  auto instructionBranchBit(n3, bool) -> void;
  auto instructionBranchNotDirect() -> void;
  auto instructionBranchNotDirectDecrement() -> void;
  auto instructionBranchNotDirectIndexed(n8&) -> void;
  auto instructionBranchNotYDecrement() -> void;
  auto instructionBreak() -> void;
  auto instructionCallAbsolute() -> void;
  auto instructionCallPage() -> void;
  auto instructionCallTable(n4) -> void;
  auto instructionComplementCarry() -> void;
  auto instructionDecimalAdjustAdd() -> void;
  auto instructionDecimalAdjustSub() -> void;
  auto instructionDirectRead(fpb, n8&) -> void;
  auto instructionDirectModify(fps) -> void;
  auto instructionDirectWrite(n8&) -> void;
  auto instructionDirectDirectCompare(fpb) -> void;
  auto instructionDirectDirectModify(fpb) -> void;
  auto instructionDirectDirectWrite() -> void;
  auto instructionDirectImmediateCompare(fpb) -> void;
  auto instructionDirectImmediateModify(fpb) -> void;
  auto instructionDirectImmediateWrite() -> void;
  auto instructionDirectCompareWord(fpw) -> void;
  auto instructionDirectReadWord(fpw) -> void;
  auto instructionDirectModifyWord(s32) -> void;
  auto instructionDirectWriteWord() -> void;
  auto instructionDirectIndexedRead(fpb, n8&, n8&) -> void;
  auto instructionDirectIndexedModify(fps, n8&) -> void;
  auto instructionDirectIndexedWrite(n8&, n8&) -> void;
  auto instructionDivide() -> void;
  auto instructionExchangeNibble() -> void;
  auto instructionFlagSet(bool&, bool) -> void;
  auto instructionImmediateRead(fpb, n8&) -> void;
  auto instructionImpliedModify(fps, n8&) -> void;
  auto instructionIndexedIndirectRead(fpb, n8&) -> void;
  auto instructionIndexedIndirectWrite(n8&, n8&) -> void;
  auto instructionIndirectIndexedRead(fpb, n8&) -> void;
  auto instructionIndirectIndexedWrite(n8&, n8&) -> void;
  auto instructionIndirectXRead(fpb) -> void;
  auto instructionIndirectXWrite(n8&) -> void;
  auto instructionIndirectXIncrementRead(n8&) -> void;
  auto instructionIndirectXIncrementWrite(n8&) -> void;
  auto instructionIndirectXCompareIndirectY(fpb) -> void;
  auto instructionIndirectXWriteIndirectY(fpb) -> void;
  auto instructionJumpAbsolute() -> void;
  auto instructionJumpIndirectX() -> void;
  auto instructionMultiply() -> void;
  auto instructionNoOperation() -> void;
  auto instructionOverflowClear() -> void;
  auto instructionPull(n8&) -> void;
  auto instructionPullP() -> void;
  auto instructionPush(n8) -> void;
  auto instructionReturnInterrupt() -> void;
  auto instructionReturnSubroutine() -> void;
  auto instructionStop() -> void;
  auto instructionTestSetBitsAbsolute(bool) -> void;
  auto instructionTransfer(n8&, n8&) -> void;
  auto instructionWait() -> void;

  //serialization.cpp
  auto serialize(serializer&) -> void;

  //disassembler.cpp
  noinline auto disassembleInstruction(n16 address, n1 p) -> string;
  noinline auto disassembleInstruction() -> string;
  noinline auto disassembleContext() -> string;

  struct Flags {
    bool c;  //carry
    bool z;  //zero
    bool i;  //interrupt disable
    bool h;  //half-carry
    bool b;  //break
    bool p;  //page
    bool v;  //overflow
    bool n;  //negative

    operator u32() const {
      return c << 0 | z << 1 | i << 2 | h << 3 | b << 4 | p << 5 | v << 6 | n << 7;
    }

    auto& operator=(n8 data) {
      c = data.bit(0);
      z = data.bit(1);
      i = data.bit(2);
      h = data.bit(3);
      b = data.bit(4);
      p = data.bit(5);
      v = data.bit(6);
      n = data.bit(7);
      return *this;
    }
  };

  struct Registers {
    union Pair {
      Pair() : w(0) {}
      n16 w;
      struct Byte { n8 order_lsb2(l, h); } byte;
    } pc, ya;
    n8 x, s;
    Flags p;

    bool wait = false;
    bool stop = false;
  } r;
};

}
