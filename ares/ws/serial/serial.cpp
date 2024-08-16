#include <ws/ws.hpp>

namespace ares::WonderSwan {

Serial serial;
#include "debugger.cpp"
#include "serialization.cpp"

auto Serial::load(Node::Object parent) -> void {
  node = parent->append<Node::Object>("Serial");
  debugger.load(node);
}

auto Serial::unload() -> void {
  debugger.unload(node);
  node.reset();
}

auto Serial::main() -> void {
  step(80);
  
  if (!state.enable) return;
  if (!state.baudRate && ++state.baudClock < 4) return;
  state.baudClock = 0;

  // stub implementation
  if(state.txFull) {
    if(++state.txBitClock == 9) {
      state.txBitClock = 0;
      state.txFull = 0;
    }
  }
  cpu.irqLevel(CPU::Interrupt::SerialSend, !state.txFull);
  cpu.irqLevel(CPU::Interrupt::SerialReceive, state.rxFull);
}

auto Serial::step(u32 clocks) -> void {
  Thread::step(clocks);
  Thread::synchronize(cpu);
}

auto Serial::readIO(n16 address) -> n8 {
  n8 data;

  switch(address) {

  case 0x00b1:  //SER_DATA
    data = state.dataRx;
    state.rxFull = 0;
    break;

  case 0x00b3:  //SER_STATUS
    data.bit(0) = state.rxFull;
    data.bit(1) = state.rxOverrun;
    data.bit(2) = !state.txFull;
    data.bit(6) = state.baudRate;
    data.bit(7) = state.enable;
    break;

  }

  return data;
}

auto Serial::writeIO(n16 address, n8 data) -> void {
  switch(address) {

  case 0x00b1:  //SER_DATA
    if(!state.txFull) {
      state.dataTx = data;
      state.txFull = 1;
    }
    break;

  case 0x00b3:  //SER_STATUS
    state.rxOverrun &= ~data.bit(5);
    state.baudRate  = data.bit(6);
    state.enable    = data.bit(7);
    break;

  }

  return;
}

auto Serial::power() -> void {
  Thread::create(3'072'000, {&Serial::main, this});

  bus.map(this, 0x00b1);
  bus.map(this, 0x00b3);

  state = {};
}

}
