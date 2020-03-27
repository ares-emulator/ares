#include <ngp/ngp.hpp>

namespace ares::NeoGeoPocket {

APU apu;
#include "memory.cpp"
#include "serialization.cpp"

auto APU::load(Node::Object parent, Node::Object from) -> void {
  node = Node::append<Node::Component>(parent, from, "APU");
  from = Node::scan(parent = node, from);

  debugInstruction = Node::append<Node::Instruction>(parent, from, "Instruction", "APU");
  debugInstruction->setAddressBits(16);

  debugInterrupt = Node::append<Node::Notification>(parent, from, "Interrupt", "APU");

  ram.allocate(4_KiB, 0x00);
  if(auto fp = platform->open(system.node, "apu.ram", File::Read)) {
    ram.load(fp);
  }
}

auto APU::save() -> void {
  if(auto fp = platform->open(system.node, "apu.ram", File::Write)) {
    ram.save(fp);
  }
}

auto APU::unload() -> void {
  ram.reset();

  node = {};
  debugInstruction = {};
  debugInterrupt = {};
}

auto APU::main() -> void {
  if(!io.enable) return step(16);

  if(nmi.line) {
    nmi.line = 0;  //edge-sensitive
    if(debugInterrupt->enabled()) debugInterrupt->notify("NMI");
    Z80::irq(0, 0x0066, 0xff);
  }

  if(irq.line) {
    //level-sensitive
    if(debugInterrupt->enabled()) debugInterrupt->notify("IRQ");
    Z80::irq(1, 0x0038, 0xff);
  }

  if(debugInstruction->enabled() && debugInstruction->address(r.pc)) {
    debugInstruction->notify(disassembleInstruction(), disassembleContext());
  }
  instruction();
}

auto APU::step(uint clocks) -> void {
  Thread::step(clocks);
  Thread::synchronize(cpu, psg);
}

auto APU::power() -> void {
  Z80::bus = this;
  Z80::power();
  bus->grant(false);
  Thread::create(system.frequency() / 2.0, {&APU::main, this});

  nmi = {};
  irq = {};
  io = {};
  io.enable = false;
}

auto APU::enable() -> void {
  Thread::destroy();
  Z80::power();
  bus->grant(false);
  Thread::create(system.frequency() / 2.0, {&APU::main, this});

  nmi = {};
  irq = {};
  io = {};
  io.enable = true;
}

auto APU::disable() -> void {
  io.enable = false;
}

}
