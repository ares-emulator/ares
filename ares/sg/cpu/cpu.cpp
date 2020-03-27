#include <sg/sg.hpp>

namespace ares::SG1000 {

CPU cpu;
#include "memory.cpp"
#include "serialization.cpp"

auto CPU::load(Node::Object parent, Node::Object from) -> void {
  node = Node::append<Node::Component>(parent, from, "CPU");
  from = Node::scan(parent = node, from);

  debugInstruction = Node::append<Node::Instruction>(parent, from, "Instruction", "CPU");
  debugInstruction->setAddressBits(16);

  debugInterrupt = Node::append<Node::Notification>(parent, from, "Interrupt", "CPU");
}

auto CPU::unload() -> void {
  debugInstruction = {};
  debugInterrupt = {};
  node = {};
}

auto CPU::main() -> void {
  if(state.nmiLine) {
    state.nmiLine = 0;  //edge-sensitive
    if(debugInterrupt->enabled()) debugInterrupt->notify("NMI");
    irq(0, 0x0066, 0xff);
  }

  if(state.irqLine) {
    //level-sensitive
    if(debugInterrupt->enabled()) debugInterrupt->notify("IRQ");
    irq(1, 0x0038, 0xff);
  }

  if(debugInstruction->enabled() && debugInstruction->address(r.pc)) {
    debugInstruction->notify(disassembleInstruction(), disassembleContext());
  }
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
  if(Model::SG1000()) ram.allocate(0x0400);
  if(Model::SC3000()) ram.allocate(0x0800);
  r.pc = 0x0000;  //reset vector address
  state = {};
}

}
