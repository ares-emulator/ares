#include <ps1/ps1.hpp>

namespace ares::PlayStation {

Peripheral peripheral;
#include "io.cpp"
#include "serialization.cpp"
#include "port.cpp"
#include "digital-gamepad/digital-gamepad.cpp"
#include "memory-card/memory-card.cpp"

auto Peripheral::load(Node::Object parent) -> void {
  node = parent->append<Node::Object>("Peripheral");
}

auto Peripheral::unload() -> void {
  node.reset();
}

auto Peripheral::main() -> void {
  if(io.counter > 0) {
    io.counter -= 8;
    if(io.counter <= 0) {
      io.counter = 0;
      io.interruptRequest = 1;
      if(io.acknowledgeInterruptEnable) {
        interrupt.raise(Interrupt::Peripheral);
      }
    }
  }

  step(8);
}

auto Peripheral::step(u32 clocks) -> void {
  Thread::clock += clocks;
}

auto Peripheral::power(bool reset) -> void {
  Thread::reset();
  Memory::Interface::setWaitStates(2, 2, 2);
  io = {};
}

}
