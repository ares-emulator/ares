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
  if(nmi.pending && nmi.enable) {
    Z80::nmi();
    nmi.pending = 0;
    debugger.interrupt("NMI");
  }

  if(irq.pending) {
    Z80::irq();
    irq.pending = 0;
    debugger.interrupt("IRQ");
  }

  debugger.instruction();
  Z80::instruction();
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
  nmi = {};
  irq = {};
  rom = {};
}

}
