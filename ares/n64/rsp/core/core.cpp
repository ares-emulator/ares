#include "registers.hpp"
#include "scc-registers.cpp"
#include "memory.cpp"
#include "instruction.cpp"
#include "cpu-instructions.cpp"
#include "scc-instructions.cpp"
#include "vu-instructions.cpp"
#include "disassembler.cpp"
#include "serialization.cpp"

auto RSP::powerCore() -> void {
  for(uint n : range(32)) GPR[n].u32 = 0;
  LO.u32 = 0;
  HI.u32 = 0;
  PC = 0;
  IP = nothing;
  scc = {};
  for(uint n : range(32)) VPR[n].u128 = 0;
  ACCH = zero;
  ACCM = zero;
  ACCL = zero;
  VCOH = zero;
  VCOL = zero;
  VCCH = zero;
  VCCL = zero;
  VCE  = zero;
  DIVIN  = 0;
  DIVOUT = 0;
  DIVDP  = 0;

  reciprocals[0] = u16(~0);
  for(u16 index : range(1, 512)) {
    u64 a = index + 512;
    u64 b = (u64(1) << 34) / a;
    reciprocals[index] = u16(b + 1 >> 8);
  }

  for(u16 index : range(0, 512)) {
    u64 a = index + 512 >> (index % 2 == 1);
    u64 b = 1 << 17;
    //find the largest b where b < 1.0 / sqrt(a)
    while(a * (b + 1) * (b + 1) < (u64(1) << 44)) b++;
    inverseSquareRoots[index] = u16(b >> 1);
  }
}
