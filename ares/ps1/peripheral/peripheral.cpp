#include <ps1/ps1.hpp>

namespace ares::PlayStation {

Peripheral peripheral;
#include "io.cpp"
#include "serialization.cpp"
#include "port.cpp"
#include "digital-gamepad/digital-gamepad.cpp"
#include "dualshock/dualshock.cpp"
#include "memory-card/memory-card.cpp"

auto Peripheral::load(Node::Object parent) -> void {
  node = parent->append<Node::Object>("Peripheral");
}

auto Peripheral::unload() -> void {
  node.reset();
}

auto Peripheral::step(u32 clocks) -> void {
  while(clocks > 0) {
    int nextTransfer = (io.transferCounter > 0) ? io.transferCounter : (i32)INT32_MAX;
    int nextAck      = (io.transferCounter <= 0 && io.ackCounter > 0) ? io.ackCounter : (i32)INT32_MAX;

    u32 stepClocks = static_cast<u32>(std::min<int>(clocks, std::min(nextTransfer, nextAck)));

    if(io.transferCounter > 0) {
      if(stepClocks >= static_cast<u32>(io.transferCounter)) {
        stepClocks = static_cast<u32>(io.transferCounter);
        io.transferCounter = 0;
        io.receiveSize = 1;
      } else {
        io.transferCounter -= static_cast<int>(stepClocks);
      }
    }

    if(io.transferCounter <= 0 && io.ackCounter > 0) {
      if(stepClocks >= static_cast<u32>(io.ackCounter)) {
        stepClocks = static_cast<u32>(io.ackCounter);
        io.ackCounter = 0;

        if(!io.acknowledgeAsserted) {
          io.interruptRequest = 1;
          io.acknowledgeAsserted = 1;
          if(io.acknowledgeInterruptEnable) interrupt.raise(Interrupt::Peripheral);
          io.ackCounter = 96; // ACK duration
        } else {
          io.acknowledgeAsserted = 0;
        }
      } else {
        io.ackCounter -= static_cast<int>(stepClocks);
      }
    }

    clocks -= stepClocks;
  }
}

auto Peripheral::power(bool reset) -> void {
  io = {};
  io.transmitStarted = 1;
  io.transmitFinished = 1;
  io.acknowledgeAsserted = 0;
  io.baudrateReloadFactor = 1;
  io.baudrateReloadValue = 0x88;
}

}
