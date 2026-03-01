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
  information.executable = pak->attribute("executable").boolean();

  if(!executable()) {
    fd = pak->read("cd.rom");
    if(!fd) return disconnect();

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
  }
}

  auto Disc::disconnect() -> void {
  fd.reset();
  cd.reset();
  pak.reset();

  information = {};
  command = {};

  ssr = {};
  ssr.shellOpen = 1;
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

  ssr.error = 1;
  queueResponse(ResponseType::Error, { status(), ErrorCode_DoorOpen });
}

auto Disc::main() -> void {
  flushDeferredResponse();
  counter.sector += 128;
  if(counter.sector >= 451584 >> drive.mode.speed) {
    //75hz (single speed) or 37.5hz (double speed)
    counter.sector -= 451584 >> drive.mode.speed;
    drive.clockSector();
  }

  counter.cdda += 128;
  if(counter.cdda >= 768) {
    //44100hz
    counter.cdda -= 768;
    cdda.clockSample();
  }

  counter.cdxa += 128;
  if(counter.cdxa >= 896) {
    //37800hz
    counter.cdxa -= 896;
    cdxa.clockSample();
  }

  if(command.transfer.started) {
    command.transfer.counter -= 128;
    if(command.transfer.counter <= 0) {
      command.transfer.counter = 0;
      command.transfer.started = 0;

      if(command.current.pending) {
        command.queued.command = command.transfer.command;
        command.queued.pending = 1;
      } else {
        command.current.command = command.transfer.command;
        command.current.pending = 1;
        command.current.invocation = 0;
        command.current.counter = 0;

        executeCommand(command.current.command);

        if(command.current.counter == 0 && command.current.invocation == 0) {
          command.current.pending = 0;
        }
      }
    }
  }

  if(command.current.pending && command.current.counter > 0) {
    command.current.counter -= 128;
    if(command.current.counter <= 0) {
      command.current.counter = 0;
      executeCommand(command.current.command);
      if(command.current.counter == 0 && command.current.invocation == 0) {
        command.current.pending = 0;
      }
    }
  }

  if(!command.current.pending && command.queued.pending) {
    command.current.command = command.queued.command;
    command.current.pending = 1;
    command.current.invocation = 0;
    command.current.counter = 0;
    command.queued.pending = 0;

    executeCommand(command.current.command);

    if(command.current.counter == 0 && command.current.invocation == 0) {
      command.current.pending = 0;
    }
  }


  if(drive.mode.report && ssr.playingCDDA) {
    counter.report -= 128;
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

auto Disc::step(u32 clocks) -> void {
  static u32 clockAccumulator = 0;
  clockAccumulator += clocks;
  while (clockAccumulator >= 128) {
    main();
    clockAccumulator -= 128;
  }
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
  drive.recentlyReset = 75 << drive.mode.speed;
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
