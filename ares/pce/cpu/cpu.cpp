#include <pce/pce.hpp>

namespace ares::PCEngine {

CPU cpu;
#include "io.cpp"
#include "debugger.cpp"
#include "serialization.cpp"

auto CPU::load(Node::Object parent) -> void {
  node = parent->append<Node::Component>("CPU");

  if(Model::SuperGrafx() == 0) ram.allocate( 8_KiB, 0x00);
  if(Model::SuperGrafx() == 1) ram.allocate(32_KiB, 0x00);

  debugger.load(node);
}

auto CPU::unload() -> void {
  ram.reset();

  node = {};
  debugger = {};
}

auto CPU::main() -> void {
  if(tiq.pending) {
    debugger.interrupt("TIQ");
    return interrupt(tiq.vector);
  }

  if(irq1.pending) {
    debugger.interrupt("IRQ1");
    return interrupt(irq1.vector);
  }

  if(irq2.pending) {
    debugger.interrupt("IRQ2");
    return interrupt(irq2.vector);
  }

  debugger.instruction();
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

  r.pc.byte(0) = read(r.mpr[reset.vector >> 13], uint13(reset.vector + 0));
  r.pc.byte(1) = read(r.mpr[reset.vector >> 13], uint13(reset.vector + 1));

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
