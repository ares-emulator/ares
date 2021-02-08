//NEC V30MZ (reduced functionality NEC V30 for embedded use)

//V30 missing instructions:
//  0f 10,11,18,19  test1
//  0f 12,13,1a,1b  clr1
//  0f 14,15,1c,1d  set1
//  0f 16,17,1e,1f  not1
//  0f 20           add4s
//  0f 22           sub4s
//  0f 26           cmp4s
//  0f 28           rol4
//  0f 2a           ror4
//  0f 31,39        ins
//  0f 33,3b        ext
//  0f ff           brkem (8080 emulation mode) [retem, calln]
//  64              repnc
//  65              repc
//  66,67           fpo2
//  d8-df           fpo1

//x86 variant instructions:
//  8f c0-c7  pop reg [CPU bug: pops from stack; fails to set register]
//  d4 xx     aam [ignores the immediate; always uses (base) 10]
//  d5 xx     aad [ignores the immediate; always uses (base) 10]
//  d6        xlat (mirror of d7) [this is salc on x86 CPUs]
//  f1        ??? [this is int 0x1 on x86 CPUs; said to be a two-byte NOP on V20; unknown on V30/V30MZ]
//  ff f8-ff  push (mirror of ff f0-f7)

//x86 unemulated variation:
//  after interrupts, NEC V20/V30 CPUs resume string instructions with prefixes intact. unlike x86 CPUs
//  I need more information on this behavior in order to emulate it ...
//  also, the opcode f1 behavior is not currently known

//V30 opcode prefix functionality:
//  there is a seven-level stack for opcode prefixes. once full, older prefixes are pushed off the stack

//other notes:
//  0f     pop cs (not nop) [on the V20; the V30 uses this for instruction extensions; unsure on the V30MZ]
//  8e xx  mov cs,modRM (works as expected; able to set CS)

//I currently emulate opcode 0f as pop cs, although it's unknown if that is correct.

#pragma once

namespace ares {

struct V30MZ {
  using Size = u32;
  enum : u32 { Byte = 1, Word = 2, Long = 4 };
  enum : u32 {
    SegmentOverrideES  = 0x26,
    SegmentOverrideCS  = 0x2e,
    SegmentOverrideSS  = 0x36,
    SegmentOverrideDS  = 0x3e,
    Lock               = 0xf0,
    RepeatWhileZeroLo  = 0xf2,
    RepeatWhileZeroHi  = 0xf3,
  };

  virtual auto wait(u32 clocks = 1) -> void = 0;
  virtual auto read(n20 address) -> n8 = 0;
  virtual auto write(n20 address, n8 data) -> void = 0;
  virtual auto in(n16 port) -> n8 = 0;
  virtual auto out(n16 port, n8 data) -> void = 0;

  auto warning(string text) -> void;
  auto power() -> void;
  auto exec() -> void;

  //instruction.cpp
  auto interrupt(n8 vector) -> void;
  auto instruction() -> void;

  //registers.cpp
  auto repeat() -> n8;
  auto segment(n16) -> n16;

  auto getAcc(Size) -> n32;
  auto setAcc(Size, n32) -> void;

  //modrm.cpp
  auto modRM() -> void;

  auto getMem(Size, u32 offset = 0) -> n16;
  auto setMem(Size, n16) -> void;

  auto getReg(Size) -> n16;
  auto setReg(Size, n16) -> void;

  auto getSeg() -> n16;
  auto setSeg(n16) -> void;

  //memory.cpp
  auto read(Size, n16, n16) -> n32;
  auto write(Size, n16, n16, n16) -> void;

  auto in(Size, n16) -> n16;
  auto out(Size, n16, n16) -> void;

  auto fetch(Size = Byte) -> n16;
  auto pop() -> n16;
  auto push(n16) -> void;

  //algorithms.cpp
  auto parity(n8) const -> bool;
  auto ADC (Size, n16, n16) -> n16;
  auto ADD (Size, n16, n16) -> n16;
  auto AND (Size, n16, n16) -> n16;
  auto DEC (Size, n16     ) -> n16;
  auto DIV (Size, n32, n32) -> n32;
  auto DIVI(Size, i32, i32) -> n32;
  auto INC (Size, n16     ) -> n16;
  auto MUL (Size, n16, n16) -> n32;
  auto MULI(Size, i16, i16) -> n32;
  auto NEG (Size, n16     ) -> n16;
  auto NOT (Size, n16     ) -> n16;
  auto OR  (Size, n16, n16) -> n16;
  auto RCL (Size, n16, n5 ) -> n16;
  auto RCR (Size, n16, n5 ) -> n16;
  auto ROL (Size, n16, n4 ) -> n16;
  auto ROR (Size, n16, n4 ) -> n16;
  auto SAL (Size, n16, n5 ) -> n16;
  auto SAR (Size, n16, n5 ) -> n16;
  auto SBB (Size, n16, n16) -> n16;
  auto SUB (Size, n16, n16) -> n16;
  auto SHL (Size, n16, n5 ) -> n16;
  auto SHR (Size, n16, n5 ) -> n16;
  auto XOR (Size, n16, n16) -> n16;

  //instructions-adjust.cpp
  auto instructionDecimalAdjust(bool) -> void;
  auto instructionAsciiAdjust(bool) -> void;
  auto instructionAdjustAfterMultiply() -> void;
  auto instructionAdjustAfterDivide() -> void;

  //instructions-alu.cpp
  auto instructionAddMemReg(Size) -> void;
  auto instructionAddRegMem(Size) -> void;
  auto instructionAddAccImm(Size) -> void;
  auto instructionOrMemReg(Size) -> void;
  auto instructionOrRegMem(Size) -> void;
  auto instructionOrAccImm(Size) -> void;
  auto instructionAdcMemReg(Size) -> void;
  auto instructionAdcRegMem(Size) -> void;
  auto instructionAdcAccImm(Size) -> void;
  auto instructionSbbMemReg(Size) -> void;
  auto instructionSbbRegMem(Size) -> void;
  auto instructionSbbAccImm(Size) -> void;
  auto instructionAndMemReg(Size) -> void;
  auto instructionAndRegMem(Size) -> void;
  auto instructionAndAccImm(Size) -> void;
  auto instructionSubMemReg(Size) -> void;
  auto instructionSubRegMem(Size) -> void;
  auto instructionSubAccImm(Size) -> void;
  auto instructionXorMemReg(Size) -> void;
  auto instructionXorRegMem(Size) -> void;
  auto instructionXorAccImm(Size) -> void;
  auto instructionCmpMemReg(Size) -> void;
  auto instructionCmpRegMem(Size) -> void;
  auto instructionCmpAccImm(Size) -> void;
  auto instructionTestMemReg(Size) -> void;
  auto instructionTestAcc(Size) -> void;
  auto instructionMultiplySignedRegMemImm(Size) -> void;
  auto instructionIncReg(u16&) -> void;
  auto instructionDecReg(u16&) -> void;
  auto instructionSignExtendByte() -> void;
  auto instructionSignExtendWord() -> void;

  //instructions-exec.cpp
  auto instructionLoop() -> void;
  auto instructionLoopWhile(bool) -> void;
  auto instructionJumpShort() -> void;
  auto instructionJumpIf(bool) -> void;
  auto instructionJumpNear() -> void;
  auto instructionJumpFar() -> void;
  auto instructionCallNear() -> void;
  auto instructionCallFar() -> void;
  auto instructionReturn() -> void;
  auto instructionReturnImm() -> void;
  auto instructionReturnFar() -> void;
  auto instructionReturnFarImm() -> void;
  auto instructionReturnInt() -> void;
  auto instructionInt3() -> void;
  auto instructionIntImm() -> void;
  auto instructionInto() -> void;
  auto instructionEnter() -> void;
  auto instructionLeave() -> void;
  auto instructionPushReg(u16&) -> void;
  auto instructionPopReg(u16&) -> void;
  auto instructionPushFlags() -> void;
  auto instructionPopFlags() -> void;
  auto instructionPushAll() -> void;
  auto instructionPopAll() -> void;
  auto instructionPushImm(Size) -> void;
  auto instructionPopMem() -> void;

  //instructions-flag.cpp
  auto instructionStoreFlagsAcc() -> void;
  auto instructionLoadAccFlags() -> void;
  auto instructionComplementCarry() -> void;
  auto instructionClearFlag(u32) -> void;
  auto instructionSetFlag(u32) -> void;

  //instructions-group.cpp
  auto instructionGroup1MemImm(Size, bool) -> void;
  auto instructionGroup2MemImm(Size, maybe<n8> = {}) -> void;
  auto instructionGroup3MemImm(Size) -> void;
  auto instructionGroup4MemImm(Size) -> void;

  //instructions-misc.cpp
  auto instructionSegment(n16) -> void;
  auto instructionRepeat() -> void;
  auto instructionLock() -> void;
  auto instructionWait() -> void;
  auto instructionHalt() -> void;
  auto instructionNop() -> void;
  auto instructionIn(Size) -> void;
  auto instructionOut(Size) -> void;
  auto instructionInDX(Size) -> void;
  auto instructionOutDX(Size) -> void;
  auto instructionTranslate() -> void;
  auto instructionBound() -> void;

  //instructions-move.cpp
  auto instructionMoveMemReg(Size) -> void;
  auto instructionMoveRegMem(Size) -> void;
  auto instructionMoveMemSeg() -> void;
  auto instructionMoveSegMem() -> void;
  auto instructionMoveAccMem(Size) -> void;
  auto instructionMoveMemAcc(Size) -> void;
  auto instructionMoveRegImm(u8&) -> void;
  auto instructionMoveRegImm(u16&) -> void;
  auto instructionMoveMemImm(Size) -> void;
  auto instructionExchange(u16&, u16&) -> void;
  auto instructionExchangeMemReg(Size) -> void;
  auto instructionLoadEffectiveAddressRegMem() -> void;
  auto instructionLoadSegmentMem(u16&) -> void;

  //instructions-string.cpp
  auto instructionInString(Size) -> void;
  auto instructionOutString(Size) -> void;
  auto instructionMoveString(Size) -> void;
  auto instructionCompareString(Size) -> void;
  auto instructionStoreString(Size) -> void;
  auto instructionLoadString(Size) -> void;
  auto instructionScanString(Size) -> void;

  //serialization.cpp
  auto serialize(serializer&) -> void;

  //disassembler.cpp
  auto disassembleInstruction(n16 cs, n16 ip) -> string;
  auto disassembleInstruction() -> string;
  auto disassembleContext() -> string;

  struct State {
    bool halt;    //set to true for hlt instruction; blocks execution until next interrupt
    bool poll;    //set to false to suppress interrupt polling between CPU instructions
    bool prefix;  //set to true for prefix instructions; prevents flushing of Prefix struct
  } state;

  n8 opcode;
  vector<n8> prefixes;

  struct ModRM {
    n2 mod;
    n3 reg;
    n3 mem;

    n16 segment;
    n16 address;
  } modrm;

  struct Registers {
    union { u16 ax; struct { u8 order_lsb2(al, ah); }; };
    union { u16 cx; struct { u8 order_lsb2(cl, ch); }; };
    union { u16 dx; struct { u8 order_lsb2(dl, dh); }; };
    union { u16 bx; struct { u8 order_lsb2(bl, bh); }; };
    u16 sp;
    u16 bp;
    u16 si;
    u16 di;
    u16 es;
    u16 cs;
    u16 ss;
    u16 ds;
    u16 ip;

    u8*  b[8]{&al, &cl, &dl, &bl, &ah, &ch, &dh, &bh};
    u16* w[8]{&ax, &cx, &dx, &bx, &sp, &bp, &si, &di};
    u16* s[8]{&es, &cs, &ss, &ds, &es, &cs, &ss, &ds};

    struct Flags {
      n16 data;
      BitField<16, 0> c{&data};  //carry
      BitField<16, 2> p{&data};  //parity
      BitField<16, 4> h{&data};  //half-carry
      BitField<16, 6> z{&data};  //zero
      BitField<16, 7> s{&data};  //sign
      BitField<16, 8> b{&data};  //break
      BitField<16, 9> i{&data};  //interrupt
      BitField<16,10> d{&data};  //direction
      BitField<16,11> v{&data};  //overflow
      BitField<16,15> m{&data};  //mode

      operator u32() const { return data & 0x8fd5 | 0x7002; }
      auto& operator =(u32 value) { return data  = value, *this; }
      auto& operator&=(u32 value) { return data &= value, *this; }
      auto& operator^=(u32 value) { return data ^= value, *this; }
      auto& operator|=(u32 value) { return data |= value, *this; }
    } f;
  } r;
};

}
