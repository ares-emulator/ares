#include <a52/a52.hpp>

namespace ares::Atari5200 {

CPU cpu;
#include "memory.cpp"
#include "timing.cpp"
#include "debugger.cpp"

auto CPU::load(Node::Object parent) -> void {
  node = parent->append<Node::Object>("CPU");
  debugger.load(node);
}

auto CPU::unload() -> void {
  Thread::destroy();
  debugger = {};
  node = {};
}

auto CPU::main() -> void {
  if(io.interruptPending) {
    if(io.resetPending) {
      debugger.interrupt("Reset");
      reset();
      io.resetPending = 0;
    } else if(io.nmiPending) {
      debugger.interrupt("NMI");
      interrupt();
    } else {
      debugger.interrupt("IRQ");
      interrupt();
    }
  }

  debugger.instruction();
  instruction();
}

auto CPU::step(u32 clocks) -> void {
  Thread::step(clocks);
  Thread::synchronize();
}

auto CPU::power(bool reset) -> void {
  MOS6502::BCD = 1;
  if(!reset) MOS6502::power();
  Thread::create(system.frequency() / Timing::ColorClocksPerMachineCycle, std::bind_front(&CPU::main, this));

  io = {};
  io.rdyLine = 1;
  io.resetPending = 1;
  io.interruptPending = 1;
}

}
