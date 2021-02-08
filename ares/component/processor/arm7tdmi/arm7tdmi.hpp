//ARMv4 (ARM7TDMI)

#pragma once

namespace ares {

struct ARM7TDMI {
  enum : u32 {
    Nonsequential = 1 << 0,  //N cycle
    Sequential    = 1 << 1,  //S cycle
    Prefetch      = 1 << 2,  //instruction fetch
    Byte          = 1 << 3,  // 8-bit access
    Half          = 1 << 4,  //16-bit access
    Word          = 1 << 5,  //32-bit access
    Load          = 1 << 6,  //load operation
    Store         = 1 << 7,  //store operation
    Signed        = 1 << 8,  //sign-extend
  };

  virtual auto step(u32 clocks) -> void = 0;
  virtual auto sleep() -> void = 0;
  virtual auto get(u32 mode, n32 address) -> n32 = 0;
  virtual auto set(u32 mode, n32 address, n32 word) -> void = 0;

  //arm7tdmi.cpp
  ARM7TDMI();
  auto power() -> void;

  //registers.cpp
  struct GPR;
  struct PSR;
  auto r(n4) -> GPR&;
  auto cpsr() -> PSR&;
  auto spsr() -> PSR&;
  auto privileged() const -> bool;
  auto exception() const -> bool;

  //memory.cpp
  auto idle() -> void;
  auto read(u32 mode, n32 address) -> n32;
  auto load(u32 mode, n32 address) -> n32;
  auto write(u32 mode, n32 address, n32 word) -> void;
  auto store(u32 mode, n32 address, n32 word) -> void;

  //algorithms.cpp
  auto ADD(n32, n32, bool) -> n32;
  auto ASR(n32, n8) -> n32;
  auto BIT(n32) -> n32;
  auto LSL(n32, n8) -> n32;
  auto LSR(n32, n8) -> n32;
  auto MUL(n32, n32, n32) -> n32;
  auto ROR(n32, n8) -> n32;
  auto RRX(n32) -> n32;
  auto SUB(n32, n32, bool) -> n32;
  auto TST(n4) -> bool;

  //instruction.cpp
  auto fetch() -> void;
  auto instruction() -> void;
  auto exception(u32 mode, n32 address) -> void;
  auto armInitialize() -> void;
  auto thumbInitialize() -> void;

  //instructions-arm.cpp
  auto armALU(n4 mode, n4 target, n4 source, n32 data) -> void;
  auto armMoveToStatus(n4 field, n1 source, n32 data) -> void;

  auto armInstructionBranch(i24, n1) -> void;
  auto armInstructionBranchExchangeRegister(n4) -> void;
  auto armInstructionDataImmediate(n8, n4, n4, n4, n1, n4) -> void;
  auto armInstructionDataImmediateShift(n4, n2, n5, n4, n4, n1, n4) -> void;
  auto armInstructionDataRegisterShift(n4, n2, n4, n4, n4, n1, n4) -> void;
  auto armInstructionLoadImmediate(n8, n1, n4, n4, n1, n1, n1) -> void;
  auto armInstructionLoadRegister(n4, n1, n4, n4, n1, n1, n1) -> void;
  auto armInstructionMemorySwap(n4, n4, n4, n1) -> void;
  auto armInstructionMoveHalfImmediate(n8, n4, n4, n1, n1, n1, n1) -> void;
  auto armInstructionMoveHalfRegister(n4, n4, n4, n1, n1, n1, n1) -> void;
  auto armInstructionMoveImmediateOffset(n12, n4, n4, n1, n1, n1, n1, n1) -> void;
  auto armInstructionMoveMultiple(n16, n4, n1, n1, n1, n1, n1) -> void;
  auto armInstructionMoveRegisterOffset(n4, n2, n5, n4, n4, n1, n1, n1, n1, n1) -> void;
  auto armInstructionMoveToRegisterFromStatus(n4, n1) -> void;
  auto armInstructionMoveToStatusFromImmediate(n8, n4, n4, n1) -> void;
  auto armInstructionMoveToStatusFromRegister(n4, n4, n1) -> void;
  auto armInstructionMultiply(n4, n4, n4, n4, n1, n1) -> void;
  auto armInstructionMultiplyLong(n4, n4, n4, n4, n1, n1, n1) -> void;
  auto armInstructionSoftwareInterrupt(n24 immediate) -> void;
  auto armInstructionUndefined() -> void;

  //instructions-thumb.cpp
  auto thumbInstructionALU(n3, n3, n4) -> void;
  auto thumbInstructionALUExtended(n4, n4, n2) -> void;
  auto thumbInstructionAddRegister(n8, n3, n1) -> void;
  auto thumbInstructionAdjustImmediate(n3, n3, n3, n1) -> void;
  auto thumbInstructionAdjustRegister(n3, n3, n3, n1) -> void;
  auto thumbInstructionAdjustStack(n7, n1) -> void;
  auto thumbInstructionBranchExchange(n4) -> void;
  auto thumbInstructionBranchFarPrefix(i11) -> void;
  auto thumbInstructionBranchFarSuffix(n11) -> void;
  auto thumbInstructionBranchNear(i11) -> void;
  auto thumbInstructionBranchTest(i8, n4) -> void;
  auto thumbInstructionImmediate(n8, n3, n2) -> void;
  auto thumbInstructionLoadLiteral(n8, n3) -> void;
  auto thumbInstructionMoveByteImmediate(n3, n3, n5, n1) -> void;
  auto thumbInstructionMoveHalfImmediate(n3, n3, n5, n1) -> void;
  auto thumbInstructionMoveMultiple(n8, n3, n1) -> void;
  auto thumbInstructionMoveRegisterOffset(n3, n3, n3, n3) -> void;
  auto thumbInstructionMoveStack(n8, n3, n1) -> void;
  auto thumbInstructionMoveWordImmediate(n3, n3, n5, n1) -> void;
  auto thumbInstructionShiftImmediate(n3, n3, n5, n2) -> void;
  auto thumbInstructionSoftwareInterrupt(n8) -> void;
  auto thumbInstructionStackMultiple(n8, n1, n1) -> void;
  auto thumbInstructionUndefined() -> void;

  //serialization.cpp
  auto serialize(serializer&) -> void;

  //disassembler.cpp
  auto disassembleInstruction(maybe<n32> pc = {}, maybe<boolean> thumb = {}) -> string;
  auto disassembleContext() -> string;

  struct GPR {
    operator u32() const { return data; }
    auto operator=(const GPR& value) -> GPR& { return operator=(value.data); }

    auto operator=(n32 value) -> GPR& {
      data = value;
      if(modify) modify();
      return *this;
    }

    n32 data;
    function<auto () -> void> modify;
  };

  struct PSR {
    enum : u32 {
      USR = 0x10,  //user
      FIQ = 0x11,  //fast interrupt
      IRQ = 0x12,  //interrupt
      SVC = 0x13,  //service
      ABT = 0x17,  //abort
      UND = 0x1b,  //undefined
      SYS = 0x1f,  //system
    };

    operator u32() const {
      return m << 0 | t << 5 | f << 6 | i << 7 | v << 28 | c << 29 | z << 30 | n << 31;
    }

    auto operator=(n32 data) -> PSR& {
      m = data.bit(0,4);
      t = data.bit(5);
      f = data.bit(6);
      i = data.bit(7);
      v = data.bit(28);
      c = data.bit(29);
      z = data.bit(30);
      n = data.bit(31);
      return *this;
    }

    //serialization.cpp
    auto serialize(serializer&) -> void;

    n5 m;  //mode
    b1 t;  //thumb
    b1 f;  //fiq
    b1 i;  //irq
    b1 v;  //overflow
    b1 c;  //carry
    b1 z;  //zero
    b1 n;  //negative
  };

  struct Processor {
    //serialization.cpp
    auto serialize(serializer&) -> void;

    GPR r0, r1, r2, r3, r4, r5, r6, r7, r8, r9, r10, r11, r12, r13, r14, r15;
    PSR cpsr;

    struct FIQ {
      GPR r8, r9, r10, r11, r12, r13, r14;
      PSR spsr;
    } fiq;

    struct IRQ {
      GPR r13, r14;
      PSR spsr;
    } irq;

    struct SVC {
      GPR r13, r14;
      PSR spsr;
    } svc;

    struct ABT {
      GPR r13, r14;
      PSR spsr;
    } abt;

    struct UND {
      GPR r13, r14;
      PSR spsr;
    } und;
  } processor;

  struct Pipeline {
    //serialization.cpp
    auto serialize(serializer&) -> void;

    struct Instruction {
      n32 address;
      n32 instruction;
      b1  thumb;  //not used by fetch stage
    };

    n1 reload = 1;
    n1 nonsequential = 1;
    Instruction fetch;
    Instruction decode;
    Instruction execute;
  } pipeline;

  n32 opcode;
  b1  carry;
  b1  irq;

  function<void (n32 opcode)> armInstruction[4096];
  function<void ()> thumbInstruction[65536];

  //disassembler.cpp
  auto armDisassembleBranch(i24, n1) -> string;
  auto armDisassembleBranchExchangeRegister(n4) -> string;
  auto armDisassembleDataImmediate(n8, n4, n4, n4, n1, n4) -> string;
  auto armDisassembleDataImmediateShift(n4, n2, n5, n4, n4, n1, n4) -> string;
  auto armDisassembleDataRegisterShift(n4, n2, n4, n4, n4, n1, n4) -> string;
  auto armDisassembleLoadImmediate(n8, n1, n4, n4, n1, n1, n1) -> string;
  auto armDisassembleLoadRegister(n4, n1, n4, n4, n1, n1, n1) -> string;
  auto armDisassembleMemorySwap(n4, n4, n4, n1) -> string;
  auto armDisassembleMoveHalfImmediate(n8, n4, n4, n1, n1, n1, n1) -> string;
  auto armDisassembleMoveHalfRegister(n4, n4, n4, n1, n1, n1, n1) -> string;
  auto armDisassembleMoveImmediateOffset(n12, n4, n4, n1, n1, n1, n1, n1) -> string;
  auto armDisassembleMoveMultiple(n16, n4, n1, n1, n1, n1, n1) -> string;
  auto armDisassembleMoveRegisterOffset(n4, n2, n5, n4, n4, n1, n1, n1, n1, n1) -> string;
  auto armDisassembleMoveToRegisterFromStatus(n4, n1) -> string;
  auto armDisassembleMoveToStatusFromImmediate(n8, n4, n4, n1) -> string;
  auto armDisassembleMoveToStatusFromRegister(n4, n4, n1) -> string;
  auto armDisassembleMultiply(n4, n4, n4, n4, n1, n1) -> string;
  auto armDisassembleMultiplyLong(n4, n4, n4, n4, n1, n1, n1) -> string;
  auto armDisassembleSoftwareInterrupt(n24) -> string;
  auto armDisassembleUndefined() -> string;

  auto thumbDisassembleALU(n3, n3, n4) -> string;
  auto thumbDisassembleALUExtended(n4, n4, n2) -> string;
  auto thumbDisassembleAddRegister(n8, n3, n1) -> string;
  auto thumbDisassembleAdjustImmediate(n3, n3, n3, n1) -> string;
  auto thumbDisassembleAdjustRegister(n3, n3, n3, n1) -> string;
  auto thumbDisassembleAdjustStack(n7, n1) -> string;
  auto thumbDisassembleBranchExchange(n4) -> string;
  auto thumbDisassembleBranchFarPrefix(i11) -> string;
  auto thumbDisassembleBranchFarSuffix(n11) -> string;
  auto thumbDisassembleBranchNear(i11) -> string;
  auto thumbDisassembleBranchTest(i8, n4) -> string;
  auto thumbDisassembleImmediate(n8, n3, n2) -> string;
  auto thumbDisassembleLoadLiteral(n8, n3) -> string;
  auto thumbDisassembleMoveByteImmediate(n3, n3, n5, n1) -> string;
  auto thumbDisassembleMoveHalfImmediate(n3, n3, n5, n1) -> string;
  auto thumbDisassembleMoveMultiple(n8, n3, n1) -> string;
  auto thumbDisassembleMoveRegisterOffset(n3, n3, n3, n3) -> string;
  auto thumbDisassembleMoveStack(n8, n3, n1) -> string;
  auto thumbDisassembleMoveWordImmediate(n3, n3, n5, n1) -> string;
  auto thumbDisassembleShiftImmediate(n3, n3, n5, n2) -> string;
  auto thumbDisassembleSoftwareInterrupt(n8) -> string;
  auto thumbDisassembleStackMultiple(n8, n1, n1) -> string;
  auto thumbDisassembleUndefined() -> string;

  function<string (n32 opcode)> armDisassemble[4096];
  function<string ()> thumbDisassemble[65536];

  n32 _pc;
  string _c;
};

}
