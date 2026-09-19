#include <gamate/gamate.hpp>

namespace ares::Gamate {

CPU cpu;
#include "memory.cpp"
#include "debugger.cpp"
#include "serialization.cpp"

auto CPU::load(Node::Object parent) -> void {
  ram.allocate(0x400);

  node = parent->append<Node::Object>("CPU");

  debugger.load(node);
}

auto CPU::unload() -> void {
  ram.reset();
  node = {};
  debugger = {};
}

auto CPU::main() -> void {
  if(state.interruptPending) {
    if(state.resetPending) {
      state.resetPending = 0;
      debugger.interrupt("Reset");
      return reset();
    }
    debugger.interrupt("IRQ");
    return interrupt();
  }

  debugger.instruction();
  instruction();
}

auto CPU::step(u32 clocks) -> void {
  for(u32 n : range(clocks)) {
    if(state.release && !--state.release) setIRQ(0);
    if(!--state.timer) {
      state.timer = TimerPeriod;
      state.release = TimerAssert;
      setIRQ(1);
    }
  }

  Thread::step(clocks);
  Thread::synchronize();
}

auto CPU::setIRQ(bool value) -> void {
  state.irqLine = value;
}

auto CPU::power() -> void {
  MOS6502::BCD = 1;
  MOS6502::power();
  Thread::create(CpuClock, std::bind_front(&CPU::main, this));

  state = {};
  state.resetPending = 1;
  state.interruptPending = 1;
  state.timer = TimerPeriod;

  //the RAM reads back as 0xff on a cold machine
  ram.fill(0xff);
}

}
