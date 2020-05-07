#include "registers.hpp"
#include "memory.cpp"
#include "instruction.cpp"
#include "instructions-core.cpp"
#include "instructions-cop0.cpp"
#include "instructions-cop2.cpp"
#include "disassembler.cpp"
#include "serialization.cpp"

auto RSP::powerCore() -> void {
  for(uint n : range(32)) GPR[n].u64 = 0;
  for(uint n : range(32)) SPR[n].u64 = 0;
  for(uint n : range(32)) VPR[n].u128 = 0;
  SPR[COP0::Register::Status].u64 = 0xf400'0000;
  LO.u64 = 0;
  HI.u64 = 0;
  PC = 0;
  IP = nothing;
  LLBIT = 0;

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
