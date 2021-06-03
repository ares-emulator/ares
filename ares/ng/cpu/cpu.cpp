#include <ng/ng.hpp>

namespace ares::NeoGeo {

CPU cpu;
#include "memory.cpp"
#include "debugger.cpp"
#include "serialization.cpp"

auto CPU::load(Node::Object parent) -> void {
  node = parent->append<Node::Object>("CPU");
  debugger.load(node);
}

auto CPU::unload() -> void {
  debugger = {};
  node.reset();
}

auto CPU::main() -> void {
  if(io.interruptPending) {
    if(lower(Interrupt::Reset)) {
      r.a[7] = read(1, 1, 0) << 16 | read(1, 1, 2) << 0;
      r.pc   = read(1, 1, 4) << 16 | read(1, 1, 6) << 0;
      prefetch();
      prefetch();
      debugger.interrupt("Reset");
    }

    if(3 > r.i && lower(Interrupt::Power)) {
      debugger.interrupt("Power");
      return interrupt(Vector::Level3, 3);
    }

    if(2 > r.i && lower(Interrupt::Timer)) {
      debugger.interrupt("Timer");
      return interrupt(Vector::Level2, 2);
    }

    if(1 > r.i && lower(Interrupt::Vblank)) {
      debugger.interrupt("Vblank");
      return interrupt(Vector::Level1, 1);
    }
  }

  debugger.instruction();
  instruction();
}

auto CPU::idle(u32 clocks) -> void {
  Thread::step(clocks);
}

auto CPU::wait(u32 clocks) -> void {
  Thread::step(clocks);
  Thread::synchronize();
}

auto CPU::raise(Interrupt interrupt) -> void {
  io.interruptPending.bit((u32)interrupt) = 1;
}

auto CPU::lower(Interrupt interrupt) -> bool {
  if(!io.interruptPending.bit((u32)interrupt)) return false;
  return io.interruptPending.bit((u32)interrupt) = 0, true;
}

auto CPU::power(bool reset) -> void {
  M68000::power();
  Thread::create(12'000'000, {&CPU::main, this});
  io = {};
  raise(Interrupt::Reset);
}

}
