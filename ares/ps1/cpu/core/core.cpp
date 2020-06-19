#include "scc-registers.cpp"
#include "memory.cpp"
#include "decoder.cpp"
#include "cpu-instructions.cpp"
#include "scc-instructions.cpp"
#include "gte-instructions.cpp"
#include "serialization.cpp"

auto CPU::raiseException(uint code, uint coprocessor) -> void {
}

auto CPU::instruction() -> void {
}

auto CPU::instructionEpilogue() -> bool {
  core.r[0] = 0;

  switch(branch.state) {
  case Branch::Step: core.pc += 4; return 0;
  case Branch::Take: core.pc += 4; branch.delaySlot(); return 0;
  case Branch::DelaySlot: core.pc = branch.pc; branch.reset(); return 1;
  case Branch::Exception: branch.reset(); return 1;
  }

  unreachable;
}

auto CPU::instructionDebug() -> void {
}

auto CPU::powerCore(bool reset) -> void {
  for(uint n : range(32)) core.r[n] = 0;
  core.lo = 0;
  core.hi = 0;
  core.pc = 0xbfc0'0000;
  branch.reset();
}
