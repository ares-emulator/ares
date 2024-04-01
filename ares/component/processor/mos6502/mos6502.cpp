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
#include "instruction.cpp"
#include "disassembler.cpp"
#include "serialization.cpp"

auto MOS6502::interrupt() -> void {
  read(PC); // dummy read
  read(PC); // dummy read
  push(PCH);
  push(PCL);
  n16 vector = 0xfffe;
  nmi(vector);
  push(P | 0x20);
  I = 1;
  PCL = read(vector++);
L PCH = read(vector++);
}

auto MOS6502::power(bool reset) -> void {
 if(reset) {
   S-=3;
   P.i = 1;
   return;
 }

  A   = 0x00;
  X   = 0x00;
  Y   = 0x00;
  S   = 0xfd;
  P   = 0x04;
  MDR = 0x00;
}

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

}
