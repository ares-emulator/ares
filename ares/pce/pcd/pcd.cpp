#include <pce/pce.hpp>

namespace ares::PCEngine {

PCD pcd;
#include "io.cpp"
#include "drive.cpp"
#include "scsi.cpp"
#include "cdda.cpp"
#include "adpcm.cpp"
#include "fader.cpp"
#include "debugger.cpp"
#include "serialization.cpp"

auto PCD::load(Node::Object parent) -> void {
  node = parent->append<Node::Object>("PC Engine CD");

  tray = node->append<Node::Port>("Disc Tray");
  tray->setFamily("PC Engine CD");
  tray->setType("Compact Disc");
  tray->setHotSwappable(true);
  tray->setAllocate([&](auto name) { return allocate(tray); });
  tray->setConnect([&] { return connect(); });
  tray->setDisconnect([&] { return disconnect(); });

  //subclass simulation
  drive.session = session;
  scsi.session = session;
  scsi.drive = drive;
  scsi.cdda = cdda;
  scsi.adpcm = adpcm;
  cdda.drive = drive;
  cdda.scsi = scsi;
  cdda.fader = fader;
  adpcm.scsi = scsi;
  adpcm.fader = fader;

  wram.allocate(64_KiB);
  bram.allocate( 2_KiB);
  if(Model::PCEngineDuo()) {
    bios.allocate(256_KiB);
    sram.allocate(192_KiB);
  }

  cdda.load(node);
  adpcm.load(node);
  debugger.load(node);
}

auto PCD::unload() -> void {
  if(!node) return;
  disconnect();

  debugger = {};
  wram.reset();
  bram.reset();
  if(Model::PCEngineDuo()) {
    bios.reset();
    sram.reset();
  }

  cdda.unload(node);
  adpcm.unload(node);

  tray.reset();
  node.reset();
}

auto PCD::allocate(Node::Port parent) -> Node::Peripheral {
  return disc = parent->append<Node::Peripheral>("PC Engine CD Disc");
}

auto PCD::connect() -> void {
  if(!disc->setPak(pak = platform->pak(disc))) return;

  information = {};
  information.title = pak->attribute("title");

  fd = pak->read("cd.rom");
  if(!fd) return disconnect();

  //read TOC (table of contents) from disc lead-in
  u32 sectors = fd->size() / 2448;
  vector<u8> subchannel;
  subchannel.resize(sectors * 96);
  for(u32 sector : range(sectors)) {
    fd->seek(sector * 2448 + 2352);
    fd->read({subchannel.data() + sector * 96, 96});
  }
  session.decode(subchannel, 96);

  if(auto fp = system.pak->read("backup.ram")) {
    bram.load(fp);
  }
}

auto PCD::disconnect() -> void {
  if(!disc) return;

  save();
  fd.reset();
  pak.reset();
  disc.reset();
}

auto PCD::save() -> void {
  if(auto fp = system.pak->write("backup.ram")) {
    bram.save(fp);
  }
}

auto PCD::main() -> void {
  if(++clock.drive == 122892) {
    //75hz
    clock.drive = 0;
    scsi.clockSector();
    cdda.clockSector();
  }

  if(++clock.cdda == 209) {
    //44100hz
    clock.cdda = 0;
    cdda.clockSample();
  }

  if(++clock.adpcm == 288) {
    //32000hz
    clock.adpcm = 0;
    adpcm.clockSample();
  }

  if(++clock.fader == 9217) {
    //1000hz
    clock.fader = 0;
    fader.clock();
  }

  scsi.clock();
  adpcm.clock();
  step(1);
}

auto PCD::step(u32 clocks) -> void {
  Thread::step(clocks);
  Thread::synchronize(cpu);
}

auto PCD::irqLine() const -> bool {
  if(scsi.irq.ready.poll()) return 1;
  if(scsi.irq.completed.poll()) return 1;
  if(adpcm.irq.halfReached.poll()) return 1;
  if(adpcm.irq.endReached.poll()) return 1;
  return 0;
}

auto PCD::power() -> void {
  Thread::create(9'216'900, {&PCD::main, this});
  drive.power();
  scsi.power();
  cdda.power();
  adpcm.power();
  fader.power();
  clock = {};
  io = {};

  if(Model::PCEngineDuo()) {
    if(auto fp = system.pak->read("bios.rom")) {
      bios.load(fp);
    }
  }
}

}
