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

  virtual auto step(u32 clocks) -> void = 0;
  virtual auto width(n24 address) -> u32 = 0;
  virtual auto speed(u32 size, n24 address) -> n32 = 0;
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
  auto expand(Register<n32>) const -> Register<n32> = delete;  //unused
  auto shrink(Register<n32>) const -> Register<n16>;
  auto shrink(Register<n16>) const -> Register<n8 >;
  auto shrink(Register<n8 >) const -> Register<n8 > = delete;  //unused
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

  //prefetch.cpp
  auto invalidate() -> void;
  auto prefetch(u32 clocks) -> void;
  template<typename T = n8> auto fetch() -> T;

  //memory.cpp
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
  template<typename T> auto toRegister3(n3) const -> Register<T>;
  template<typename T> auto toRegister8(n8) const -> Register<T>;
  template<typename T> auto toControlRegister(n8) const -> ControlRegister<T>;
  template<typename T> auto toMemory(n32 address) const -> Memory<T>;
  template<typename T> auto toImmediate(n32 constant) const -> Immediate<T>;
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
  template<typename Source> auto instructionBitSearch1Backward(Source) -> void;
  template<typename Source> auto instructionBitSearch1Forward(Source) -> void;
  template<typename Source> auto instructionCall(Source) -> void;
  template<typename Source> auto instructionCallRelative(Source) -> void;
  template<typename Target, typename Offset> auto instructionChange(Target, Offset) -> void;
  template<typename Size, s32 Adjust, typename Target, typename Source> auto instructionCompare(Target, Source) -> void;
  template<typename Size, s32 Adjust, typename Target, typename Source> auto instructionCompareRepeat(Target, Source) -> void;
  template<typename Target, typename Source> auto instructionCompare(Target, Source) -> void;
  template<typename Target> auto instructionComplement(Target) -> void;
  template<typename Modify> auto instructionDecimalAdjustAccumulator(Modify) -> void;
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
  template<typename Size, s32 Adjust, typename Target, typename Source> auto instructionLoad(Target, Source) -> void;
  template<typename Size, s32 Adjust, typename Target, typename Source> auto instructionLoadRepeat(Target, Source) -> void;
  template<u32 Modulo, typename Target, typename Source> auto instructionModuloDecrement(Target, Source) -> void;
  template<u32 Modulo, typename Target, typename Source> auto instructionModuloIncrement(Target, Source) -> void;
  template<typename Modify> auto instructionMirror(Modify) -> void;
  template<typename Target, typename Source> auto instructionMultiply(Target, Source) -> void;
  template<typename Modify> auto instructionMultiplyAdd(Modify) -> void;
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
    struct { n8 order_lsb4(b0, b1, b2, b3); } b;
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
  } r;

  n1 CF, CA;  //carry
  n1 NF, NA;  //negative
  n1 VF, VA;  //overflow or parity
  n1 HF, HA;  //half carry
  n1 ZF, ZA;  //zero
  n1 SF, SA;  //sign
  n2 RFP;     //register file pointer
  n3 IFF;     //interrupt mask flip-flop

  n8 OP;             //first opcode byte
  n1 HALT;           //set if HALT instruction executed
  n8 PIC;            //prefetch instruction counter
  queue<u8[4]> PIQ;  //prefetch instruction queue
  n24 MAR;           //A0-A23: memory address register
  n16 MDR;           //D0-D15: memory data register

  static inline const Register<n8 > RA0 {0x00}; static inline const Register<n8 > RW0 {0x01};
  static inline const Register<n8 > QA0 {0x02}; static inline const Register<n8 > QW0 {0x03};
  static inline const Register<n8 > RC0 {0x04}; static inline const Register<n8 > RB0 {0x05};
  static inline const Register<n8 > QC0 {0x06}; static inline const Register<n8 > QB0 {0x07};
  static inline const Register<n8 > RE0 {0x08}; static inline const Register<n8 > RD0 {0x09};
  static inline const Register<n8 > QE0 {0x0a}; static inline const Register<n8 > QD0 {0x0b};
  static inline const Register<n8 > RL0 {0x0c}; static inline const Register<n8 > RH0 {0x0d};
  static inline const Register<n8 > QL0 {0x0e}; static inline const Register<n8 > QH0 {0x0f};
  static inline const Register<n8 > RA1 {0x10}; static inline const Register<n8 > RW1 {0x11};
  static inline const Register<n8 > QA1 {0x12}; static inline const Register<n8 > QW1 {0x13};
  static inline const Register<n8 > RC1 {0x14}; static inline const Register<n8 > RB1 {0x15};
  static inline const Register<n8 > QC1 {0x16}; static inline const Register<n8 > QB1 {0x17};
  static inline const Register<n8 > RE1 {0x18}; static inline const Register<n8 > RD1 {0x19};
  static inline const Register<n8 > QE1 {0x1a}; static inline const Register<n8 > QD1 {0x1b};
  static inline const Register<n8 > RL1 {0x1c}; static inline const Register<n8 > RH1 {0x1d};
  static inline const Register<n8 > QL1 {0x1e}; static inline const Register<n8 > QH1 {0x1f};
  static inline const Register<n8 > RA2 {0x20}; static inline const Register<n8 > RW2 {0x21};
  static inline const Register<n8 > QA2 {0x22}; static inline const Register<n8 > QW2 {0x23};
  static inline const Register<n8 > RC2 {0x24}; static inline const Register<n8 > RB2 {0x25};
  static inline const Register<n8 > QC2 {0x26}; static inline const Register<n8 > QB2 {0x27};
  static inline const Register<n8 > RE2 {0x28}; static inline const Register<n8 > RD2 {0x29};
  static inline const Register<n8 > QE2 {0x2a}; static inline const Register<n8 > QD2 {0x2b};
  static inline const Register<n8 > RL2 {0x2c}; static inline const Register<n8 > RH2 {0x2d};
  static inline const Register<n8 > QL2 {0x2e}; static inline const Register<n8 > QH2 {0x2f};
  static inline const Register<n8 > RA3 {0x30}; static inline const Register<n8 > RW3 {0x31};
  static inline const Register<n8 > QA3 {0x32}; static inline const Register<n8 > QW3 {0x33};
  static inline const Register<n8 > RC3 {0x34}; static inline const Register<n8 > RB3 {0x35};
  static inline const Register<n8 > QC3 {0x36}; static inline const Register<n8 > QB3 {0x37};
  static inline const Register<n8 > RE3 {0x38}; static inline const Register<n8 > RD3 {0x39};
  static inline const Register<n8 > QE3 {0x3a}; static inline const Register<n8 > QD3 {0x3b};
  static inline const Register<n8 > RL3 {0x3c}; static inline const Register<n8 > RH3 {0x3d};
  static inline const Register<n8 > QL3 {0x3e}; static inline const Register<n8 > QH3 {0x3f};
  static inline const Register<n8 > A_  {0xd0}; static inline const Register<n8 > W_  {0xd1};
  static inline const Register<n8 > QA_ {0xd2}; static inline const Register<n8 > QW_ {0xd3};
  static inline const Register<n8 > C_  {0xd4}; static inline const Register<n8 > B_  {0xd5};
  static inline const Register<n8 > QC_ {0xd6}; static inline const Register<n8 > QB_ {0xd7};
  static inline const Register<n8 > E_  {0xd8}; static inline const Register<n8 > D_  {0xd9};
  static inline const Register<n8 > QE_ {0xda}; static inline const Register<n8 > QD_ {0xdb};
  static inline const Register<n8 > L_  {0xdc}; static inline const Register<n8 > H_  {0xdd};
  static inline const Register<n8 > QL_ {0xde}; static inline const Register<n8 > QH_ {0xdf};
  static inline const Register<n8 > A   {0xe0}; static inline const Register<n8 > W   {0xe1};
  static inline const Register<n8 > QA  {0xe2}; static inline const Register<n8 > QW  {0xe3};
  static inline const Register<n8 > C   {0xe4}; static inline const Register<n8 > B   {0xe5};
  static inline const Register<n8 > QC  {0xe6}; static inline const Register<n8 > QB  {0xe7};
  static inline const Register<n8 > E   {0xe8}; static inline const Register<n8 > D   {0xe9};
  static inline const Register<n8 > QE  {0xea}; static inline const Register<n8 > QD  {0xeb};
  static inline const Register<n8 > L   {0xec}; static inline const Register<n8 > H   {0xed};
  static inline const Register<n8 > QL  {0xee}; static inline const Register<n8 > QH  {0xef};
  static inline const Register<n8 > IXL {0xf0}; static inline const Register<n8 > IXH {0xf1};
  static inline const Register<n8 > QIXL{0xf2}; static inline const Register<n8 > QIXH{0xf3};
  static inline const Register<n8 > IYL {0xf4}; static inline const Register<n8 > IYH {0xf5};
  static inline const Register<n8 > QIYL{0xf6}; static inline const Register<n8 > QIYH{0xf7};
  static inline const Register<n8 > IZL {0xf8}; static inline const Register<n8 > IZH {0xf9};
  static inline const Register<n8 > QIZL{0xfa}; static inline const Register<n8 > QIZH{0xfb};
  static inline const Register<n8 > SPL {0xfc}; static inline const Register<n8 > SPH {0xfd};
  static inline const Register<n8 > QSPL{0xfe}; static inline const Register<n8 > QSPH{0xff};

  static inline const Register<n16> RWA0{0x00}; static inline const Register<n16> QWA0{0x02};
  static inline const Register<n16> RBC0{0x04}; static inline const Register<n16> QBC0{0x06};
  static inline const Register<n16> RDE0{0x08}; static inline const Register<n16> QDE0{0x0a};
  static inline const Register<n16> RHL0{0x0c}; static inline const Register<n16> QHL0{0x0e};
  static inline const Register<n16> RWA1{0x10}; static inline const Register<n16> QWA1{0x12};
  static inline const Register<n16> RBC1{0x14}; static inline const Register<n16> QBC1{0x16};
  static inline const Register<n16> RDE1{0x18}; static inline const Register<n16> QDE1{0x1a};
  static inline const Register<n16> RHL1{0x1c}; static inline const Register<n16> QHL1{0x1e};
  static inline const Register<n16> RWA2{0x20}; static inline const Register<n16> QWA2{0x22};
  static inline const Register<n16> RBC2{0x24}; static inline const Register<n16> QBC2{0x26};
  static inline const Register<n16> RDE2{0x28}; static inline const Register<n16> QDE2{0x2a};
  static inline const Register<n16> RHL2{0x2c}; static inline const Register<n16> QHL2{0x2e};
  static inline const Register<n16> RWA3{0x30}; static inline const Register<n16> QWA3{0x32};
  static inline const Register<n16> RBC3{0x34}; static inline const Register<n16> QBC3{0x36};
  static inline const Register<n16> RDE3{0x38}; static inline const Register<n16> QDE3{0x3a};
  static inline const Register<n16> RHL3{0x3c}; static inline const Register<n16> QHL3{0x3e};
  static inline const Register<n16> WA_ {0xd0}; static inline const Register<n16> QWA_{0xd2};
  static inline const Register<n16> BC_ {0xd4}; static inline const Register<n16> QBC_{0xd6};
  static inline const Register<n16> DE_ {0xd8}; static inline const Register<n16> QDE_{0xda};
  static inline const Register<n16> HL_ {0xdc}; static inline const Register<n16> QHL_{0xde};
  static inline const Register<n16> WA  {0xe0}; static inline const Register<n16> QWA {0xe2};
  static inline const Register<n16> BC  {0xe4}; static inline const Register<n16> QBC {0xe6};
  static inline const Register<n16> DE  {0xe8}; static inline const Register<n16> QDE {0xea};
  static inline const Register<n16> HL  {0xec}; static inline const Register<n16> QHL {0xee};
  static inline const Register<n16> IX  {0xf0}; static inline const Register<n16> QIX {0xf2};
  static inline const Register<n16> IY  {0xf4}; static inline const Register<n16> QIY {0xf6};
  static inline const Register<n16> IZ  {0xf8}; static inline const Register<n16> QIZ {0xfa};
  static inline const Register<n16> SP  {0xfc}; static inline const Register<n16> QSP {0xfe};

  static inline const Register<n32> XWA0{0x00};
  static inline const Register<n32> XBC0{0x04};
  static inline const Register<n32> XDE0{0x08};
  static inline const Register<n32> XHL0{0x0c};
  static inline const Register<n32> XWA1{0x10};
  static inline const Register<n32> XBC1{0x14};
  static inline const Register<n32> XDE1{0x18};
  static inline const Register<n32> XHL1{0x1c};
  static inline const Register<n32> XWA2{0x20};
  static inline const Register<n32> XBC2{0x24};
  static inline const Register<n32> XDE2{0x28};
  static inline const Register<n32> XHL2{0x2c};
  static inline const Register<n32> XWA3{0x30};
  static inline const Register<n32> XBC3{0x34};
  static inline const Register<n32> XDE3{0x38};
  static inline const Register<n32> XHL3{0x3c};
  static inline const Register<n32> XWA_{0xd0};
  static inline const Register<n32> XBC_{0xd4};
  static inline const Register<n32> XDE_{0xd8};
  static inline const Register<n32> XHL_{0xdc};
  static inline const Register<n32> XWA {0xe0};
  static inline const Register<n32> XBC {0xe4};
  static inline const Register<n32> XDE {0xe8};
  static inline const Register<n32> XHL {0xec};
  static inline const Register<n32> XIX {0xf0};
  static inline const Register<n32> XIY {0xf4};
  static inline const Register<n32> XIZ {0xf8};
  static inline const Register<n32> XSP {0xfc};

  static inline const FlagRegister F {0};
  static inline const FlagRegister FP{1};

  static inline const StatusRegister SR{};
  static inline const ProgramCounter PC{};

  static inline const ControlRegister<n16> DMAS0L {0x00}; static inline const ControlRegister<n16> DMAS0H{0x02};
  static inline const ControlRegister<n16> DMAS1L {0x04}; static inline const ControlRegister<n16> DMAS1H{0x06};
  static inline const ControlRegister<n16> DMAS2L {0x08}; static inline const ControlRegister<n16> DMAS2H{0x0a};
  static inline const ControlRegister<n16> DMAS3L {0x0c}; static inline const ControlRegister<n16> DMAS3H{0x0e};
  static inline const ControlRegister<n16> DMAD0L {0x10}; static inline const ControlRegister<n16> DMAD0H{0x12};
  static inline const ControlRegister<n16> DMAD1L {0x14}; static inline const ControlRegister<n16> DMAD1H{0x16};
  static inline const ControlRegister<n16> DMAD2L {0x18}; static inline const ControlRegister<n16> DMAD2H{0x1a};
  static inline const ControlRegister<n16> DMAD3L {0x1c}; static inline const ControlRegister<n16> DMAD3H{0x1e};
  static inline const ControlRegister<n16> DMAC0  {0x20}; static inline const ControlRegister<n16> DMAC0H{0x22};
  static inline const ControlRegister<n16> DMAC1  {0x24}; static inline const ControlRegister<n16> DMAC1H{0x26};
  static inline const ControlRegister<n16> DMAC2  {0x28}; static inline const ControlRegister<n16> DMAC2H{0x2a};
  static inline const ControlRegister<n16> DMAC3  {0x2c}; static inline const ControlRegister<n16> DMAC3H{0x2e};
  static inline const ControlRegister<n16> INTNEST{0x3c};

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

  static inline const n4 False{0x00};
  static inline const n4 True {0x08};

  static inline const n1 Undefined = 0;

  //disassembler.cpp
  virtual auto disassembleRead(n24 address) -> n8 { return read(Byte, address); }
  auto disassembleInstruction() -> string;
  auto disassembleContext() -> string;
};

}
