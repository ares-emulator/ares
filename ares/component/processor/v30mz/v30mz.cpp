#include <ares/ares.hpp>
#include "v30mz.hpp"

namespace ares {

#include "registers.cpp"
#include "modrm.cpp"
#include "memory.cpp"
#include "algorithms.cpp"
#include "instruction.cpp"
#include "instructions-adjust.cpp"
#include "instructions-alu.cpp"
#include "instructions-exec.cpp"
#include "instructions-flag.cpp"
#include "instructions-group.cpp"
#include "instructions-misc.cpp"
#include "instructions-move.cpp"
#include "instructions-string.cpp"
#include "serialization.cpp"

auto V30MZ::power() -> void {
  state.halt = 0;
  state.poll = 1;
  state.prefix = 0;
  prefixes.reset();

  r.ax = 0x0000;
  r.cx = 0x0000;
  r.dx = 0x0000;
  r.bx = 0x0000;
  r.sp = 0x2000;
  r.bp = 0x0000;
  r.si = 0x0000;
  r.di = 0x0000;
  r.es = 0x0000;
  r.cs = 0xffff;
  r.ss = 0x0000;
  r.ds = 0x0000;
  r.ip = 0x0000;
  r.f  = 0x8000;
  flush();
}

auto V30MZ::exec() -> void {
  state.poll = 1;
  state.prefix = 0;
  if(state.halt) return wait(1);

  instruction();
  if(!state.prefix) prefixes.reset();
}

#undef bits
#include "disassembler.cpp"

}
