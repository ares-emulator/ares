#include <ms/ms.hpp>

namespace ares::MasterSystem {

CPU cpu;
#include "memory.cpp"
#include "debugger.cpp"
#include "serialization.cpp"

auto CPU::load(Node::Object parent) -> void {
  ram.allocate(8_KiB);

  node = parent->append<Node::Object>("CPU");

  debugger.load(node);
}

auto CPU::unload() -> void {
  ram.reset();
  node = {};
  debugger = {};
}

auto CPU::main() -> void {
  if(state.nmiLine) {
    state.nmiLine = 0;  //edge-sensitive
    if(nmi()) {
      debugger.interrupt("NMI");
    }
  }

  if(state.irqLine) {
    //level-sensitive
    if(irq()) {
      debugger.interrupt("IRQ");
    }
  }

  debugger.instruction();
  instruction();
}

auto CPU::step(u32 clocks) -> void {
  Thread::step(clocks);
  Thread::synchronize();
}

auto CPU::setNMI(bool value) -> void {
  state.nmiLine = value;
}

auto CPU::setIRQ(bool value) -> void {
  state.irqLine = value;
}

auto CPU::power() -> void {
  Z80::bus = this;
  Z80::power();
  Thread::create(system.colorburst(), {&CPU::main, this});
  PC = 0x0000;  //reset vector address
  SP = 0xfffd;  //initial stack pointer location

  ram.fill(0);  //fixes hang in Shanghai II (Japan) (GG)
                //bios usually clears ram, so it should be safe

  ram.write(0xc000, 0xab);  //CPU $3e initial value
  ram.write(0xc700, 0x9b);  //VDP $01 initial value
  state = {};
  bus = {};
  bus.biosEnable = (bool)bios;
  bus.cartridgeEnable = !(bool)bios;
  if(Model::MasterSystemII()) bus.pullup = 0xff;
  if(Model::GameGear()) bus.pullup = 0xff;
  sio = {};
}

}
