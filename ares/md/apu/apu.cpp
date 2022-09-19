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
  //stall the APU until the CPU relinquishes control of the bus
  if(!state.resLine || state.busreqLatch) {
    return step(1);
  }

  if(state.nmiLine) {
    state.nmiLine = 0;  //edge-sensitive
    debugger.interrupt("NMI");
    nmi();
  }

  if(state.intLine) {
    //level-sensitive
    debugger.interrupt("IRQ");
    irq();
  }

  debugger.instruction();
  instruction();
}

auto APU::step(u32 clocks) -> void {
  Thread::step(clocks);
  state.busreqLatch = busownerCPU() ? 1 : 0;
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
    Z80::power();
    ram.fill();
    state.resLine = 0;
    state.busreqLine = 0;
    state.busreqLatch = 0;
  } else {
    Z80::reset();
  }
  state.nmiLine = 0;
  state.intLine = 0;
  state.bank = 0;
  opn2.power(reset);
}

auto APU::restart() -> void {
  Z80::reset();
  Thread::restart({&APU::main, this});
  state.nmiLine = 0;
  state.intLine = 0;
  state.busreqLatch = state.busreqLine;
  opn2.power(true);
}

}
