#include <n64/n64.hpp>

namespace ares::Nintendo64 {

DD dd;
#include "controller.cpp"
#include "io.cpp"
#include "debugger.cpp"
#include "serialization.cpp"

auto DD::load(Node::Object parent) -> void {
  node = parent->append<Node::Object>("Nintendo 64DD");

  drive = node->append<Node::Port>("Disk Drive");
  drive->setFamily("Nintendo 64DD");
  drive->setType("Floppy Disk");
  drive->setHotSwappable(true);
  drive->setAllocate([&](auto name) { return allocate(drive); });
  drive->setConnect([&] { return connect(); });
  drive->setDisconnect([&] { return disconnect(); });

  iplrom.allocate(4_MiB);
  c2s.allocate(0x400);
  ds.allocate(0x100);
  ms.allocate(0x40);
  rtc.allocate(0x8);

  // TODO: Detect correct CIC from ipl rom
  if(auto fp = system.pak->read("64dd.ipl.rom")) {
    iplrom.load(fp);
  }

  debugger.load(node);
}

auto DD::unload() -> void {
  if(!node) return;
  disconnect();

  debugger = {};
  iplrom.reset();
  c2s.reset();
  ds.reset();
  ms.reset();
  rtc.reset();
  drive.reset();
  node.reset();
}

auto DD::allocate(Node::Port parent) -> Node::Peripheral {
  return disk = parent->append<Node::Peripheral>("Nintendo 64DD Disk");
}

auto DD::connect() -> void {
  if(!disk->setPak(pak = platform->pak(disk))) return;

  information = {};
  information.title = pak->attribute("title");

  fd = pak->read("program.disk");
  if(!fd) return disconnect();

  if(auto fp = system.pak->read("time.rtc")) {
    rtc.load(fp);
  }
}

auto DD::disconnect() -> void {
  if(!drive) return;
  save();
  pak.reset();
  information = {};
}

auto DD::save() -> void {
  if(auto fp = system.pak->write("time.rtc")) {
    rtc.save(fp);
  }
}

auto DD::power(bool reset) -> void {
  c2s.fill();
  ds.fill();
  ms.fill();

  irq = {};
  ctl = {};
  io = {};

  io.status.resetState = 1;
}

auto DD::raise(IRQ source) -> void {
  debugger.interrupt((u32)source);
  switch(source) {
  case IRQ::MECHA: irq.mecha.line = 1; break;
  case IRQ::BM: irq.bm.line = 1; break;
  }
  poll();
}

auto DD::lower(IRQ source) -> void {
  switch(source) {
  case IRQ::MECHA: irq.mecha.line = 0; break;
  case IRQ::BM: irq.bm.line = 0; break;
  }
  poll();
}

auto DD::poll() -> void {
  bool line = 0;
  line |= irq.mecha.line & irq.mecha.mask;
  line |= irq.bm.line & irq.bm.mask;
  cpu.scc.cause.interruptPending.bit(3) = line;
}

}
