#include <krokha/krokha.hpp>

namespace ares::Krokha {

CPU cpu;
#include "memory.cpp"
#include "debugger.cpp"
#include "serialization.cpp"

auto CPU::load(Node::Object parent) -> void {
  ram.allocate(2_KiB);

  node = parent->append<Node::Object>("CPU");

  debugger.load(node);
}

auto CPU::unload() -> void {
  ram.reset();
  node = {};
  debugger = {};
}

auto CPU::main() -> void {
  if(state.irqLine) {
    debugger.interrupt("IRQ");
    //nothing drives the data bus during the acknowledge cycle, so the CPU
    //reads 0xff off the floating bus: RST 7. the interrupt is edge-acked,
    //not level-held, so drop the line once it has been taken
    if(irq(0xff)) state.irqLine = 0;
  }

  debugger.instruction();
  instruction();
}

auto CPU::step(u32 clocks) -> void {
  Thread::step(clocks);
  Thread::synchronize();
}

auto CPU::setIRQ(bool value) -> void {
  state.irqLine = value;
}

auto CPU::power() -> void {
  I8080::bus = this;
  I8080::power();
  Thread::create(system.frequency(), std::bind_front(&CPU::main, this));

  PC = 0x0000;  //reset vector address
  state = {};
  ram.fill(0);
}

}
