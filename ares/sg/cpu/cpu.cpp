#include <sg/sg.hpp>

namespace ares::SG1000 {

CPU cpu;
#include "memory.cpp"
#include "debugger.cpp"
#include "serialization.cpp"

auto CPU::load(Node::Object parent) -> void {
  if(Model::SG1000()) ram.allocate(1_KiB);
  if(Model::SC3000()) ram.allocate(2_KiB);
  if(Model::SG1000A()) ram.allocate(1_KiB);

  node = parent->append<Node::Object>("CPU");

  debugger.load(node);
}

auto CPU::unload() -> void {
  ram.reset();
  debugger = {};
  node = {};
}

auto CPU::main() -> void {
  if(state.nmiLine) {
    state.nmiLine = 0;  //edge-sensitive
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
  state.nmiLine = value;
}

auto CPU::setIRQ(bool value) -> void {
  state.irqLine = value;
}

auto CPU::power() -> void {
  Z80::bus = this;
  Z80::power();
  Thread::create(system.colorburst(), std::bind_front(&CPU::main, this));
  PC = 0x0000;  //reset vector address
  state = {};
}

}
