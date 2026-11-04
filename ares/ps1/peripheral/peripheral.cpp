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

auto Peripheral::baudrateReload() const -> u32 {
  static const u8 factors[4] = {1, 1, 16, 64};
  u32 cyclesPerBit = io.baudrateReloadValue * factors[io.baudrateReloadFactor] & ~1u;
  return max(1u, cyclesPerBit >> 1);
}

auto Peripheral::stepBaudrateTimer(u32 clocks) -> u32 {
  u32 reload = baudrateReload();
  if(clocks < io.baudrateTimer) {
    io.baudrateTimer -= clocks;
    return 0;
  }
  u32 past = clocks - io.baudrateTimer;
  io.baudrateTimer = (reload - past % reload) & 0x1f'ffff;
  return 1 + past / reload;
}

auto Peripheral::pollInterrupt() -> void {
  bool request = io.transmitInterruptEnable && (io.transmitStarted || io.transmitFinished);
  request |= io.receiveInterruptEnable && io.receiveSize >= (1u << io.receiveInterruptMode);
  request |= io.acknowledgeInterruptEnable && io.acknowledgeAsserted;
  if(request) io.interruptRequest = 1;
  interrupt.drive(Interrupt::Peripheral, io.interruptRequest);
}

auto Peripheral::step(u32 clocks) -> void {
  while(clocks > 0) {
    auto pending = [&](i32 counter, bool active) {
      return active && counter > 0 ? counter : (i32)INT32_MAX;
    };
    i32 next = min((i32)clocks,
      pending((i32)io.baudrateTimer, io.transferElapses > 0),
      pending(io.ackDelayCounter, io.transferElapses == 0),
      pending(io.ackAssertCounter, true));
    u32 stepClocks = (u32)next;

    u32 elapses = stepBaudrateTimer(stepClocks);

    if(io.ackAssertCounter > 0) {
      io.ackAssertCounter -= (i32)stepClocks;
      if(io.ackAssertCounter <= 0) {
        io.ackAssertCounter = 0;
        io.acknowledgeAsserted = 0;
      }
    }

    if(io.transferElapses > 0) {
      if(elapses && !io.transmitPending) io.transmitStarted = 1;
      io.transferElapses -= min(io.transferElapses, elapses);
      if(io.transferElapses == 0 && (io.joyOutput || io.receiveEnable)) {
        u32 index = min<u32>(io.receiveSize, 7);
        for(u32 byte = index; byte < 8; byte++) {
          io.receiveData.byte(byte) = io.receiveShift;
        }
        io.receiveSize = min<u32>(io.receiveSize + 1, 8);
        io.receiveEnable = 0;
      }
      if(io.transferElapses == 0) {
        io.transmitFinished = 1;
        startTransfer();
        if(io.transferElapses) io.transmitStarted = 1;
      }
    } else if(io.ackDelayCounter > 0) {
      io.ackDelayCounter -= (i32)stepClocks;
      if(io.ackDelayCounter <= 0) {
        io.ackDelayCounter = 0;
        io.acknowledgeAsserted = 1;
        io.ackAssertCounter = ackPulseWidth;
      }
    }

    pollInterrupt();

    clocks -= stepClocks;
  }
}

auto Peripheral::nextEvent() const -> u32 {
  u32 next = ~0u;
  if(io.transferElapses) {
    next = io.baudrateTimer;
  } else if(io.ackDelayCounter > 0) {
    next = (u32)io.ackDelayCounter;
  }
  if(io.ackAssertCounter > 0) next = min(next, (u32)io.ackAssertCounter);
  return next == ~0u ? 0 : max(1u, next);
}

auto Peripheral::power(bool reset) -> void {
  io = {};
  io.transmitStarted = 1;
  io.transmitFinished = 1;
  io.acknowledgeAsserted = 0;
  io.baudrateTimer = baudrateReload();
}

}
