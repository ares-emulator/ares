#include <ps1/ps1.hpp>

namespace ares::PlayStation {

Disc disc;
#include "drive.cpp"
#include "cdda.cpp"
#include "cdxa.cpp"
#include "io.cpp"
#include "command.cpp"
#include "irq.cpp"
#include "debugger.cpp"
#include "serialization.cpp"

auto Disc::load(Node::Object parent) -> void {
  node = parent->append<Node::Object>("PlayStation");

  tray = node->append<Node::Port>("Disc Tray");
  tray->setFamily("PlayStation");
  tray->setType("Compact Disc");
  tray->setHotSwappable(true);
  tray->setAllocate([&](auto name) { return allocate(tray); });
  tray->setConnect([&] { return connect(); });
  tray->setDisconnect([&] { return disconnect(); });

  //subclass simulation
  drive.session = session;
  drive.cdda = cdda;
  drive.cdxa = cdxa;
  cdda.drive = drive;
  cdxa.drive = drive;

  cdda.load(node);
  cdxa.load(node);
  debugger.load(node);
}

auto Disc::unload() -> void {
  debugger = {};
  cdda.unload(node);
  cdxa.unload(node);

  disconnect();
  tray.reset();
  node.reset();
}

auto Disc::allocate(Node::Port parent) -> Node::Peripheral {
  return cd = parent->append<Node::Peripheral>("PlayStation Disc");
}

auto Disc::connect() -> void {
  if(!cd->setPak(pak = platform->pak(cd))) return;

  information = {};
  information.title      = pak->attribute("title");
  information.region     = pak->attribute("region");
  information.audio      = pak->attribute("audio").boolean();
  fd = pak->read("cd.rom");
  if(!fd) return disconnect();

  if(pak->attribute("executable-disc").boolean()) {
    information.region = Region::NTSCJ() ? "NTSC-J" : Region::PAL() ? "PAL" : "NTSC-U";
    auto* raw = fd->data() + (u64)(CD::LeadInSectors + CD::Track1Pregap + 4) * 2448;
    string license = Region::NTSCJ() ? "Sony Computer Entertainment Inc.\n"
                   : Region::PAL()   ? "Sony Computer Entertainment Euro pe   "
                                     : "Sony Computer Entertainment Amer  ica ";

    memory::fill<u8>(raw + 24 + 32, 2048 - 32, 0);
    memory::copy(raw + 24 + 32, license.data(), license.size());
    if(Region::NTSCJ()) {
      for(u32 n : range(1983)) raw[24 + 65 + n] = n % 64 == 62 ? '\n' : '0';
    }

    CD::EDC::create({raw + 16, 2056}, {raw + 2072, 4});
    u8 header[4];
    memory::copy(header, raw + 12, 4);
    memory::fill<u8>(raw + 12, 4, 0);
    CD::RSPC::encodeMode1({raw, 2352});
    memory::copy(raw + 12, header, 4);
  }

  //read TOC (table of contents) from disc lead-in
  u32 sectors = fd->size() / 2448;
  std::vector<u8> subchannel;
  subchannel.resize(sectors * 96);
  for(u32 sector : range(sectors)) {
    fd->seek(sector * 2448 + 2352);
    fd->read({subchannel.data() + sector * 96, 96});
  }
  session.decode(subchannel, 96);
  ssr.motorOn = 1;

  ssr.shellOpen = 0;
}

auto Disc::disconnect() -> void {
  bool active = drive.standby || drive.seeking || ssr.reading || ssr.playingCDDA;
  u8 ejectStatus = active ? (status() & 0x02) | 0x14 : 0x01;
  fd.reset();
  cd.reset();
  pak.reset();

  information = {};
  command = {};
  drive.seeking = 0;
  drive.seekDelay = 0;
  drive.seekType = Drive::SeekType::None;
  drive.pendingOperation = Drive::PendingOperation::None;
  drive.seekRetries = 0;
  drive.headerValid = 0;
  drive.standby = 0;
  drive.lba.pending = 0;

  ssr = {};
  ssr.motorOn = 0;
  ssr.reading = 0;
  ssr.playingCDDA = 0;
  ssr.seekError = 0;
  ssr.idError = 0;

  fifo.parameter.flush();
  fifo.response.flush();
  fifo.data.flush();
  fifo.deferred = {};

  irq.ready.flag = 0;
  irq.complete.flag = 0;
  irq.acknowledge.flag = 0;
  irq.end.flag = 0;
  irq.error.flag = 0;
  irq.poll();

  ssr.error = 1;
  queueResponse(ResponseType::Error, { ejectStatus, ErrorCode_DoorOpen });
  ssr.shellOpen = 1;
}

auto Disc::step(u32 clocks) -> void {
  if(fifo.deferred.counter > 0) {
    fifo.deferred.counter -= min<s32>(fifo.deferred.counter, clocks);
  }
  flushDeferredResponse();

  counter.sector += clocks;
  u32 sectorPeriod = 451584 >> drive.mode.speed;
  if(unlikely(counter.sector >= sectorPeriod)) {
    counter.sector -= sectorPeriod;
    drive.clockSector();
  }

  counter.cdda += clocks;
  while(counter.cdda >= 768) {
    counter.cdda -= 768;
    cdda.clockSample();
  }

  counter.cdxa += clocks;
  while(counter.cdxa >= 896) {
    counter.cdxa -= 896;
    cdxa.clockSample();
  }

  if(unlikely(command.transfer.started) && !irq.pending()
  && fifo.deferred.acknowledge.type == ResponseType::None
  && fifo.deferred.complete.type == ResponseType::None
  && fifo.deferred.ready.type == ResponseType::None) {
    command.transfer.counter -= clocks;
    if(command.transfer.counter <= 0) {
      command.transfer.counter = 0;
      command.transfer.started = 0;
      executeCommand(command.transfer.command);
    }
  }

  if(unlikely(command.secondResponse.pending && command.secondResponse.counter > 0)) {
    command.secondResponse.counter -= clocks;
    if(command.secondResponse.counter <= 0) {
      command.secondResponse.counter = 0;
      command.secondResponse.pending = 0;
      executeSecondResponse(command.secondResponse.command);
    }
  }

  if(unlikely(drive.mode.report && ssr.playingCDDA)) {
    counter.report -= clocks;
    if(counter.report <= 0) {
      counter.report += system.frequency() / 75;

      s32 lba = drive.lba.current;
      s32 lbaTrack = 0;
      u8  inTrack = 0;
      u8  inIndex = 0;
      if(auto trackID = session.inTrack(lba)) {
        inTrack = *trackID;
        if(auto track = session.track(inTrack)) {
          if(auto indexID = track->inIndex(lba)) {
            inIndex = *indexID;
            if(auto index = track->index(1)) {
              lbaTrack = index->lba;
            }
          }
        }
      }

      auto amsf = CD::MSF::fromABA(CD::LBAtoABA(lba));
      auto rmsf = CD::MSF::fromLBA(lba - lbaTrack);
      u8 aminute = amsf.minute, asecond = amsf.second, aframe = amsf.frame;
      u8 rminute = rmsf.minute, rsecond = rmsf.second, rframe = rmsf.frame;

      //sectors  0, 20, 40, 60 report absolute time
      //sectors 10, 30, 50, 70 report relative time
      //note: bad sector reads report immediately; but bad sectors are not emulated
      u8 frameBCD = BCD::encode(aframe);
      if((frameBCD & 15) == 0) {
        bool relative = frameBCD & 16;

        queueResponse(ResponseType::Ready, {
          status(),
          BCD::encode(inTrack),
          BCD::encode(inIndex),
          BCD::encode(!relative ? aminute : rminute),
          u8(BCD::encode(!relative ? asecond : rsecond) | (relative << 7)),
          BCD::encode(!relative ? aframe  : rframe),
          0xff,  //todo: peak lo
          u8(0x7f | relative << 7),  //todo: peak hi + left/right channel
        });
      }
    }
  }
}

auto Disc::nextEvent() const -> u32 {
  u32 next = ~0u;
  auto schedule = [&](s32 clocks) {
    next = min(next, (u32)max(1, clocks));
  };

  if(fifo.deferred.counter > 0) schedule(fifo.deferred.counter);
  if(command.transfer.started && command.transfer.counter > 0) schedule(command.transfer.counter);
  if(command.secondResponse.pending && command.secondResponse.counter > 0) schedule(command.secondResponse.counter);
  if(ssr.reading || ssr.playingCDDA || drive.seeking || drive.seekDelay) {
    schedule((451584 >> drive.mode.speed) - counter.sector);
  }
  if(ssr.playingCDDA) schedule(768 - counter.cdda);
  if(!cdxa.samples.empty()) schedule(896 - counter.cdxa);
  if(drive.mode.report && ssr.playingCDDA) schedule(counter.report);
  return next == ~0u ? 0 : next;
}

auto Disc::power(bool reset) -> void {
  drive.lba.current = 0;
  drive.lba.request = 0;
  drive.lba.pending = 0;
  for(auto& v : drive.sector.data) v = 0;
  for(auto& v : drive.sector.subq) v = 0;
  drive.sector.offset = 0;
  drive.mode = {};
  drive.seeking = 0;
  drive.seekDelay = 0;
  drive.seekRetries = 0;
  drive.seekType = Drive::SeekType::None;
  drive.pendingOperation = Drive::PendingOperation::None;
  drive.headerValid = 0;
  drive.standby = 0;
  audio = {};
  //configure for stereo sound at 100% volume level
  audio.volume[0] = audio.volumeLatch[0] = 0x80;
  audio.volume[1] = audio.volumeLatch[1] = 0x00;
  audio.volume[2] = audio.volumeLatch[2] = 0x00;
  audio.volume[3] = audio.volumeLatch[3] = 0x80;
  cdda.playMode = CDDA::PlayMode::Normal;
  cdda.sample.left = 0;
  cdda.sample.right = 0;
  cdxa.filter = {};
  cdxa.sample.left = 0;
  cdxa.sample.right = 0;
  cdxa.monaural = 0;
  cdxa.samples.flush();
  for(auto& v : cdxa.previousSamples) v = 0;
  command = {};
  irq = {};
  fifo.parameter.flush();
  fifo.response.flush();
  fifo.data.flush();
  fifo.deferred = {};
  psr = {};
  ssr = {};
  io = {};
  counter.sector = 0;
  counter.cdda = 0;
  counter.cdxa = 0;
  counter.report = 0;
}

}
