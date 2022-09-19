#include <cv/cv.hpp>

namespace ares::ColecoVision {

CPU cpu;
#include "memory.cpp"
#include "debugger.cpp"
#include "serialization.cpp"

auto CPU::load(Node::Object parent) -> void {
  ram.allocate(0x0400);
  expansion.allocate(0x1000);

  node = parent->append<Node::Object>("CPU");

  debugger.load(node);
}

auto CPU::unload() -> void {
  ram.reset();
  expansion.reset();
  node = {};
  debugger = {};
}

auto CPU::main() -> void {
  if(state.nmiPending) {
    state.nmiPending = 0;  //edge-sensitive
    debugger.interrupt("NMI");
    nmi();
  }

  if(state.irqLine) {
    //level-sensitive
    debugger.interrupt("IRQ");
    irq();
  }

  debugger.instruction();
  instruction();
}

auto CPU::step(uint clocks) -> void {
  Thread::step(clocks);
  Thread::synchronize();
}

auto CPU::setNMI(bool value) -> void {
  if(!state.nmiLine && value) state.nmiPending = 1;
  state.nmiLine = value;
}

auto CPU::setIRQ(bool value) -> void {
  state.irqLine = value;
}

auto CPU::power() -> void {
  Z80::bus = this;
  Z80::power();
  Thread::create(system.colorburst(), {&CPU::main, this});

  PC = 0x0000;  //reset vector address
  state = {};
  io = {};
  ram.fill(0);
}

}
