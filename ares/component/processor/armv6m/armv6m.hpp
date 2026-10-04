#pragma once

//ARMv6-M (Cortex-M0 instruction profile)

namespace ares {

struct ARMv6M {
  enum class Fault : u32 {
    None,
    Fetch,
    Read,
    Write,
    Alignment,
    Undefined,
    Breakpoint,
    Suspend,
    Return,
  };

  enum Mode : u32 {
    Load = 1 << 0,
    Store = 1 << 1,
    Prefetch = 1 << 2,
    Byte = 1 << 3,
    Half = 1 << 4,
    Word = 1 << 5,
    Signed = 1 << 6,
  };

  virtual auto step(u32 clocks) -> void = 0;
  virtual auto get(u32 mode, n32 address, n32& data) -> Fault = 0;
  virtual auto getDebugger(u32 mode, n32 address, n32& data) -> Fault { return get(mode, address, data); }
  virtual auto set(u32 mode, n32 address, n32 data) -> Fault = 0;

  auto power() -> void;
  auto instruction() -> Fault;
  auto disassembleInstruction(maybe<n32> pc = {}) -> string;
  auto disassembleContext() -> string;
  auto serialize(serializer&) -> void;

  enum class InstructionFamily : u32 {
    ShiftImmediate,
    AddSubtract,
    Immediate,
    DataProcessing,
    HighRegister,
    LoadLiteral,
    LoadStoreRegister,
    LoadStoreImmediate,
    LoadStoreHalf,
    LoadStoreStack,
    AddAddress,
    AdjustStack,
    ChangeInterrupt,
    SetEndian,
    Extend,
    Push,
    Reverse,
    Breakpoint,
    Hint,
    Pop,
    LoadStoreMultiple,
    BranchConditional,
    Branch,
    BranchLink,
    Undefined,
  };

  struct InstructionDecode {
    InstructionFamily family = InstructionFamily::Undefined;
    u32 width = 2;
    bool valid = false;
  };

  static auto decodeInstruction(n16 prefix, maybe<n16> suffix = {}) -> InstructionDecode;

  struct PSR {
    b1 n;
    b1 z;
    b1 c;
    b1 v;
    n8 it;

    auto serialize(serializer&) -> void;
  };

  struct Processor {
    n32 r[16];
    PSR psr;

    auto serialize(serializer&) -> void;
  } processor;

  auto r(u32 index) -> n32& { return processor.r[index & 15]; }
  auto r(u32 index) const -> const n32& { return processor.r[index & 15]; }
  auto psr() -> PSR& { return processor.psr; }
  auto psr() const -> const PSR& { return processor.psr; }

protected:
  auto load(u32 mode, u32 address, n32& data) -> Fault;
  auto store(u32 mode, u32 address, n32 data) -> Fault;
  auto branch(u32 address) -> Fault;
  auto condition(u32 code) const -> bool;
  auto setNZ(u32 value) -> void;
  auto add(u32 lhs, u32 rhs, u32 carry, bool flags = true) -> u32;
  auto sub(u32 lhs, u32 rhs, u32 carry, bool flags = true) -> u32;
  auto shiftLSL(u32 value, u32 amount, bool flags = true) -> u32;
  auto shiftLSR(u32 value, u32 amount, bool flags = true) -> u32;
  auto shiftASR(u32 value, u32 amount, bool flags = true) -> u32;
  auto shiftROR(u32 value, u32 amount, bool flags = true) -> u32;

  n16 opcode;
  n32 instructionAddress;
};

}
