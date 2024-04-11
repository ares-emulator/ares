#include <ares/ares.hpp>
#include "mos6502.hpp"

namespace ares {

#define PCH PC.byte(1)
#define PCL PC.byte(0)
#define ADDR (this->*mode)
#define ALG (this->*alg)
#define C P.c
#define Z P.z
#define I P.i
#define D P.d
#define V P.v
#define N P.n
#define L lastCycle();

#include "addresses.cpp"
#include "memory.cpp"
#include "algorithms.cpp"
#include "instruction.cpp"
#include "instructions.cpp"
#include "disassembler.cpp"
#include "serialization.cpp"

#undef PCH
#undef PCL
#undef ADDR
#undef ALG
#undef C
#undef Z
#undef I
#undef D
#undef V
#undef N
#undef L

auto MOS6502::power(bool reset) -> void {
 if(reset) {
   resetting = 1;
   return;
 }

  A   = 0x00;
  X   = 0x00;
  Y   = 0x00;
  S   = 0x00;
  P   = 0x04;
  MDR = 0x00;
  resetting = 1;
}

}
