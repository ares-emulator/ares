#include "registers.hpp"
#include "scc-registers.cpp"
#include "memory.cpp"
#include "instruction.cpp"
#include "cpu-instructions.cpp"
#include "scc-instructions.cpp"
#include "fpu-instructions.cpp"
#include "disassembler.cpp"
#include "serialization.cpp"

auto CPU::powerR4300() -> void {
  //ROUND.W/L conform to IEEE 754, which uses round-to-nearest mode.
  //ensure that we are in this mode to prevent off-by-one errors.
  //note: this is an expensive operation, so we assume this won't be changed.
  //if other code elsewhere changes this, it will harm emulation accuracy.
  fesetround(FE_TONEAREST);

  for(uint n : range(32)) GPR[n].u64 = 0;
  scc = {};
  LO.u64 = 0;
  HI.u64 = 0;
  PC = 0xbfc00000;
  IP = nothing;
}
