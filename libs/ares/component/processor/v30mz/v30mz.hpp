//NEC V30MZ (optimized and more 80186-compatible NEC V30)

//does not contain V20/V30 extended instructions

//x86 unemulated variation:
//  after interrupts, NEC V20/V30 CPUs resume string instructions with prefixes intact. unlike x86 CPUs
//  I need more information on this behavior in order to emulate it ...

//V30MZ opcode prefix functionality:
//  there is a seven-level stack for opcode prefixes. once full, older prefixes are pushed off the stack

#pragma once

namespace ares {

struct V30MZ {
  enum : u32 { Byte = 1, Word = 2, Long = 4 };
  enum : u32 {
    SegmentOverrideDS1 = 0x26,  //ES:
    SegmentOverridePS  = 0x2e,  //CS:
    SegmentOverrideSS  = 0x36,  //SS:
    SegmentOverrideDS0 = 0x3e,  //DS:
    Lock               = 0xf0,  //LOCK:
    RepeatWhileZeroLo  = 0xf2,  //REPNZ:
    RepeatWhileZeroHi  = 0xf3,  //REPZ:
  };

  virtual auto step(u32 clocks = 1) -> void = 0;
  virtual auto width(n20 address) -> u32 = 0;
  virtual auto speed(n20 address) -> n32 = 0;
  virtual auto read(n20 address) -> n8 = 0;
  virtual auto write(n20 address, n8 data) -> void = 0;
  virtual auto in(n16 port) -> n8 = 0;
  virtual auto out(n16 port, n8 data) -> void = 0;
  virtual auto ioWidth(n16 port) -> u32 = 0;
  virtual auto ioSpeed(n16 port) -> n32 = 0;

  //v30mz.cpp
  auto power() -> void;

  //instruction.cpp
  auto prefixFlush() -> void;
  auto interrupt(u8 vector) -> bool;
  auto nonMaskableInterrupt(bool value) -> bool;
  auto instruction() -> void;

  //registers.cpp
  auto repeat() -> u8;
  auto segment(u16) -> u16;

  template<u32> auto getAccumulator() -> u32;
  template<u32> auto setAccumulator(u32) -> void;

  //modrm.cpp
  auto modRM(bool forceAddress = false) -> void;

  auto getSegment() -> u16;
  auto setSegment(u16) -> void;

  template<u32> auto getRegister() -> u16;
  template<u32> auto setRegister(u16) -> void;

  template<u32> auto getMemory(u32 offset = 0) -> u16;
  template<u32> auto setMemory(u16) -> void;

  //memory.cpp
  template<u32> auto read(u16 segment, u16 address) -> u32;
  template<u32> auto write(u16 segment, u16 address, u16 data) -> void;

  template<u32> auto in(u16 address) -> u16;
  template<u32> auto out(u16 address, u16 data) -> void;

  auto pop() -> u16;
  auto push(u16) -> void;

  //prefetch.cpp
  auto wait(u32 clocks) -> void;
  auto loop() -> void;
  auto flush() -> void;
  auto prefetch() -> void;
  template<u32> auto fetch() -> u16;

  //algorithms.cpp
  auto parity(u8) const -> bool;
  template<u32> auto ADC (u16, u16) -> u16;
  template<u32> auto ADD (u16, u16) -> u16;
  template<u32> auto AND (u16, u16) -> u16;
  template<u32> auto DEC (u16     ) -> u16;
  template<u32> auto DIVI(s32, s32) -> u32;
  template<u32> auto DIVU(u32, u32) -> u32;
  template<u32> auto INC (u16     ) -> u16;
  template<u32> auto MULI(s16, s16) -> u32;
  template<u32> auto MULU(u16, u16) -> u32;
  template<u32> auto NEG (u16     ) -> u16;
  template<u32> auto NOT (u16     ) -> u16;
  template<u32> auto OR  (u16, u16) -> u16;
  template<u32> auto RCL (u16, u5 ) -> u16;
  template<u32> auto RCR (u16, u5 ) -> u16;
  template<u32> auto ROL (u16, u5 ) -> u16;
  template<u32> auto ROR (u16, u5 ) -> u16;
  template<u32> auto SAR (u16, u5 ) -> u16;
  template<u32> auto SBB (u16, u16) -> u16;
  template<u32> auto SUB (u16, u16) -> u16;
  template<u32> auto SHL (u16, u5 ) -> u16;
  template<u32> auto SHR (u16, u5 ) -> u16;
  template<u32> auto XOR (u16, u16) -> u16;

  //instructions-adjust.cpp
  auto instructionDecimalAdjust(bool) -> void;
  auto instructionAsciiAdjust(bool) -> void;
  auto instructionAdjustAfterMultiply() -> void;
  auto instructionAdjustAfterDivide() -> void;

  //instructions-alu.cpp
  template<u32> auto instructionAddMemReg() -> void;
  template<u32> auto instructionAddRegMem() -> void;
  template<u32> auto instructionAddAccImm() -> void;
  template<u32> auto instructionOrMemReg() -> void;
  template<u32> auto instructionOrRegMem() -> void;
  template<u32> auto instructionOrAccImm() -> void;
  template<u32> auto instructionAdcMemReg() -> void;
  template<u32> auto instructionAdcRegMem() -> void;
  template<u32> auto instructionAdcAccImm() -> void;
  template<u32> auto instructionSbbMemReg() -> void;
  template<u32> auto instructionSbbRegMem() -> void;
  template<u32> auto instructionSbbAccImm() -> void;
  template<u32> auto instructionAndMemReg() -> void;
  template<u32> auto instructionAndRegMem() -> void;
  template<u32> auto instructionAndAccImm() -> void;
  template<u32> auto instructionSubMemReg() -> void;
  template<u32> auto instructionSubRegMem() -> void;
  template<u32> auto instructionSubAccImm() -> void;
  template<u32> auto instructionXorMemReg() -> void;
  template<u32> auto instructionXorRegMem() -> void;
  template<u32> auto instructionXorAccImm() -> void;
  template<u32> auto instructionCmpMemReg() -> void;
  template<u32> auto instructionCmpRegMem() -> void;
  template<u32> auto instructionCmpAccImm() -> void;
  template<u32> auto instructionTestMemReg() -> void;
  template<u32> auto instructionTestAcc() -> void;
  template<u32> auto instructionMultiplySignedRegMemImm() -> void;
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
  auto instructionPushSeg(u16&) -> void;
  auto instructionPopReg(u16&) -> void;
  auto instructionPopSeg(u16&) -> void;
  auto instructionPushFlags() -> void;
  auto instructionPopFlags() -> void;
  auto instructionPushAll() -> void;
  auto instructionPopAll() -> void;
  template<u32> auto instructionPushImm() -> void;
  auto instructionPopMem() -> void;

  //instructions-flag.cpp
  auto instructionStoreFlagsAcc() -> void;
  auto instructionLoadAccFlags() -> void;
  auto instructionComplementCarry() -> void;
  auto instructionClearFlag(u32) -> void;
  auto instructionSetFlag(u32) -> void;

  //instructions-group.cpp
  template<u32> auto instructionGroup1MemImm(bool) -> void;
  template<u32> auto instructionGroup2MemImm(u8, maybe<u8> = {}) -> void;
  template<u32> auto instructionGroup3MemImm() -> void;
  template<u32> auto instructionGroup4MemImm() -> void;

  //instructions-misc.cpp
  auto instructionSegment(n16) -> void;
  auto instructionRepeat() -> void;
  auto instructionLock() -> void;
  auto instructionWait() -> void;
  auto instructionHalt() -> void;
  auto instructionNop() -> void;
  auto instructionUndefined() -> void;
  auto instructionUndefined1() -> void;
  template<u32> auto instructionIn() -> void;
  template<u32> auto instructionOut() -> void;
  template<u32> auto instructionInDW() -> void;
  template<u32> auto instructionOutDW() -> void;
  auto instructionTranslate() -> void;
  auto instructionSetALCarry() -> void;
  auto instructionBound() -> void;

  //instructions-move.cpp
  template<u32> auto instructionMoveMemReg() -> void;
  template<u32> auto instructionMoveRegMem() -> void;
  auto instructionMoveMemSeg() -> void;
  auto instructionMoveSegMem() -> void;
  template<u32> auto instructionMoveAccMem() -> void;
  template<u32> auto instructionMoveMemAcc() -> void;
  auto instructionMoveRegImm(u8&) -> void;
  auto instructionMoveRegImm(u16&) -> void;
  template<u32> auto instructionMoveMemImm() -> void;
  auto instructionExchange(u16&, u16&) -> void;
  template<u32> auto instructionExchangeMemReg() -> void;
  auto instructionLoadEffectiveAddressRegMem() -> void;
  auto instructionLoadSegmentMem(u16&) -> void;

  //instructions-string.cpp
  template<u32> auto instructionInString() -> void;
  template<u32> auto instructionOutString() -> void;
  template<u32> auto instructionMoveString() -> void;
  template<u32> auto instructionCompareString() -> void;
  template<u32> auto instructionStoreString() -> void;
  template<u32> auto instructionLoadString() -> void;
  template<u32> auto instructionScanString() -> void;

  //serialization.cpp
  auto serialize(serializer&) -> void;

  //disassembler.cpp
  auto disassembleInstruction(u16 ps, u16 pc) -> string;
  auto disassembleInstruction() -> string;
  auto disassembleContext() -> string;

  struct State {
    bool halt;      //set to true for hlt instruction; blocks execution until next interrupt
    bool poll;      //set to false to suppress interrupt polling between CPU instructions
    bool prefix;    //set to true for prefix instructions; prevents flushing of Prefix struct
    bool interrupt; //interrupt enable processor status before CPU instruction execution
    bool brk;       //breakpoint enable processor status before CPU instruction execution
    bool nmi;       //non-maskable interrupt requested
  } state;

  u8 opcode;
  struct Prefix {
    u16 count;
    u8 lock;
    u8 repeat;
    u8 segment;
  } prefix;

  struct ModRM {
    u2 mod;
    u3 reg;
    u3 mem;

    u16 segment;
    u16 address;

    bool useAddress;
  } modrm;

  union { u16 AW; struct { u8 order_lsb2(AL, AH); }; };  //AX
  union { u16 CW; struct { u8 order_lsb2(CL, CH); }; };  //CX
  union { u16 DW; struct { u8 order_lsb2(DL, DH); }; };  //DX
  union { u16 BW; struct { u8 order_lsb2(BL, BH); }; };  //BX
  u16 SP;   //SP
  u16 BP;   //BP
  u16 IX;   //SI
  u16 IY;   //DI
  u16 DS1;  //ES
  u16 PS;   //CS
  u16 SS;   //SS
  u16 DS0;  //DS
  u16 PC;   //IP
  u16 PFP;  //prefetch pointer
  queue<u8[16]> PF;  //prefetch queue
  n8 PFW; //prefetch wait time

  struct ProgramStatusWord {
    u16 data;
    BitField<16, 0> CY {&data};  //carry
    BitField<16, 2> P  {&data};  //parity
    BitField<16, 4> AC {&data};  //auxiliary carry
    BitField<16, 6> Z  {&data};  //zero
    BitField<16, 7> S  {&data};  //sign
    BitField<16, 8> BRK{&data};  //break
    BitField<16, 9> IE {&data};  //interrupt enable
    BitField<16,10> DIR{&data};  //direction
    BitField<16,11> V  {&data};  //overflow
    BitField<16,15> MD {&data};  //mode (unused, for V30HL compatibility only)

    operator u16() const { return data & 0x8fd5 | 0x7002; }
    auto& operator =(u16 value) { return data  = value, *this; }
    auto& operator&=(u16 value) { return data &= value, *this; }
    auto& operator^=(u16 value) { return data ^= value, *this; }
    auto& operator|=(u16 value) { return data |= value, *this; }
  } PSW;

  u8*  const RB[8]{&AL,  &CL, &DL, &BL,  &AH,  &CH, &DH, &BH };
  u16* const RW[8]{&AW,  &CW, &DW, &BW,  &SP,  &BP, &IX, &IY };
  u16* const RS[8]{&DS1, &PS, &SS, &DS0, &DS1, &PS, &SS, &DS0};

protected:
  // TODO: implement interrupt priorities
  enum class InterruptSource : u32 {
    CPU = 1, // internal request
    NMI = 2, // external NMI
    INT = 3, // external interrupt request
    SingleStep = 4 // internal request - single step
  };

  // algorithms.cpp
  template<u32> auto ADD (u16, u16, u16) -> u16;
  template<u32> auto SUB (u16, u16, u16) -> u16;

  // instruction.cpp
  auto interrupt(u8 vector, InterruptSource source) -> bool;
};

}
