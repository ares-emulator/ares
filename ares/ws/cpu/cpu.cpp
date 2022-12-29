#include <ws/ws.hpp>

namespace ares::WonderSwan {

CPU cpu;
#include "io.cpp"
#include "keypad.cpp"
#include "interrupt.cpp"
#include "dma.cpp"
#include "debugger.cpp"
#include "serialization.cpp"

auto CPU::load(Node::Object parent) -> void {
  node = parent->append<Node::Object>("CPU");
  debugger.load(node);
}

auto CPU::unload() -> void {
  debugger.unload(node);
  node.reset();
}

auto CPU::main() -> void {
  poll();
  debugger.instruction();
  instruction();
}

auto CPU::step(u32 clocks) -> void {
  Thread::step(clocks);
  Thread::synchronize();
}

auto CPU::width(n20 address) -> u32 {
  switch(address >> 16) {
    case 0: return Word; // internal RAM
    case 1: return Byte; // SRAM
    default: return io.cartridgeRomWidth ? Word : Byte; // cartridge ROM
  }
}

auto CPU::speed(n20 address) -> n32 {
  switch(address >> 16) {
    case 0: return 1; // internal RAM
    case 1: return system.color() ? 1 : 3; // SRAM
    default: return io.cartridgeRomWait ? 3 : 1; // cartridge ROM
  }
}

auto CPU::read(n20 address) -> n8 {
  return bus.read(address);
}

auto CPU::write(n20 address, n8 data) -> void {
  return bus.write(address, data);
}

auto CPU::in(n16 port) -> n8 {
  return bus.readIO(port & 0x01ff);
}

auto CPU::out(n16 port, n8 data) -> void {
  return bus.writeIO(port & 0x01ff, data);
}

auto CPU::power() -> void {
  V30MZ::power();
  Thread::create(3'072'000, {&CPU::main, this});

  bus.map(this, 0x00a0);
  bus.map(this, 0x00b0);
  bus.map(this, 0x00b2);
  bus.map(this, 0x00b4, 0x00b7);

  if(Model::WonderSwanColor() || Model::SwanCrystal()) {
    bus.map(this, 0x0040, 0x0049);
    bus.map(this, 0x0062);
  }

  io = {};
  keypad.power();
}

}
