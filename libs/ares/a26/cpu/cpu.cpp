#include <a26/a26.hpp>

namespace ares::Atari2600 {

CPU cpu;
#include "memory.cpp"
#include "timing.cpp"
#include "debugger.cpp"
#include "serialization.cpp"

auto CPU::load(Node::Object parent) -> void {
  node = parent->append<Node::Object>("CPU");
  debugger.load(node);
}

auto CPU::unload() -> void {
  debugger = {};
  node = {};
}

auto CPU::main() -> void {
  debugger.instruction();
  instruction();
}

auto CPU::step(u32 clocks) -> void {
  if(io.rdyLine == 1) io.scanlineCycles += clocks;
  Thread::step(clocks);
  Thread::synchronize();
}

auto CPU::power(bool reset) -> void {
  MOS6502::BCD = 1;
  MOS6502::power();
  Thread::create(system.frequency() / 3, std::bind_front(&CPU::main, this));

  io = {};
}

}
