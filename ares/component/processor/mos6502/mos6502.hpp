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
  virtual auto delayIrq() -> void {}
  virtual auto cancelNmi() -> void {}

  //mos6502.cpp
  auto power(bool reset = false) -> void;

  //memory.cpp
  auto opcode() -> n8;
  auto load(n8 addr) -> n8;
  auto push(n8 data) -> void;
  auto pull() -> n8;

  //instruction.cpp
  auto interrupt() -> void;
  auto instruction() -> void;

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

  n16 MAR;
  i16 TI16;
  n16 TN16;
  n8  TN8;
};

}
