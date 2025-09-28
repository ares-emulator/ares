#include <sfc/sfc.hpp>

namespace ares::SuperFamicom {

SMP smp;
#include "memory.cpp"
#include "io.cpp"
#include "timing.cpp"
#include "debugger.cpp"
#include "serialization.cpp"

auto SMP::load(Node::Object parent) -> void {
  node = parent->append<Node::Object>("SMP");

  debugger.load(node);
}

auto SMP::unload() -> void {
  debugger = {};
  node = {};
}

auto SMP::main() -> void {
  if(r.wait) return instructionWait();
  if(r.stop) return instructionStop();

  debugger.instruction();
  instruction();
}

auto SMP::power(bool reset) -> void {
  if(auto fp = system.pak->read("ipl.rom")) {
    fp->read(iplrom, 64);
  }

  SPC700::power();
  create(system.apuFrequency() / 12.0, std::bind_front(&SMP::main, this));

  r.pc.byte.l = iplrom[62];
  r.pc.byte.h = iplrom[63];

  io = {};
  timer0 = {};
  timer1 = {};
  timer2 = {};
}

}
