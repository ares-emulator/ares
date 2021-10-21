#include <md/md.hpp>

namespace ares::MegaDrive {

APU apu;
#include "bus.cpp"
#include "debugger.cpp"
#include "serialization.cpp"

auto APU::load(Node::Object parent) -> void {
  node = parent->append<Node::Object>("APU");
  ram.allocate(8_KiB);
  debugger.load(node);
}

auto APU::unload() -> void {
  debugger = {};
  ram.reset();
  node.reset();
}

auto APU::main() -> void {
  //handle a bus switching request from the CPU
  if(state.resLine) {
    state.busreqLatch = state.busreqLine;
  }

  //stall the APU until the CPU relinquishes control of the bus
  if(!state.resLine || state.busreqLatch) {
    return step(1);
  }

  if(state.nmiLine) {
    state.nmiLine = 0;  //edge-sensitive
    debugger.interrupt("NMI");
    irq(0, 0x0066, 0xff);
  }

  if(state.intLine) {
    //level-sensitive
    debugger.interrupt("IRQ");
    irq(1, 0x0038, 0xff);
  }

  debugger.instruction();
  instruction();
}

auto APU::step(u32 clocks) -> void {
  Thread::step(clocks);
  Thread::synchronize(cpu, vdp, vdp.psg, opn2);
}

auto APU::setNMI(n1 line) -> void {
  state.nmiLine = line;
}

auto APU::setINT(n1 line) -> void {
  state.intLine = line;
}

auto APU::setRES(n1 line) -> void {
  if(!state.resLine && line) restart();
  state.resLine = line;
}

auto APU::setBUSREQ(n1 line) -> void {
  state.busreqLine = line;
}

auto APU::power(bool reset) -> void {
  Z80::bus = this;
  Z80::power();
  Thread::create(system.frequency() / 15.0, {&APU::main, this});
  if(!reset) {
    ram.fill();
    state.resLine = 0;
    state.busreqLine = 0;
    state.busreqLatch = 0;
  }
  state.nmiLine = 0;
  state.intLine = 0;
  state.bank = 0;
  opn2.power(reset);
}

auto APU::restart() -> void {
  Z80::power();
  Thread::restart({&APU::main, this});
  state.nmiLine = 0;
  state.intLine = 0;
  state.bank = 0;
  opn2.power(true);
}

}
