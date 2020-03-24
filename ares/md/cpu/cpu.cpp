#include <md/md.hpp>

namespace ares::MegaDrive {

CPU cpu;
#include "bus.cpp"
#include "io.cpp"
#include "serialization.cpp"

auto CPU::load(Node::Object parent, Node::Object from) -> void {
  node = Node::append<Node::Component>(parent, from, "CPU");
  from = Node::scan(parent = node, from);

  eventInstruction = Node::append<Node::Instruction>(parent, from, "Instruction", "CPU");
  eventInstruction->setAddressBits(24);

  eventInterrupt = Node::append<Node::Notification>(parent, from, "Interrupt", "CPU");
}

auto CPU::unload() -> void {
  eventInstruction = {};
  eventInterrupt = {};
  node = {};
}

auto CPU::main() -> void {
  if(state.interruptPending) {
    if(state.interruptPending.bit((uint)Interrupt::Reset)) {
      state.interruptPending.bit((uint)Interrupt::Reset) = 0;
      r.a[7] = read(1, 1, 0) << 16 | read(1, 1, 2) << 0;
      r.pc   = read(1, 1, 4) << 16 | read(1, 1, 6) << 0;
      prefetch();
      prefetch();
      if(eventInterrupt->enabled()) eventInterrupt->notify("Reset");
    }

    if(state.interruptPending.bit((uint)Interrupt::HorizontalBlank)) {
      if(4 > r.i) {
        state.interruptPending.bit((uint)Interrupt::HorizontalBlank) = 0;
        if(eventInterrupt->enabled()) eventInterrupt->notify("Hblank");
        return interrupt(Vector::Level4, 4);
      }
    }

    if(state.interruptPending.bit((uint)Interrupt::VerticalBlank)) {
      if(6 > r.i) {
        state.interruptPending.bit((uint)Interrupt::VerticalBlank) = 0;
        if(eventInterrupt->enabled()) eventInterrupt->notify("Vblank");
        return interrupt(Vector::Level6, 6);
      }
    }
  }

  if(eventInstruction->enabled() && eventInstruction->address(r.pc - 4)) {
    eventInstruction->notify(disassembleInstruction(r.pc - 4), disassembleContext());
  }
  instruction();
}

auto CPU::step(uint clocks) -> void {
  refresh.ram += clocks;
  while(refresh.ram >= 133) refresh.ram -= 133;
  refresh.external += clocks;
  Thread::step(clocks);
}

auto CPU::idle(uint clocks) -> void {
  step(clocks);
}

auto CPU::wait(uint clocks) -> void {
  while(vdp.dma.active) {
    Thread::step(1);
    Thread::synchronize(vdp);
  }

  step(clocks);
  Thread::synchronize();
}

auto CPU::raise(Interrupt interrupt) -> void {
  if(!state.interruptLine.bit((uint)interrupt)) {
    state.interruptLine.bit((uint)interrupt) = 1;
    state.interruptPending.bit((uint)interrupt) = 1;
  }
}

auto CPU::lower(Interrupt interrupt) -> void {
  state.interruptLine.bit((uint)interrupt) = 0;
  state.interruptPending.bit((uint)interrupt) = 0;
}

auto CPU::power(bool reset) -> void {
  M68K::power();
  Thread::create(system.frequency() / 7.0, {&CPU::main, this});

  ram.allocate(64_KiB >> 1);

  tmssEnable = false;
  if(system.tmss->value()) {
    tmss.allocate(2_KiB >> 1);
    if(auto fp = platform->open(system.node, "tmss.rom", File::Read, File::Required)) {
      for(uint address : range(tmss.size())) tmss.program(address, fp->readm(2));
      tmssEnable = true;
    }
  }

  if(!reset) memory::fill(ram.data(), sizeof(ram));

  io = {};
  io.version = tmssEnable;
  io.romEnable = !tmssEnable;
  io.vdpEnable[0] = !tmssEnable;
  io.vdpEnable[1] = !tmssEnable;

  refresh = {};

  state = {};
  state.interruptPending.bit((uint)Interrupt::Reset) = 1;
}

}
