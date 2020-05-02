#include "registers.hpp"
#include "memory.cpp"
#include "instruction.cpp"
#include "instructions-core.cpp"
#include "instructions-cop0.cpp"
#include "instructions-cop1.cpp"
#include "disassembler.cpp"
#include "serialization.cpp"

auto CPU::powerR4300() -> void {
  for(uint n : range(32)) GPR[n].u64 = 0;
  LO.u64 = 0;
  HI.u64 = 0;
  PC = 0xbfc00000;
  IP = nothing;
  LLBIT = 0;
}
