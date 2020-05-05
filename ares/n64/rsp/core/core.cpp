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
}
