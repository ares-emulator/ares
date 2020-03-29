#include <cv/cv.hpp>

namespace ares::ColecoVision {

CPU cpu;
#include "memory.cpp"
#include "debugger.cpp"
#include "serialization.cpp"

auto CPU::load(Node::Object parent, Node::Object from) -> void {
  node = Node::append<Node::Component>(parent, from, "CPU");
  from = Node::scan(parent = node, from);

  debugger.load(parent, from);
}

auto CPU::unload() -> void {
  debugger = {};
  node = {};
}

auto CPU::main() -> void {
  if(state.nmiLine) {
    state.nmiLine = 0;  //edge-sensitive
    debugger.interrupt("NMI");
    irq(0, 0x0066, 0xff);
  }

  if(state.irqLine) {
    //level-sensitive
    debugger.interrupt("IRQ");
    irq(1, 0x0038, 0xff);
  }

  debugger.instruction();
  instruction();
}

auto CPU::step(uint clocks) -> void {
  Thread::step(clocks);
  Thread::synchronize();
}

auto CPU::setNMI(bool value) -> void {
  state.nmiLine = value;
}

auto CPU::setIRQ(bool value) -> void {
  state.irqLine = value;
}

auto CPU::power() -> void {
  Z80::bus = this;
  Z80::power();
  Thread::create(system.colorburst(), {&CPU::main, this});

  if(Model::ColecoVision()) ram.allocate(0x0400), expansion.allocate(0x1000);
  if(Model::ColecoAdam())   ram.allocate(0x0400), expansion.allocate(0x1000);

  r.pc = 0x0000;  //reset vector address
  state = {};
  io = {};
}

}
