#include <n64/n64.hpp>

namespace ares::Nintendo64 {

CPU cpu;
#include "core/core.cpp"
#include "serialization.cpp"

auto CPU::load(Node::Object parent) -> void {
  node = parent->append<Node::Component>("CPU");
}

auto CPU::unload() -> void {
  node = {};
}

auto CPU::main() -> void {
  for(uint index : range(2048)) {
    instruction();
    core.r[0].u64 = 0;
  }
  step(2048);
}

auto CPU::step(uint clocks) -> void {
  Thread::step(clocks);
  Thread::synchronize();
}

auto CPU::power() -> void {
  Thread::create(93'750'000, {&CPU::main, this});
  powerR4300();

  //PIF ROM simulation:
  core.r[20].u64 = 0x0000'0001;
  core.r[22].u64 = 0x0000'003f;
  core.r[29].u64 = 0xa400'1ff0;
  core.pc        = 0xa400'0040;

  for(uint offset = 0; offset < 0x1000; offset += 4) {
    auto data = bus.readWord(0xb000'0000 + offset);
    bus.writeWord(0xa400'0000 + offset, data);
  }
}

}
