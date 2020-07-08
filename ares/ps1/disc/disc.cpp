#include <ps1/ps1.hpp>

namespace ares::PlayStation {

Disc disc;
#include "io.cpp"
#include "command.cpp"
#include "irq.cpp"
#include "serialization.cpp"

auto Disc::load(Node::Object parent) -> void {
  node = parent->append<Node::Component>("PlayStation");

  tray = node->append<Node::Port>("Disc Tray");
  tray->setFamily("PlayStation");
  tray->setType("Compact Disc");
  tray->setHotSwappable(true);
  tray->setAllocate([&](auto name) { return allocate(tray); });
  tray->setConnect([&] { return connect(); });
  tray->setDisconnect([&] { return disconnect(); });

  fifo.parameter.resize(16);
  fifo.response.resize(16);
  fifo.data.resize(2340);
}

auto Disc::unload() -> void {
  fifo.parameter.reset();
  fifo.response.reset();
  fifo.data.reset();

  disconnect();
  tray = {};
  node = {};
}

auto Disc::allocate(Node::Port parent) -> Node::Peripheral {
  return cd = parent->append<Node::Peripheral>("PlayStation");
}

auto Disc::connect() -> void {
  cd->setManifest([&] { return information.manifest; });

  information = {};
  if(auto fp = platform->open(cd, "manifest.bml", File::Read, File::Required)) {
    information.manifest = fp->reads();
  }

  auto document = BML::unserialize(information.manifest);
  information.name = document["game/label"].string();

  fd = platform->open(cd, "cd.rom", File::Read, File::Required);
  if(!fd) return disconnect();

  //read disc TOC (table of contents)
  uint sectors = fd->size() / 2448;
  vector<uint8_t> subchannel;
  subchannel.resize(sectors * 96);
  for(uint sector : range(sectors)) {
    fd->seek(sector * 2448 + 2352);
    fd->read(subchannel.data() + sector * 96, 96);
  }
  session.decode(subchannel, 96);
}

auto Disc::disconnect() -> void {
  fd = {};
  cd = {};
}

auto Disc::main() -> void {
  if(irq.acknowledge.delay && !--irq.acknowledge.delay) {
    irq.acknowledge.flag = 1;
    irq.poll();
  } else if(irq.error.delay && !--irq.error.delay) {
    fifo.response.write(0x0a);
    fifo.response.write(0x90);
    fifo.response.write(0x00);
    fifo.response.write(0x00);
    fifo.response.write(0x00);
    fifo.response.write(0x00);
    fifo.response.write(0x00);
    fifo.response.write(0x00);

    irq.error.flag = 1;
    irq.poll();
  }
  step(33'868'800 / 75);
}

auto Disc::step(uint clocks) -> void {
  Thread::clock += clocks;
}

auto Disc::power(bool reset) -> void {
  Thread::reset();
  io = {};
}

}
