#include <md/md.hpp>

namespace ares::MegaDrive {

APU apu;
#include "bus.cpp"
#include "serialization.cpp"

auto APU::load(Node::Object parent, Node::Object from) -> void {
  node = Node::append<Node::Component>(parent, from, "APU");
  from = Node::scan(parent = node, from);

  debugInstruction = Node::append<Node::Instruction>(parent, from, "Instruction", "APU");
  debugInstruction->setAddressBits(16);

  debugInterrupt = Node::append<Node::Notification>(parent, from, "Interrupt", "APU");
}

auto APU::unload() -> void {
  debugInstruction = {};
  debugInterrupt = {};
  node = {};
}

auto APU::main() -> void {
  if(!state.enabled) {
    return step(1);
  }

  if(state.nmiLine) {
    state.nmiLine = 0;  //edge-sensitive
    if(debugInterrupt->enabled()) debugInterrupt->notify("NMI");
    irq(0, 0x0066, 0xff);
  }

  if(state.intLine) {
    //level-sensitive
    if(debugInterrupt->enabled()) debugInterrupt->notify("IRQ");
    irq(1, 0x0038, 0xff);
  }

  if(debugInstruction->enabled() && debugInstruction->address(r.pc)) {
    debugInstruction->notify(disassembleInstruction(), disassembleContext());
  }
  instruction();
}

auto APU::step(uint clocks) -> void {
  Thread::step(clocks);
  Thread::synchronize(cpu, vdp, psg, ym2612);
}

auto APU::setNMI(bool value) -> void {
  state.nmiLine = value;
}

auto APU::setINT(bool value) -> void {
  state.intLine = value;
}

auto APU::enable(bool value) -> void {
  //68K cannot disable the Z80 without bus access
  if(!bus->granted() && !value) return;
  if(state.enabled && !value) reset();
  state.enabled = value;
}

auto APU::power(bool reset) -> void {
  Z80::bus = this;
  Z80::power();
  bus->grant(false);
  Thread::create(system.frequency() / 15.0, {&APU::main, this});
  if(!reset) ram.allocate(8_KiB);
  state = {};
}

auto APU::reset() -> void {
  Z80::power();
  bus->grant(false);
  Thread::create(system.frequency() / 15.0, {&APU::main, this});
  state = {};
}

}
