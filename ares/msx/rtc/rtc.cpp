#include <msx/msx.hpp>

namespace ares::MSX {
#include "serialization.cpp"

RTC rtc;

auto RTC::load(Node::Object parent) -> void {
  node = parent->append<Node::Object>("RTC");
  sram.allocate(26);
}

auto RTC::save() -> void {

}

auto RTC::unload() -> void {
  node.reset();
}

auto RTC::main() -> void {
  step(1);
}

auto RTC::step(u32 clocks) -> void {
  Thread::step(clocks);
  Thread::synchronize(cpu);
}

auto RTC::power() -> void {
  io = {};
  Thread::create(16, std::bind_front(&RTC::main, this));
}

auto RTC::read() -> n4 {
  n4 data = 0;
  if(io.registerIndex == 13) {
    data.bit(0, 1) = io.registerBank;
    data.bit(2)    = io.timerEnable;
    return data;
  }

  if(io.registerBank == 2 || io.registerBank == 3) {
    auto base = io.registerBank == 2 ? 0 : 13;
    return sram[base + io.registerIndex];
  }

  debug(unimplemented, "[RTC::read] bank: ", io.registerBank, " index: ", io.registerIndex);
  return data;
}

auto RTC::write(n4 data) -> void {
  if(io.registerIndex == 13) {
    io.registerBank = data.bit(0,1);
    io.timerEnable = data.bit(2);
    return;
  }

  if(io.registerBank == 2 || io.registerBank == 3) {
    auto base = io.registerBank == 2 ? 0 : 13;
    sram[base + io.registerIndex] = data;
    return;
  }

  debug(unimplemented, "[RTC::write] bank: ", io.registerBank, " index: ", io.registerIndex, " = ", data);
}

}
