#include <md/md.hpp>

namespace ares::MegaDrive {

static n16 Unmapped = 0;

MCD mcd;
#include "bus.cpp"
#include "bus-external.cpp"
#include "io.cpp"
#include "io-external.cpp"
#include "irq.cpp"
#include "cdc.cpp"
#include "cdc-transfer.cpp"
#include "cdd.cpp"
#include "cdd-dac.cpp"
#include "timer.cpp"
#include "gpu.cpp"
#include "pcm.cpp"
#include "debugger.cpp"
#include "serialization.cpp"

auto MCD::load(Node::Object parent) -> void {
  node = parent->append<Node::Object>("Mega CD");

  tray = parent->append<Node::Port>("Disc Tray");
  tray->setFamily("Mega CD");
  tray->setType("Compact Disc");
  tray->setHotSwappable(true);
  tray->setAllocate([&](auto name) { return allocate(tray); });
  tray->setConnect([&] { return connect(); });
  tray->setDisconnect([&] { return disconnect(); });

  bios.allocate   (128_KiB >> 1);
  pram.allocate   (512_KiB >> 1);
  wram.allocate   (256_KiB >> 1);
  bram.allocate   (  8_KiB >> 0);
  cdc.ram.allocate( 16_KiB >> 1);

  if(expansion.node) {
    if(auto fp = expansion.pak->read("backup.ram")) {
      bram.load(fp);
    }
  }

  cdd.load(node);
  pcm.load(node);
  debugger.load(node);
}

auto MCD::unload() -> void {
  if(!node) return;
  disconnect();

  if(expansion.node) {
    if(auto fp = expansion.pak->write("backup.ram")) {
      bram.save(fp);
    }
  }

  debugger = {};
  cdd.unload(node);
  pcm.unload(node);

  bios.reset();
  pram.reset();
  wram.reset();
  bram.reset();
  cdc.ram.reset();

  tray.reset();
  node.reset();
}

auto MCD::allocate(Node::Port parent) -> Node::Peripheral {
  return disc = parent->append<Node::Peripheral>("Mega CD");
}

auto MCD::connect() -> void {
  if(!disc->setPak(pak = platform->pak(node))) return;

  information = {};
  if(auto fp = pak->read("manifest.bml")) {
    information.manifest = fp->reads();
  }
  auto document = BML::unserialize(information.manifest);
  information.name = document["game/label"].string();

  fd = pak->read("cd.rom");
  cdd.insert();
}

auto MCD::disconnect() -> void {
  if(!disc) return;
  cdd.eject();
  disc.reset();
  fd.reset();
  pak.reset();
  information = {};
}

auto MCD::main() -> void {
  if(io.halt) return wait(16);

  if(irq.pending) {
    if(1 > r.i && gpu.irq.lower()) {
      debugger.interrupt("GPU");
      return interrupt(Vector::Level1, 1);
    }
    if(2 > r.i && external.irq.lower()) {
      debugger.interrupt("External");
      return interrupt(Vector::Level2, 2);
    }
    if(3 > r.i && timer.irq.lower()) {
      debugger.interrupt("Timer");
      return interrupt(Vector::Level3, 3);
    }
    if(4 > r.i && cdd.irq.lower()) {
      debugger.interrupt("CDD");
      return interrupt(Vector::Level4, 4);
    }
    if(5 > r.i && cdc.irq.lower()) {
      debugger.interrupt("CDC");
      return interrupt(Vector::Level5, 5);
    }
    if(6 > r.i && irq.subcode.lower()) {
      debugger.interrupt("IRQ");
      return interrupt(Vector::Level6, 6);
    }
    if(irq.reset.lower()) {
      debugger.interrupt("Reset");
      r.a[7] = read(1, 1, 0) << 16 | read(1, 1, 2) << 0;
      r.pc   = read(1, 1, 4) << 16 | read(1, 1, 6) << 0;
      prefetch();
      prefetch();
      return;
    }
  }

  debugger.instruction();
  instruction();
}

auto MCD::step(u32 clocks) -> void {
  gpu.step(clocks);
  counter.divider += clocks;
  while(counter.divider >= 384) {
    counter.divider -= 384;
    cdc.clock();
    cdd.clock();
    timer.clock();
    pcm.clock();
  }
  counter.dma += clocks;
  while(counter.dma >= 6) {
    counter.dma -= 6;
    cdc.transfer.dma();
  }
  counter.pcm += clocks;
  while(counter.pcm >= frequency() / 44100.0) {
    counter.pcm -= frequency() / 44100.0;
    cdd.sample();
  }

  Thread::step(clocks);
}

auto MCD::idle(u32 clocks) -> void {
  step(clocks);
}

auto MCD::wait(u32 clocks) -> void {
  step(clocks);
  Thread::synchronize(cpu);
}

auto MCD::power(bool reset) -> void {
  if(auto fp = expansion.pak->read("program.rom")) {
    for(u32 address : range(bios.size())) bios.program(address, fp->readm(2));
  }

  M68K::power();
  Thread::create(12'500'000, {&MCD::main, this});
  counter = {};
  if(!reset) {
    io = {};
    led = {};
    irq = {};
    external = {};
    communication = {};
    cdc.power(reset);
    cdd.power(reset);
    timer.power(reset);
    gpu.power(reset);
    pcm.power(reset);
  }
  irq.reset.enable = 1;
  irq.reset.raise();
  bios.program(0x72 >> 1, 0xffff);
}

}
