#include <msx/msx.hpp>

namespace ares::MSX {

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
  node = {};
  debugInstruction = {};
  debugInterrupt = {};
}

auto CPU::main() -> void {
  if(io.irqLine) {
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

auto CPU::power() -> void {
  Z80::bus = this;
  Z80::power();
  Thread::create(system.colorburst(), {&CPU::main, this});

  r.pc = 0x0000;  //reset vector address

  if(Model::MSX() ) ram.allocate (64_KiB);
  if(Model::MSX2()) ram.allocate(256_KiB);

  slot[0] = {3, 0, {0, 0, 0, 0}};
  slot[1] = {2, 1, {0, 0, 0, 0}};
  slot[2] = {1, 2, {0, 0, 0, 0}};
  slot[3] = {0, 3, {0, 0, 0, 0}};

  io = {};
}

auto CPU::setIRQ(bool line) -> void {
  io.irqLine = line;
}

}
