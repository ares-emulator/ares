#include "registers.hpp"
#include "scc-tlb.cpp"
#include "scc-registers.cpp"
#include "fpu-registers.cpp"
#include "memory.cpp"
#include "instruction.cpp"
#include "cpu-instructions.cpp"
#include "scc-instructions.cpp"
#include "fpu-instructions.cpp"
#include "disassembler.cpp"
#include "serialization.cpp"

auto CPU::powerR4300(bool reset) -> void {
  for(uint n : range(32)) GPR[n].u64 = 0;
  scc = {};
  scc.status.softReset = reset;
  fpu = {};
  LO.u64 = 0;
  HI.u64 = 0;
  GPR[Core::Register::SP].u64 = i32(0xa400'1ff0);
  PC = i32(0xbfc00000);
  branch.reset();
  fesetround(FE_TONEAREST);
  context.setMode();
}

auto CPU::Context::setMode() -> void {
  mode = min(2, self.scc.status.privilegeMode);
  if(self.scc.status.exceptionLevel) mode = Mode::Kernel;
  if(self.scc.status.errorLevel) mode = Mode::Kernel;

  segment[0] = Segment::Mapped;
  segment[1] = Segment::Mapped;
  segment[2] = Segment::Mapped;
  segment[3] = Segment::Mapped;
  if(mode == Mode::Kernel) {
    segment[4] = Segment::Cached;
    segment[5] = Segment::Uncached;
    segment[6] = Segment::Mapped;
    segment[7] = Segment::Mapped;
  }
  if(mode == Mode::Supervisor) {
    segment[4] = Segment::Invalid;
    segment[5] = Segment::Invalid;
    segment[6] = Segment::Mapped;
    segment[7] = Segment::Invalid;
  }
  if(mode == Mode::User) {
    segment[4] = Segment::Invalid;
    segment[5] = Segment::Invalid;
    segment[6] = Segment::Invalid;
    segment[7] = Segment::Invalid;
  }
}
