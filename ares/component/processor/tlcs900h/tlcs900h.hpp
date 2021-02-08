//Toshiba TLCS900/H

/* open questions:
 *
 * what happens when a prohibited instruction operand size is used? (eg adc.l (memory),#immediate)
 * what happens when %11 is used for pre-decrement and post-increment addressing?
 * what happens when using 8-bit register indexing and d0 is set (Word) or d1/d0 is set (Long)?
 * what happens during an LDX instruction when the three padding bytes aren't all 0x00?
 * what value is read back from a non-existent 8-bit register ID? (eg 0x40-0xcf)
 * many instructions are undefined, some are marked as dummy instructions ... what do each do?
 * what happens when using (AND,OR,XOR,LD)CF (byte)source,A with A.bit(3) set?
 * what happens when using MINC#,MDEC# with a non-power of two? what about with a value of 1?
 */

#pragma once

namespace ares {

struct TLCS900H {
  enum : u32 { Byte = 1, Word = 2, Long = 4 };

  virtual auto idle(u32 clocks) -> void = 0;
  virtual auto width(n24 address) -> u32 = 0;
  virtual auto read(u32 size, n24 address) -> n32 = 0;
  virtual auto write(u32 size, n24 address, n32 data) -> void = 0;

  struct FlagRegister   { using type = n8;  enum : u32 { bits = 8  }; n1 id; };
  struct StatusRegister { using type = n16; enum : u32 { bits = 16 }; };
  struct ProgramCounter { using type = n32; enum : u32 { bits = 32 }; };
  template<typename T> struct ControlRegister { using type = T; enum : u32 { bits = 8 * sizeof(T) }; n8  id; };
  template<typename T> struct Register        { using type = T; enum : u32 { bits = 8 * sizeof(T) }; n8  id; };
  template<typename T> struct Memory          { using type = T; enum : u32 { bits = 8 * sizeof(T) }; n32 address;  };
  template<typename T> struct Immediate       { using type = T; enum : u32 { bits = 8 * sizeof(T) }; n32 constant; };

  template<typename T> auto load(Immediate<T> immediate) const -> T { return immediate.constant; }

  //tlcs900h.cpp
  auto interrupt(n8 vector) -> void;
  auto power() -> void;

  //registers.cpp
  template<typename T> auto map(Register<T>) const -> maybe<T&>;
  template<typename T> auto load(Register<T>) const -> T;
  template<typename T> auto store(Register<T>, n32) -> void;
  auto expand(Register<n8 >) const -> Register<n16>;
  auto expand(Register<n16>) const -> Register<n32>;
  auto shrink(Register<n32>) const -> Register<n16>;
  auto shrink(Register<n16>) const -> Register<n8 >;
  auto load(FlagRegister) const -> n8;
  auto store(FlagRegister, n8) -> void;
  auto load(StatusRegister) const -> n16;
  auto store(StatusRegister, n16) -> void;
  auto load(ProgramCounter) const -> n32;
  auto store(ProgramCounter, n32) -> void;

  //control-registers.cpp
  template<typename T> auto map(ControlRegister<T>) const -> maybe<T&>;
  template<typename T> auto load(ControlRegister<T>) const -> T;
  template<typename T> auto store(ControlRegister<T>, n32) -> void;

  //memory.cpp
  template<typename T = n8> auto fetch() -> T;
  template<typename T> auto fetchRegister() -> Register<T>;
  template<typename T, typename U> auto fetchMemory() -> Memory<T>;
  template<typename T> auto fetchImmediate() -> Immediate<T>;
  template<typename T> auto push(T) -> void;
  template<typename T> auto pop(T) -> void;
  template<typename T> auto load(Memory<T>) -> T;
  template<typename T> auto store(Memory<T>, n32) -> void;

  //conditions.cpp
  auto condition(n4 code) -> bool;

  //algorithms.cpp
  template<typename T> auto parity(T) const -> bool;
  template<typename T> auto algorithmAdd(T target, T source, n1 carry = 0) -> T;
  template<typename T> auto algorithmAnd(T target, T source) -> T;
  template<typename T> auto algorithmDecrement(T target, T source) -> T;
  template<typename T> auto algorithmIncrement(T target, T source) -> T;
  template<typename T> auto algorithmOr(T target, T source) -> T;
  template<typename T> auto algorithmRotated(T result) -> T;
  template<typename T> auto algorithmShifted(T result) -> T;
  template<typename T> auto algorithmSubtract(T target, T source, n1 carry = 0) -> T;
  template<typename T> auto algorithmXor(T target, T source) -> T;

  //dma.cpp
  auto dma(n2 channel) -> bool;

  //instruction.cpp
  template<u32 Bits> auto idleBW(u32 b, u32 w) -> void;
  template<u32 Bits> auto idleWL(u32 w, u32 l) -> void;
  template<u32 Bits> auto idleBWL(u32 b, u32 w, u32 l) -> void;

  template<typename T> auto toRegister3(n3) const -> Register<T>;
  template<typename T> auto toRegister8(n8) const -> Register<T>;
  template<typename T> auto toControlRegister(n8) const -> ControlRegister<T>;
  template<typename T> auto toMemory(n32 address) const -> Memory<T>;
  template<typename T> auto toImmediate(n32 constant) const -> Immediate<T>;
  template<typename T> auto toImmediate3(natural constant) const -> Immediate<T>;
  auto undefined() -> void;
  auto instruction() -> void;
  template<typename Register> auto instructionRegister(Register) -> void;
  template<typename Memory> auto instructionSourceMemory(Memory) -> void;
  auto instructionTargetMemory(n32 address) -> void;

  //instructions.cpp
  template<typename Target, typename Source> auto instructionAdd(Target, Source) -> void;
  template<typename Target, typename Source> auto instructionAddCarry(Target, Source) -> void;
  template<typename Target, typename Source> auto instructionAnd(Target, Source) -> void;
  template<typename Source, typename Offset> auto instructionAndCarry(Source, Offset) -> void;
  template<typename Source, typename Offset> auto instructionBit(Source, Offset) -> void;
  auto instructionBitSearch1Backward(Register<n16>) -> void;
  auto instructionBitSearch1Forward(Register<n16>) -> void;
  template<typename Source> auto instructionCall(Source) -> void;
  template<typename Source> auto instructionCallRelative(Source) -> void;
  template<typename Target, typename Offset> auto instructionChange(Target, Offset) -> void;
  template<typename Size, s32 Adjust, typename Target> auto instructionCompare(Target) -> void;
  template<typename Size, s32 Adjust, typename Target> auto instructionCompareRepeat(Target) -> void;
  template<typename Target, typename Source> auto instructionCompare(Target, Source) -> void;
  template<typename Target> auto instructionComplement(Target) -> void;
  auto instructionDecimalAdjustAccumulator(Register<n8>) -> void;
  template<typename Target, typename Source> auto instructionDecrement(Target, Source) -> void;
  template<typename Target, typename Offset> auto instructionDecrementJumpNotZero(Target, Offset) -> void;
  template<typename Target, typename Source> auto instructionDivide(Target, Source) -> void;
  template<typename Target, typename Source> auto instructionDivideSigned(Target, Source) -> void;
  template<typename Target, typename Source> auto instructionExchange(Target, Source) -> void;
  template<typename Target> auto instructionExtendSign(Target) -> void;
  template<typename Target> auto instructionExtendZero(Target) -> void;
  auto instructionHalt() -> void;
  template<typename Target, typename Source> auto instructionIncrement(Target, Source) -> void;
  template<typename Source> auto instructionJump(Source) -> void;
  template<typename Source> auto instructionJumpRelative(Source) -> void;
  template<typename Target, typename Offset> auto instructionLink(Target, Offset) -> void;
  template<typename Target, typename Source> auto instructionLoad(Target, Source) -> void;
  template<typename Source, typename Offset> auto instructionLoadCarry(Source, Offset) -> void;
  template<typename Size, s32 Adjust> auto instructionLoad() -> void;
  template<typename Size, s32 Adjust> auto instructionLoadRepeat() -> void;
  template<u32 Modulo, typename Target, typename Source> auto instructionModuloDecrement(Target, Source) -> void;
  template<u32 Modulo, typename Target, typename Source> auto instructionModuloIncrement(Target, Source) -> void;
  auto instructionMirror(Register<n16>) -> void;
  template<typename Target, typename Source> auto instructionMultiply(Target, Source) -> void;
  auto instructionMultiplyAdd(Register<n16>) -> void;
  template<typename Target, typename Source> auto instructionMultiplySigned(Target, Source) -> void;
  template<typename Target> auto instructionNegate(Target) -> void;
  auto instructionNoOperation() -> void;
  template<typename Target, typename Source> auto instructionOr(Target, Source) -> void;
  template<typename Source, typename Offset> auto instructionOrCarry(Source, Offset) -> void;
  template<typename Target> auto instructionPointerAdjustAccumulator(Target) -> void;
  template<typename Target> auto instructionPop(Target) -> void;
  template<typename Source> auto instructionPush(Source) -> void;
  template<typename Target, typename Offset> auto instructionReset(Target, Offset) -> void;
  auto instructionReturn() -> void;
  template<typename Source> auto instructionReturnDeallocate(Source) -> void;
  auto instructionReturnInterrupt() -> void;
  template<typename LHS, typename RHS> auto instructionRotateLeftDigit(LHS, RHS) -> void;
  template<typename Target, typename Amount> auto instructionRotateLeft(Target, Amount) -> void;
  template<typename Target, typename Amount> auto instructionRotateLeftWithoutCarry(Target, Amount) -> void;
  template<typename LHS, typename RHS> auto instructionRotateRightDigit(LHS, RHS) -> void;
  template<typename Target, typename Amount> auto instructionRotateRight(Target, Amount) -> void;
  template<typename Target, typename Amount> auto instructionRotateRightWithoutCarry(Target, Amount) -> void;
  template<typename Target, typename Offset> auto instructionSet(Target, Offset) -> void;
  auto instructionSetCarryFlag(n1 value) -> void;
  auto instructionSetCarryFlagComplement(n1 value) -> void;
  template<typename Target> auto instructionSetConditionCode(n4 code, Target) -> void;
  auto instructionSetInterruptFlipFlop(n3 value) -> void;
  auto instructionSetRegisterFilePointer(n2 value) -> void;
  template<typename Target, typename Amount> auto instructionShiftLeftArithmetic(Target, Amount) -> void;
  template<typename Target, typename Amount> auto instructionShiftLeftLogical(Target, Amount) -> void;
  template<typename Target, typename Amount> auto instructionShiftRightArithmetic(Target, Amount) -> void;
  template<typename Target, typename Amount> auto instructionShiftRightLogical(Target, Amount) -> void;
  template<typename Target, typename Offset> auto instructionStoreCarry(Target, Offset) -> void;
  auto instructionSoftwareInterrupt(n3 interrupt) -> void;
  template<typename Target, typename Source> auto instructionSubtract(Target, Source) -> void;
  template<typename Target, typename Source> auto instructionSubtractBorrow(Target, Source) -> void;
  template<typename Target, typename Offset> auto instructionTestSet(Target, Offset) -> void;
  template<typename Target> auto instructionUnlink(Target) -> void;
  template<typename Target, typename Source> auto instructionXor(Target, Source) -> void;
  template<typename Source, typename Offset> auto instructionXorCarry(Source, Offset) -> void;

  //serialization.cpp
  auto serialize(serializer&) -> void;

  union DataRegister {
    DataRegister() { l.l0 = 0; }
    struct { n32 order_lsb1(l0); } l;
    struct { n16 order_lsb2(w0, w1); } w;
    struct { n8  order_lsb4(b0, b1, b2, b3); } b;
  };

  struct Registers {
    DataRegister xwa[4];
    DataRegister xbc[4];
    DataRegister xde[4];
    DataRegister xhl[4];
    DataRegister xix;
    DataRegister xiy;
    DataRegister xiz;
    DataRegister xsp;
    DataRegister  pc;

    DataRegister dmas[4];
    DataRegister dmad[4];
    DataRegister dmam[4];
    DataRegister intnest;  //16-bit

    n1 c, cp;     //carry
    n1 n, np;     //negative
    n1 v, vp;     //overflow or parity
    n1 h, hp;     //half carry
    n1 z, zp;     //zero
    n1 s, sp;     //sign
    n2 rfp;       //register file pointer
    n3 iff = 7;   //interrupt mask flip-flop

    n1 halted;   //set if halt instruction executed; waits for an interrupt to resume
    n8 prefix;   //first opcode byte; needed for [CP|LD][ID](R) instructions
  } r;

  struct Prefetch {
    n3  valid;  //0-4 bytes
    n32 data;
  } p;

  //prefetch.cpp
  auto invalidate() -> void;
  auto prefetch() -> void;

  n24 mar;  //A0-A23: memory address register
  n16 mdr;  //D0-D15: memory data register

  static inline const Register<n8 > A{0xe0};
  static inline const Register<n8 > W{0xe1};
  static inline const Register<n8 > C{0xe4};
  static inline const Register<n8 > B{0xe5};
  static inline const Register<n8 > E{0xe8};
  static inline const Register<n8 > D{0xe9};
  static inline const Register<n8 > L{0xec};
  static inline const Register<n8 > H{0xed};

  static inline const Register<n16> WA{0xe0};
  static inline const Register<n16> BC{0xe4};
  static inline const Register<n16> DE{0xe8};
  static inline const Register<n16> HL{0xec};
  static inline const Register<n16> IX{0xf0};
  static inline const Register<n16> IY{0xf4};
  static inline const Register<n16> IZ{0xf8};
  static inline const Register<n16> SP{0xfc};

  static inline const Register<n32> XWA{0xe0};
  static inline const Register<n32> XBC{0xe4};
  static inline const Register<n32> XDE{0xe8};
  static inline const Register<n32> XHL{0xec};
  static inline const Register<n32> XIX{0xf0};
  static inline const Register<n32> XIY{0xf4};
  static inline const Register<n32> XIZ{0xf8};
  static inline const Register<n32> XSP{0xfc};

  static inline const FlagRegister F {0};
  static inline const FlagRegister FP{1};

  static inline const StatusRegister SR{};
  static inline const ProgramCounter PC{};

  static inline const ControlRegister<n32> DMAS0{0x00};
  static inline const ControlRegister<n32> DMAS1{0x04};
  static inline const ControlRegister<n32> DMAS2{0x08};
  static inline const ControlRegister<n32> DMAS3{0x0c};
  static inline const ControlRegister<n32> DMAD0{0x10};
  static inline const ControlRegister<n32> DMAD1{0x14};
  static inline const ControlRegister<n32> DMAD2{0x18};
  static inline const ControlRegister<n32> DMAD3{0x1c};
  static inline const ControlRegister<n32> DMAM0{0x20};
  static inline const ControlRegister<n32> DMAM1{0x24};
  static inline const ControlRegister<n32> DMAM2{0x28};
  static inline const ControlRegister<n32> DMAM3{0x2c};

  static inline const ControlRegister<n16> DMAC0{0x20};
  static inline const ControlRegister<n16> DMAC1{0x24};
  static inline const ControlRegister<n16> DMAC2{0x28};
  static inline const ControlRegister<n16> DMAC3{0x2c};
  static inline const ControlRegister<n16> INTNEST{0x3c};

  static inline const n4 False{0x00};
  static inline const n4 True {0x08};

  static inline const n1 Undefined = 0;

  //disassembler.cpp
  virtual auto disassembleRead(n24 address) -> n8 { return read(Byte, address); }
  noinline auto disassembleInstruction() -> string;
  noinline auto disassembleContext() -> string;
};

}
