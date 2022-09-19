#include <ngp/ngp.hpp>

namespace ares::NeoGeoPocket {

APU apu;
#include "memory.cpp"
#include "debugger.cpp"
#include "serialization.cpp"

auto APU::load(Node::Object parent) -> void {
  ram.allocate(4_KiB, 0x00);
  if(auto fp = system.pak->read("apu.ram")) {
    ram.load(fp);
  }

  node = parent->append<Node::Object>("APU");

  debugger.load(node);
}

auto APU::save() -> void {
  if(auto fp = system.pak->write("apu.ram")) {
    ram.save(fp);
  }
}

auto APU::unload() -> void {
  debugger.unload(node);
  ram.reset();
  node.reset();
}

auto APU::main() -> void {
  if(!io.enable) return step(16);

  if(nmi.line) {
    nmi.line = 0;  //edge-sensitive
    debugger.interrupt("NMI");
    Z80::nmi();
  }

  if(irq.line) {
    //level-sensitive
    debugger.interrupt("IRQ");
    Z80::irq();
  }

  debugger.instruction();
  instruction();
}

auto APU::step(u32 clocks) -> void {
  Thread::step(clocks);
  Thread::synchronize(cpu, psg);
}

auto APU::power() -> void {
  Z80::bus = this;
  Z80::power();
  Thread::create(system.frequency() / 2.0, {&APU::main, this});

  nmi = {};
  irq = {};
  io = {};
  io.enable = 0;
}

auto APU::enable() -> void {
  Thread::destroy();
  Z80::power();
  Thread::create(system.frequency() / 2.0, {&APU::main, this});

  nmi = {};
  irq = {};
  io = {};
  io.enable = 1;
}

auto APU::disable() -> void {
  io.enable = 0;
}

}
