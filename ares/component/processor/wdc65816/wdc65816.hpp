//WDC 65C816 CPU core
//* Ricoh 5A22
//* Nintendo SA-1

#pragma once

namespace ares {

struct WDC65816 {
  virtual auto idle() -> void = 0;
  virtual auto idleBranch() -> void {}
  virtual auto idleJump() -> void {}
  virtual auto read(n24 address) -> n8 = 0;
  virtual auto write(n24 address, n8 data) -> void = 0;
  virtual auto lastCycle() -> void = 0;
  virtual auto interruptPending() const -> bool = 0;
  virtual auto interrupt() -> void;
  virtual auto synchronizing() const -> bool = 0;

  virtual auto readDisassembler(n24 address) -> n8 { return 0; }

  auto irq() const -> bool { return r.irq; }
  auto irq(bool line) -> void { r.irq = line; }

  using r8 = n8;

  struct r16 {
    r16() {}
    r16(n32 data) : w(data) {}
    r16(const r16& data) : w(data.w) {}
    auto& operator=(u32 data) { w = data; return *this; }
    auto& operator=(const r16& data) { w = data.w; return *this; }

    n16 w;
    BitRange<16,0, 7> l{&w};
    BitRange<16,8,15> h{&w};
  };

  struct r24 {
    r24() {}
    r24(u32 data) : d(data) {}
    r24(const r24& data) : d(data.d) {}
    auto& operator=(u32 data) { d = data; return *this; }
    auto& operator=(const r24& data) { d = data.d; return *this; }

    n24 d;
    BitRange<24, 0, 7> l{&d};
    BitRange<24, 8,15> h{&d};
    BitRange<24,16,23> b{&d};
    BitRange<24, 0,15> w{&d};
  };

  //wdc65816.cpp
  auto power() -> void;

  //memory.cpp
  auto idleIRQ() -> void;
  auto idle2() -> void;
  auto idle4(n16 x, n16 y) -> void;
  auto idle6(n16 address) -> void;
  auto fetch() -> n8;
  auto pull() -> n8;
  auto push(n8 data) -> void;
  auto pullN() -> n8;
  auto pushN(n8 data) -> void;
  auto readDirect(u32 address) -> n8;
  auto readDirectX(u32 address, u32 offset) -> n8;
  auto writeDirect(u32 address, n8 data) -> void;
  auto readDirectN(u32 address) -> n8;
  auto readBank(u32 address) -> n8;
  auto writeBank(u32 address, n8 data) -> void;
  auto readStack(u32 address) -> n8;
  auto writeStack(u32 address, n8 data) -> void;

  //algorithms.cpp
  using  alu8 = auto (WDC65816::*)(n8 ) -> n8;
  using alu16 = auto (WDC65816::*)(n16) -> n16;

  auto algorithmADC8(n8) -> n8;
  auto algorithmADC16(n16) -> n16;
  auto algorithmAND8(n8) -> n8;
  auto algorithmAND16(n16) -> n16;
  auto algorithmASL8(n8) -> n8;
  auto algorithmASL16(n16) -> n16;
  auto algorithmBIT8(n8) -> n8;
  auto algorithmBIT16(n16) -> n16;
  auto algorithmCMP8(n8) -> n8;
  auto algorithmCMP16(n16) -> n16;
  auto algorithmCPX8(n8) -> n8;
  auto algorithmCPX16(n16) -> n16;
  auto algorithmCPY8(n8) -> n8;
  auto algorithmCPY16(n16) -> n16;
  auto algorithmDEC8(n8) -> n8;
  auto algorithmDEC16(n16) -> n16;
  auto algorithmEOR8(n8) -> n8;
  auto algorithmEOR16(n16) -> n16;
  auto algorithmINC8(n8) -> n8;
  auto algorithmINC16(n16) -> n16;
  auto algorithmLDA8(n8) -> n8;
  auto algorithmLDA16(n16) -> n16;
  auto algorithmLDX8(n8) -> n8;
  auto algorithmLDX16(n16) -> n16;
  auto algorithmLDY8(n8) -> n8;
  auto algorithmLDY16(n16) -> n16;
  auto algorithmLSR8(n8) -> n8;
  auto algorithmLSR16(n16) -> n16;
  auto algorithmORA8(n8) -> n8;
  auto algorithmORA16(n16) -> n16;
  auto algorithmROL8(n8) -> n8;
  auto algorithmROL16(n16) -> n16;
  auto algorithmROR8(n8) -> n8;
  auto algorithmROR16(n16) -> n16;
  auto algorithmSBC8(n8) -> n8;
  auto algorithmSBC16(n16) -> n16;
  auto algorithmTRB8(n8) -> n8;
  auto algorithmTRB16(n16) -> n16;
  auto algorithmTSB8(n8) -> n8;
  auto algorithmTSB16(n16) -> n16;

  //instructions-read.cpp
  auto instructionImmediateRead8(alu8) -> void;
  auto instructionImmediateRead16(alu16) -> void;
  auto instructionBankRead8(alu8) -> void;
  auto instructionBankRead16(alu16) -> void;
  auto instructionBankRead8(alu8, r16) -> void;
  auto instructionBankRead16(alu16, r16) -> void;
  auto instructionLongRead8(alu8, r16 = {}) -> void;
  auto instructionLongRead16(alu16, r16 = {}) -> void;
  auto instructionDirectRead8(alu8) -> void;
  auto instructionDirectRead16(alu16) -> void;
  auto instructionDirectRead8(alu8, r16) -> void;
  auto instructionDirectRead16(alu16, r16) -> void;
  auto instructionIndirectRead8(alu8) -> void;
  auto instructionIndirectRead16(alu16) -> void;
  auto instructionIndexedIndirectRead8(alu8) -> void;
  auto instructionIndexedIndirectRead16(alu16) -> void;
  auto instructionIndirectIndexedRead8(alu8) -> void;
  auto instructionIndirectIndexedRead16(alu16) -> void;
  auto instructionIndirectLongRead8(alu8, r16 = {}) -> void;
  auto instructionIndirectLongRead16(alu16, r16 = {}) -> void;
  auto instructionStackRead8(alu8) -> void;
  auto instructionStackRead16(alu16) -> void;
  auto instructionIndirectStackRead8(alu8) -> void;
  auto instructionIndirectStackRead16(alu16) -> void;

  //instructions-write.cpp
  auto instructionBankWrite8(r16) -> void;
  auto instructionBankWrite16(r16) -> void;
  auto instructionBankWrite8(r16, r16) -> void;
  auto instructionBankWrite16(r16, r16) -> void;
  auto instructionLongWrite8(r16 = {}) -> void;
  auto instructionLongWrite16(r16 = {}) -> void;
  auto instructionDirectWrite8(r16) -> void;
  auto instructionDirectWrite16(r16) -> void;
  auto instructionDirectWrite8(r16, r16) -> void;
  auto instructionDirectWrite16(r16, r16) -> void;
  auto instructionIndirectWrite8() -> void;
  auto instructionIndirectWrite16() -> void;
  auto instructionIndexedIndirectWrite8() -> void;
  auto instructionIndexedIndirectWrite16() -> void;
  auto instructionIndirectIndexedWrite8() -> void;
  auto instructionIndirectIndexedWrite16() -> void;
  auto instructionIndirectLongWrite8(r16 = {}) -> void;
  auto instructionIndirectLongWrite16(r16 = {}) -> void;
  auto instructionStackWrite8() -> void;
  auto instructionStackWrite16() -> void;
  auto instructionIndirectStackWrite8() -> void;
  auto instructionIndirectStackWrite16() -> void;

  //instructions-modify.cpp
  auto instructionImpliedModify8(alu8, r16&) -> void;
  auto instructionImpliedModify16(alu16, r16&) -> void;
  auto instructionBankModify8(alu8) -> void;
  auto instructionBankModify16(alu16) -> void;
  auto instructionBankIndexedModify8(alu8) -> void;
  auto instructionBankIndexedModify16(alu16) -> void;
  auto instructionDirectModify8(alu8) -> void;
  auto instructionDirectModify16(alu16) -> void;
  auto instructionDirectIndexedModify8(alu8) -> void;
  auto instructionDirectIndexedModify16(alu16) -> void;

  //instructions-pc.cpp
  auto instructionBranch(bool = 1) -> void;
  auto instructionBranchLong() -> void;
  auto instructionJumpShort() -> void;
  auto instructionJumpLong() -> void;
  auto instructionJumpIndirect() -> void;
  auto instructionJumpIndexedIndirect() -> void;
  auto instructionJumpIndirectLong() -> void;
  auto instructionCallShort() -> void;
  auto instructionCallLong() -> void;
  auto instructionCallIndexedIndirect() -> void;
  auto instructionReturnInterrupt() -> void;
  auto instructionReturnShort() -> void;
  auto instructionReturnLong() -> void;

  //instructions-other.cpp
  auto instructionBitImmediate8() -> void;
  auto instructionBitImmediate16() -> void;
  auto instructionNoOperation() -> void;
  auto instructionPrefix() -> void;
  auto instructionExchangeBA() -> void;
  auto instructionBlockMove8(s32) -> void;
  auto instructionBlockMove16(s32) -> void;
  auto instructionInterrupt(r16) -> void;
  auto instructionStop() -> void;
  auto instructionWait() -> void;
  auto instructionExchangeCE() -> void;
  auto instructionSetFlag(bool&) -> void;
  auto instructionClearFlag(bool&) -> void;
  auto instructionResetP() -> void;
  auto instructionSetP() -> void;
  auto instructionTransfer8(r16, r16&) -> void;
  auto instructionTransfer16(r16, r16&) -> void;
  auto instructionTransferCS() -> void;
  auto instructionTransferSX8() -> void;
  auto instructionTransferSX16() -> void;
  auto instructionTransferXS() -> void;
  auto instructionPush8(r16) -> void;
  auto instructionPush16(r16) -> void;
  auto instructionPushD() -> void;
  auto instructionPull8(r16&) -> void;
  auto instructionPull16(r16&) -> void;
  auto instructionPullD() -> void;
  auto instructionPullB() -> void;
  auto instructionPullP() -> void;
  auto instructionPushEffectiveAddress() -> void;
  auto instructionPushEffectiveIndirectAddress() -> void;
  auto instructionPushEffectiveRelativeAddress() -> void;

  //instruction.cpp
  auto instruction() -> void;

  //serialization.cpp
  auto serialize(serializer&) -> void;

  //disassembler.cpp
  noinline auto disassembleInstruction(n24 address, bool e, bool m, bool x) -> string;
  noinline auto disassembleInstruction() -> string;
  noinline auto disassembleContext(maybe<bool> e = {}) -> string;

  struct f8 {
    bool c = 0;  //carry
    bool z = 0;  //zero
    bool i = 0;  //interrupt disable
    bool d = 0;  //decimal mode
    bool x = 0;  //index register mode
    bool m = 0;  //accumulator mode
    bool v = 0;  //overflow
    bool n = 0;  //negative

    operator u32() const {
      return c << 0 | z << 1 | i << 2 | d << 3 | x << 4 | m << 5 | v << 6 | n << 7;
    }

    auto& operator=(n8 data) {
      c = data.bit(0);
      z = data.bit(1);
      i = data.bit(2);
      d = data.bit(3);
      x = data.bit(4);
      m = data.bit(5);
      v = data.bit(6);
      n = data.bit(7);
      return *this;
    }
  };

  struct Registers {
    r24 pc;
    r16 a;
    r16 x;
    r16 y;
    r16 z;
    r16 s;
    r16 d;
     r8 b;
     f8 p;

    bool e   = 0;  //emulation mode
    bool irq = 0;  //IRQ pin (0 = low, 1 = trigger)
    bool wai = 0;  //raised during wai, cleared after interrupt triggered
    bool stp = 0;  //raised during stp, never cleared

    n16 vector;  //interrupt vector address
    n24 mar;     //memory address register
    n8  mdr;     //memory data register

    r24 u;  //temporary register
    r24 v;  //temporary register
    r24 w;  //temporary register
  } r;
};

}
