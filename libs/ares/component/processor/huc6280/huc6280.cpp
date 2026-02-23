#include <ares/ares.hpp>
#include "huc6280.hpp"

namespace ares {

#define A r.a
#define X r.x
#define Y r.y
#define S r.s
#define PC r.pc
#define PCH r.pc.byte(1)
#define PCL r.pc.byte(0)
#define MPR r.mpr
#define MPL r.mpl
#define CS r.cs
#define P r.p
#define C r.p.c
#define Z r.p.z
#define I r.p.i
#define D r.p.d
#define B r.p.b
#define T r.p.t
#define V r.p.v
#define N r.p.n
#define EA r.ea
#define L lastCycle();
#define ALU (this->*alu)

#include "memory.cpp"
#include "algorithms.cpp"
#include "instruction.cpp"
#include "instructions.cpp"
#include "disassembler.cpp"
#include "serialization.cpp"

auto HuC6280::power() -> void {
  random.entropy(Random::Entropy::High);

  A = random();
  X = random();
  Y = random();
  S = random();
  PC = 0x0000;
  MPR[0] = random();
  MPR[1] = random();
  MPR[2] = random();
  MPR[3] = random();
  MPR[4] = random();
  MPR[5] = random();
  MPR[6] = random();
  MPR[7] = 0x00;
  MPL = 0x00;
  C = random();
  Z = random();
  I = 1;
  D = 0;
  B = 1;
  T = 0;
  V = random();
  N = random();
  CS = 12;

  blockMove = false;
}

#undef A
#undef X
#undef Y
#undef S
#undef PC
#undef PCH
#undef PCL
#undef MPR
#undef MPL
#undef CS
#undef P
#undef C
#undef Z
#undef I
#undef D
#undef B
#undef T
#undef V
#undef N
#undef EA
#undef L
#undef ALU

}
