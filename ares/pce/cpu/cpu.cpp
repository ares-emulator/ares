#include <pce/pce.hpp>

namespace ares::PCEngine {

CPU cpu;
#include "io.cpp"
#include "serialization.cpp"

auto CPU::load(Node::Object parent, Node::Object from) -> void {
  node = Node::append<Node::Component>(parent, from, "CPU");
  from = Node::scan(parent = node, from);

  eventInstruction = Node::append<Node::Instruction>(parent, from, "Instruction", "CPU");
  eventInstruction->setAddressBits(24);

  eventInterrupt = Node::append<Node::Notification>(parent, from, "Interrupt", "CPU");

  if(Model::PCEngine())   ram.allocate( 8_KiB, 0x00);
  if(Model::SuperGrafx()) ram.allocate(32_KiB, 0x00);
}

auto CPU::unload() -> void {
  ram.reset();

  node = {};
  eventInstruction = {};
  eventInterrupt = {};
}

auto CPU::main() -> void {
  if(tiq.pending) {
    if(eventInterrupt->enabled()) eventInterrupt->notify("TIQ");
    return interrupt(tiq.vector);
  }

  if(irq1.pending) {
    if(eventInterrupt->enabled()) eventInterrupt->notify("IRQ1");
    return interrupt(irq1.vector);
  }

  if(irq2.pending) {
    if(eventInterrupt->enabled()) eventInterrupt->notify("IRQ2");
    return interrupt(irq2.vector);
  }

  if(eventInstruction->enabled()) {
    auto bank = r.mpr[r.pc >> 13];
    auto address = (uint13)r.pc;
    if(eventInstruction->address(bank << 16 | address)) {
      eventInstruction->notify(disassembleInstruction(), disassembleContext(), {
        "V:", pad(vdp.io.vcounter, 3L), " ", "H:", pad(vdp.io.hcounter, 4L)
      });
    }
  }

  instruction();
}

auto CPU::step(uint clocks) -> void {
  timer.counter -= clocks;
  while(timer.counter < 0) {
    synchronize(psg);
    timer.counter += 1024 * 3;
    if(!timer.value--) {
      timer.value = timer.reload;
      timer.line = timer.enable;
    }
  }

  Thread::step(clocks);
  synchronize(vdp);
  if(PCD::Present()) synchronize(pcd);
}

auto CPU::power() -> void {
  HuC6280::power();
  Thread::create(system.colorburst() * 6.0, {&CPU::main, this});

  r.pc.byte(0) = cartridge.read(r.mpr[reset.vector >> 13], uint13(reset.vector + 0));
  r.pc.byte(1) = cartridge.read(r.mpr[reset.vector >> 13], uint13(reset.vector + 1));

  ram.fill(0x00);

  irq2 = {};
  irq1 = {};
  tiq = {};
  timer = {};
  io = {};
}

auto CPU::lastCycle() -> void {
  irq2.pending = 0 & !irq2.disable & !r.p.i;
  irq1.pending = vdp.irqLine() & !irq1.disable & !r.p.i;
  tiq.pending = timer.irqLine() & !tiq.disable & !r.p.i;
}

}
