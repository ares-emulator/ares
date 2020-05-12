#include "registers.hpp"
#include "scc-registers.cpp"
#include "fpu-registers.cpp"
#include "tlb.cpp"
#include "memory.cpp"
#include "instruction.cpp"
#include "cpu-instructions.cpp"
#include "scc-instructions.cpp"
#include "fpu-instructions.cpp"
#include "disassembler.cpp"
#include "serialization.cpp"

auto CPU::powerR4300() -> void {
  for(uint n : range(32)) GPR[n].u64 = 0;
  scc = {};
  LO.u64 = 0;
  HI.u64 = 0;
  PC = 0xbfc00000;
  IP = nothing;
  fesetround(FE_TONEAREST);
}
