#include <ps1/ps1.hpp>

namespace ares::PlayStation {

Peripheral peripheral;
#include "io.cpp"
#include "serialization.cpp"

auto Peripheral::load(Node::Object parent) -> void {
  node = parent->append<Node::Component>("Peripheral");
}

auto Peripheral::unload() -> void {
  node.reset();
}

auto Peripheral::main() -> void {
  if(io.counter > 0) {
    io.counter -= 8;
    if(io.counter <= 0) {
      io.counter = 0;
      io.acknowledgeLine = 1;
      if(io.acknowledgeInterruptEnable) {
        interrupt.raise(Interrupt::Peripheral);
      }
      if(io.mode == IO::Mode::ControllerAccess) {
        io.receiveData = 0xff;
        io.receiveSize = 1;
      }
      if(io.mode == IO::Mode::ControllerIDLower) {
        io.receiveData = 0x41;
        io.receiveSize = 1;
      }
      if(io.mode == IO::Mode::ControllerIDUpper) {
        io.receiveData = 0x5a;
        io.receiveSize = 1;
      }
      if(io.mode == IO::Mode::ControllerDataLower) {
        maybe<u64> receive;
        if(io.slotNumber == 0 && controllerPort1.device) {
          receive = controllerPort1.device->read();
        }
        if(io.slotNumber == 1 && controllerPort2.device) {
          receive = controllerPort2.device->read();
        }
        io.receiveData = receive ? receive() >> 16 : 0xff;
        io.receiveSize = 1;
      }
      if(io.mode == IO::Mode::ControllerDataUpper) {
        maybe<u64> receive;
        if(io.slotNumber == 0 && controllerPort1.device) {
          receive = controllerPort1.device->read();
        }
        if(io.slotNumber == 1 && controllerPort2.device) {
          receive = controllerPort2.device->read();
        }
        io.receiveData = receive ? receive() >> 24 : 0xff;
        io.receiveSize = 1;
      }
    }
  }

  step(8);
}

auto Peripheral::step(uint clocks) -> void {
  Thread::clock += clocks;
}

auto Peripheral::power(bool reset) -> void {
  Thread::reset();
  io = {};
}

}
