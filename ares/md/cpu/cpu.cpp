#include <md/md.hpp>

namespace ares::MegaDrive {

CPU cpu;
#include "bus.cpp"
#include "io.cpp"
#include "debugger.cpp"
#include "serialization.cpp"

auto CPU::load(Node::Object parent) -> void {
  node = parent->append<Node::Object>("CPU");
  tmss.allocate(2_KiB >> 1);
  ram.allocate(64_KiB >> 1);
  debugger.load(node);

  if(auto fp = system.pak->read("tmss.rom")) {
    for(auto address : range(tmss.size())) tmss.program(address, fp->readm(2L));
  }
}

auto CPU::unload() -> void {
  debugger = {};
  tmss.reset();
  ram.reset();
  node.reset();
}

auto CPU::main() -> void {
  if(state.interruptPending) {
    if(lower(Interrupt::Reset)) {
      r.a[7] = read(1, 1, 0) << 16 | read(1, 1, 2) << 0;
      r.pc   = read(1, 1, 4) << 16 | read(1, 1, 6) << 0;
      prefetch();
      prefetch();
      debugger.interrupt("Reset");
    }

    if(6 > r.i && lower(Interrupt::VerticalBlank)) {
      debugger.interrupt("Vblank");
      vdp.irq.acknowledge(6);
      return interrupt(Vector::Level6, 6);
    }

    if(4 > r.i && lower(Interrupt::HorizontalBlank)) {
      debugger.interrupt("Hblank");
      vdp.irq.acknowledge(4);
      return interrupt(Vector::Level4, 4);
    }

    if(2 > r.i && lower(Interrupt::External)) {
      debugger.interrupt("External");
      vdp.irq.acknowledge(2);
      return interrupt(Vector::Level2, 2);
    }
  }

  debugger.instruction();
  instruction();
}

auto CPU::step(u32 clocks) -> void {
  refresh.ram += clocks;
  while(refresh.ram >= 133) refresh.ram -= 133;
  refresh.external += clocks;
  Thread::step(clocks);
  cyclesUntilSync -= clocks;
}

inline auto CPU::idle(u32 clocks) -> void {
  step(clocks);
}

auto CPU::wait(u32 clocks) -> void {
  step(clocks);
  if (cyclesUntilSync <= 0) {
    Thread::synchronize();
    cyclesUntilSync += minCyclesBetweenSyncs;
  }
}

auto CPU::raise(Interrupt interrupt) -> void {
  state.interruptPending.bit((u32)interrupt) = 1;
}

auto CPU::lower(Interrupt interrupt) -> bool {
  if(!state.interruptPending.bit((u32)interrupt)) return false;
  return state.interruptPending.bit((u32)interrupt) = 0, true;
}

auto CPU::power(bool reset) -> void {
  M68000::power();
  Thread::create(system.frequency() / 7.0, {&CPU::main, this});

  tmssEnable = system.tmss->value();
  if(!reset) ram.fill();

  io = {};
  io.version = tmssEnable;
  io.romEnable = !tmssEnable;
  io.vdpEnable[0] = !tmssEnable;
  io.vdpEnable[1] = !tmssEnable;

  refresh = {};

  state = {};
  raise(Interrupt::Reset);
}

}
