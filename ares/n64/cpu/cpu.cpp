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
//print(hex(r.pc, 8L), "  ", disassembleInstruction(), "\n");
  instruction();
  step(20);
}

auto CPU::step(uint clocks) -> void {
  Thread::step(clocks);
  Thread::synchronize();
}

auto CPU::power() -> void {
  Thread::create(93'750'000, {&CPU::main, this});
  powerR4300();

  //PIF ROM simulation:
  r.gpr[20] = 0x0000'0001;
  r.gpr[22] = 0x0000'003f;
  r.gpr[29] = 0xa400'1ff0;
  r.pc      = 0xa400'0040;

  for(uint offset : range(0x1000)) {
    auto data = bus.readByte(0xb000'0000 + offset);
    bus.writeByte(0xa400'0000 + offset, data);
  }
}

}
