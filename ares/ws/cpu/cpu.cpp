#include <ws/ws.hpp>

namespace ares::WonderSwan {

CPU cpu;
#include "io.cpp"
#include "interrupt.cpp"
#include "dma.cpp"
#include "serialization.cpp"

auto CPU::load(Node::Object parent, Node::Object from) -> void {
  node = Node::append<Node::Component>(parent, from, "CPU");
  from = Node::scan(parent = node, from);

  debugInstruction = Node::append<Node::Instruction>(parent, from, "Instruction", "CPU");
  debugInstruction->setAddressBits(20);
}

auto CPU::unload() -> void {
  debugInstruction = {};
  node = {};
}

auto CPU::main() -> void {
  poll();

  if(debugInstruction->enabled()
  && debugInstruction->address(uint20(V30MZ::r.cs * 16 + V30MZ::r.ip))
  ) {
    if(auto instruction = disassembleInstruction()) {
      debugInstruction->notify(instruction, disassembleContext());
    }
  }
  exec();
}

auto CPU::step(uint clocks) -> void {
  Thread::step(clocks);
  Thread::synchronize();
}

auto CPU::wait(uint clocks) -> void {
  step(clocks);
}

auto CPU::read(uint20 addr) -> uint8 {
  return bus.read(addr);
}

auto CPU::write(uint20 addr, uint8 data) -> void {
  return bus.write(addr, data);
}

auto CPU::in(uint16 port) -> uint8 {
  return bus.portRead(port);
}

auto CPU::out(uint16 port, uint8 data) -> void {
  return bus.portWrite(port, data);
}

auto CPU::power() -> void {
  V30MZ::power();
  Thread::create(3'072'000, {&CPU::main, this});

  bus.map(this, 0x00a0);
  bus.map(this, 0x00b0, 0x00b6);

  if(Model::WonderSwanColor() || Model::SwanCrystal()) {
    bus.map(this, 0x0040, 0x0049);
    bus.map(this, 0x0062);
  }

  r = {};

  //necessary hack until all boot ROMs have been dumped.
  if(!system.bootROM) r.cartridgeEnable = 1;
}

}
