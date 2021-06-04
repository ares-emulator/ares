#include <ares/ares.hpp>
#include "mos6502.hpp"

namespace ares {

#define PCH PC.byte(1)
#define PCL PC.byte(0)
#define ALU (this->*alu)
#define C P.c
#define Z P.z
#define I P.i
#define D P.d
#define V P.v
#define N P.n
#define L lastCycle();

#include "memory.cpp"
#include "algorithms.cpp"
#include "instruction.cpp"
#include "instructions.cpp"
#include "disassembler.cpp"
#include "serialization.cpp"

#undef PCH
#undef PCL
#undef ALU
#undef C
#undef Z
#undef I
#undef D
#undef V
#undef N
#undef L

auto MOS6502::power() -> void {
  A   = 0x00;
  X   = 0x00;
  Y   = 0x00;
  S   = 0xff;
  P   = 0x04;
  MDR = 0x00;
}

}
