#include <sfc/sfc.hpp>

namespace ares::SuperFamicom {

CPU cpu;
#include "dma.cpp"
#include "memory.cpp"
#include "io.cpp"
#include "timing.cpp"
#include "irq.cpp"
#include "debugger.cpp"
#include "serialization.cpp"

auto CPU::load(Node::Object parent) -> void {
  node = parent->append<Node::Object>("CPU");

  version = parent->append<Node::Setting::Natural>("Version", 2);
  version->setAllowedValues({1, 2});

  debugger.load(node);
}

auto CPU::unload() -> void {
  version = {};
  debugger = {};
  node = {};
}

auto CPU::main() -> void {
  if(r.wai) return instructionWait();
  if(r.stp) return instructionStop();

  if(!status.interruptPending) {
    debugger.instruction();
    return instruction();
  }

  if(status.nmiPending) {
    status.nmiPending = 0;
    r.vector = r.e ? 0xfffa : 0xffea;
    debugger.interrupt("NMI");
    return interrupt();
  }

  if(status.irqPending) {
    status.irqPending = 0;
    r.vector = r.e ? 0xfffe : 0xffee;
    debugger.interrupt("IRQ");
    return interrupt();
  }

  if(status.resetPending) {
    status.resetPending = 0;
    step(132);
    r.vector = 0xfffc;
    debugger.interrupt("Reset");
    return interrupt();  //H=186
  }

  status.interruptPending = 0;
}

auto CPU::map() -> void {
  std::function<n8   (n24, n8)> reader;
  std::function<void (n24, n8)> writer;

  reader = std::bind_front(&CPU::readRAM, this);
  writer = std::bind_front(&CPU::writeRAM, this);
  bus.map(reader, writer, "00-3f,80-bf:0000-1fff", 0x2000);
  bus.map(reader, writer, "7e-7f:0000-ffff", 0x20000);

  reader = std::bind_front(&CPU::readAPU, this);
  writer = std::bind_front(&CPU::writeAPU, this);
  bus.map(reader, writer, "00-3f,80-bf:2140-217f");

  reader = std::bind_front(&CPU::readCPU, this);
  writer = std::bind_front(&CPU::writeCPU, this);
  bus.map(reader, writer, "00-3f,80-bf:2180-2183,4016-4017,4200-421f");

  reader = std::bind_front(&CPU::readDMA, this);
  writer = std::bind_front(&CPU::writeDMA, this);
  bus.map(reader, writer, "00-3f,80-bf:4300-437f");
}

auto CPU::power(bool reset) -> void {
  WDC65816::power();
  create(system.cpuFrequency(), std::bind_front(&CPU::main, this));
  coprocessors.clear();
  PPUcounter::reset();
  PPUcounter::scanline = std::bind_front(&CPU::scanline, this);

  if(!reset) random.array({(u8*)wram, sizeof(wram)});

  for(u32 id : range(8)) {
    channels[id] = {};
    channels[id].id = id;
    if(id != 7) channels[id].next = channels[id + 1];
  }

  counter = {};

  io = {};
  io.version = version->value();

  alu = {};

  status = {};
  status.dramRefreshPosition = (io.version == 1 ? 530 : 538);
  status.hdmaSetupPosition = (io.version == 1 ? 12 + 8 - dmaCounter() : 12 + dmaCounter());
  status.hdmaPosition = 1104;
  status.resetPending = 1;
  status.interruptPending = 1;
}

}
