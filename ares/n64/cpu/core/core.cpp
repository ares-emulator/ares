#include "registers.hpp"
#include "memory.cpp"
#include "instruction.cpp"
#include "instructions.cpp"
#include "disassembler.cpp"
#include "serialization.cpp"

auto CPU::powerR4300() -> void {
  for(uint n : range(32)) GPR[n] = 0;
  LO = 0;
  HI = 0;
  PC = 0xbfc00000;
  LLBIT = 0;
}
