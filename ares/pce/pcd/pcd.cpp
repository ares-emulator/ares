#include <pce/pce.hpp>

namespace ares::PCEngine {

PCD pcd;
#include "io.cpp"
#include "bram.cpp"
#include "scsi.cpp"
#include "adpcm.cpp"
#include "debugger.cpp"
#include "serialization.cpp"

auto PCD::load(Node::Object parent) -> void {
  node = parent->append<Node::Component>("PC Engine CD");

  tray = node->append<Node::Port>("Disc Tray");
  tray->setFamily("PC Engine CD");
  tray->setType("Compact Disc");
  tray->setHotSwappable(true);
  tray->setAllocate([&](auto name) { return allocate(tray); });
  tray->setConnect([&] { return connect(); });
  tray->setDisconnect([&] { return disconnect(); });

  wram.allocate(64_KiB);
  bram.load(node);
  adpcm.load(node);
  debugger.load(node);
}

auto PCD::unload() -> void {
  wram.reset();
  bram.unload();
  adpcm.unload();
  disconnect();
  node = {};
  tray = {};
}

auto PCD::allocate(Node::Port parent) -> Node::Peripheral {
  return disc = parent->append<Node::Peripheral>("PC Engine CD");
}

auto PCD::connect() -> void {
  disc->setManifest([&] { return information.manifest; });

  information = {};
  if(auto fp = platform->open(disc, "manifest.bml", File::Read, File::Required)) {
    information.manifest = fp->reads();
  }

  auto document = BML::unserialize(information.manifest);
  information.name = document["game/label"].string();

  fd = platform->open(disc, "cd.rom", File::Read, File::Required);

  uint sectors = fd->size() / 2448;
  vector<uint8_t> subchannel;
  subchannel.resize(sectors * 96);
  for(uint sector : range(sectors)) {
    fd->seek(sector * 2448 + 2352);
    fd->read(subchannel.data() + sector * 96, 96);
  }
  session.decode(subchannel, 96);
}

auto PCD::disconnect() -> void {
  session = {};
  disc = {};
  fd = {};
}

auto PCD::save() -> void {
}

auto PCD::main() -> void {
  scsi.clock();
  adpcm.clock();
  step(1);
}

auto PCD::step(uint clocks) -> void {
  Thread::step(clocks);
  Thread::synchronize(cpu);
}

auto PCD::irqLine() const -> bool {
  if(scsi.irq.ready.pending && scsi.irq.ready.enable) return 1;
  if(scsi.irq.completed.pending && scsi.irq.completed.enable) return 1;
  if(adpcm.irq.halfPlay.pending && adpcm.irq.halfPlay.enable) return 1;
  if(adpcm.irq.fullPlay.pending && adpcm.irq.fullPlay.enable) return 1;
  return 0;
}

auto PCD::power() -> void {
  Thread::create(9'216'000, {&PCD::main, this});
  scsi = {};
  adpcm.power();
  io = {};
}

}
