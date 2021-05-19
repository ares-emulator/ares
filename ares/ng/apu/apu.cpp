#include <ng/ng.hpp>

namespace ares::NeoGeo {

APU apu;
#include "memory.cpp"
#include "debugger.cpp"
#include "serialization.cpp"

auto APU::load(Node::Object parent) -> void {
  node = parent->append<Node::Object>("APU");
  ram.allocate(2_KiB);
  debugger.load(node);
}

auto APU::unload() -> void {
  debugger.unload(node);
  ram.reset();
  node.reset();
}

auto APU::main() -> void {
  if(io.nmiLine && io.nmiEnable) {
    io.nmiLine = 0;  //edge-sensitive
    debugger.interrupt("NMI");
    irq(0, 0x0066, 0xff);
  }

  if(io.irqLine) {
    //level-sensitive
    debugger.interrupt("IRQ");
    irq(1, 0x0038, 0xff);
  }

  debugger.instruction();
  instruction();
}

auto APU::step(u32 clocks) -> void {
  Thread::step(clocks);
  Thread::synchronize();
}

auto APU::power(bool reset) -> void {
  Z80::bus = this;
  Z80::power();
  Thread::create(4'000'000, {&APU::main, this});
  communication = {};
  io = {};
}

}
