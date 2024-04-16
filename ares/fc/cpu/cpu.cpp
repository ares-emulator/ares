#include <fc/fc.hpp>

namespace ares::Famicom {

CPU cpu;
#include "memory.cpp"
#include "timing.cpp"
#include "debugger.cpp"
#include "serialization.cpp"

auto CPU::load(Node::Object parent) -> void {
  ram.allocate(2_KiB);

  node = parent->append<Node::Object>("CPU");

  debugger.load(node);
}

auto CPU::unload() -> void {
  ram.reset();
  debugger = {};
  node = {};
}

auto CPU::main() -> void {
  debugger.instruction();
  Ricoh2A03::main();
}

auto CPU::step(u32 clocks) -> void {
  assert(clocks == rate());
  Thread::step(clocks);
  Thread::synchronize();
}

auto CPU::power(bool reset) -> void {
  Ricoh2A03::power(reset);
  Thread::create(system.frequency(), {&CPU::main, this});

  if(!reset) {
    ram.fill(0xff);
  }

  io = {};
}

}
