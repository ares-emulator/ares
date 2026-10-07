#include <ps1/ps1.hpp>

namespace ares::PlayStation {

Disc disc;
#include "drive.cpp"
#include "cdda.cpp"
#include "audio.cpp"
#include "cdxa.cpp"
#include "io.cpp"
#include "command.cpp"
#include "irq.cpp"
#include "debugger.cpp"
#include "serialization.cpp"

auto Disc::load(Node::Object parent) -> void {
  node = parent->append<Node::Object>("PlayStation");

  lidOpen = false;
  manualLidControl = false;
  lidSetting = node->append<Node::Setting::String>("CD-ROM Lid", "Automatic", [&](auto value) {
    // Restored state also updates the setting; only a different mode is a new user command.
    if(value == "Automatic") {
      if(manualLidControl) setLidOpen(noDisc(), false);
    } else {
      bool open = value == "Open";
      if(!manualLidControl || lidOpen != open) setLidOpen(open);
    }
  });
  lidSetting->setAllowedValues({"Automatic", "Open", "Closed"});
  lidSetting->setDynamic(true);

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

  disconnect(false);
  lidSetting.reset();
  tray.reset();
  node.reset();
}

auto Disc::allocate(Node::Port parent) -> Node::Peripheral {
  return cd = parent->append<Node::Peripheral>("PlayStation Disc");
}

auto Disc::connect() -> void {
  if(!cd->setPak(pak = platform->pak(cd))) return;

  if(fd && canReadMedia()) invalidateMedia(true);
  fd.reset();
  session = {};
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
    if(!manualLidControl) lidOpen = false;
    u32 opening = drive.shellOpening;
    stopDriveActivity();
    drive.shellOpening = opening;
    drive.setHold(0, 0);
    ssr.motorOn = 0;
    if(canReadMedia()) drive.spinUp = system.frequency();
  }
}

auto Disc::disconnect(bool forSwap) -> void {
  if(fd || !lidOpen) invalidateMedia(forSwap && bool(fd));
  if(!manualLidControl) lidOpen = true;
  fd.reset();
  cd.reset();
  pak.reset();
  session = {};
  information = {};
}

auto Disc::invalidateMedia(bool forSwap) -> void {
  // R2 reference stop delay always assumes a spinning motor, even when already stopped.
  u32 opening = forSwap && !manualLidControl ? (drive.mode.speed ? 25'000'000 : 13'000'000) + 67'737'600 : 0;
  command = {};
  stopDriveActivity();
  drive.shellOpening = opening;
  drive.lba.seek = 0;
  drive.headerValid = false;
  ssr.shellOpen = 1;
  ssr.motorOn = 0;
  ssr.seekError = 0;
  ssr.idError = 0;
  ssr.error = 0;
  // Reference removal retains parameter and Setloc latches; cancel work, not a later host-selected target.
  // Reference removal preserves already decoded host/audio data; a new stream resets it.
  fifo.deferred = {};
  // Keep an asserted interrupt and its response readable until the host acknowledges it.
  queueResponse(ResponseType::Error, {u8(status() | 1), ErrorCode_DoorOpen}, true);
}

auto Disc::setLidOpen(bool open, bool manual) -> void {
  manualLidControl = manual;
  bool changed = lidOpen != open;
  lidOpen = open;
  if(open && changed) {
    if(!drive.shellOpening) invalidateMedia();
    else ssr.shellOpen = 1;  // Changing manual input must not cancel an already-running automatic opening event.
  } else if(!open && canReadMedia() && !ssr.motorOn && !drive.spinUp) {
    // Closing starts the spindle; Nop separately acknowledges the sticky shell-open bit.
    drive.spinUp = system.frequency();
  }
  synchronizeLidSetting();
}

auto Disc::synchronizeLidSetting() -> void {
  if(lidSetting) lidSetting->setValue(!manualLidControl ? "Automatic" : lidOpen ? "Open" : "Closed");
}

auto Disc::main() -> void {
  counter.audio += 128;
  if(counter.audio >= 768) {
    counter.audio -= 768;
    audio.clockSample();
  }

}

auto Disc::step(u32 clocks) -> void {
  while(clocks) {
    u32 slice = min(clocks, 128 - clockAccumulator);
    u32 sectorPeriod = 451584 >> drive.mode.speed;
    // counter.sector is exact elapsed cycles; a negative value delays the next sector.
    if(counter.sector >= s32(sectorPeriod)) counter.sector %= sectorPeriod;
    slice = min(slice, u32(s32(sectorPeriod) - counter.sector));
    bool seekEvent = drive.seeking != 0;
    if(seekEvent) slice = min(slice, drive.seeking);
    if(drive.speedChange) slice = min(slice, drive.speedChange);
    if(command.active && command.first.counter > 0) slice = min(slice, (u32)command.first.counter);
    if(command.second.pending && command.second.counter > 0) slice = min(slice, (u32)command.second.counter);
    if(drive.sessionChange.pending) slice = min(slice, drive.sessionChange.remaining);
    if(drive.spinUp) slice = min(slice, drive.spinUp);
    if(drive.shellOpening) slice = min(slice, drive.shellOpening);
    if(drive.resetDelay) slice = min(slice, drive.resetDelay);
    if(fifo.deferred.scheduled && fifo.deferred.counter > 0) slice = min(slice, (u32)fifo.deferred.counter);
    if(command.active) {
      command.first.counter -= slice;
      command.elapsed += slice;
    }
    if(command.second.pending) command.second.counter -= slice;
    drive.physical.age += slice;
    if(drive.sessionChange.pending) drive.sessionChange.remaining -= slice;
    if(drive.spinUp) {
      drive.spinUp -= slice;
      if(!drive.spinUp) ssr.motorOn = canReadMedia();
    }
    if(drive.shellOpening) {
      drive.shellOpening -= slice;
      if(!drive.shellOpening && canReadMedia()) drive.spinUp = system.frequency();
    }
    if(drive.resetDelay) {
      drive.resetDelay -= slice;
      if(!drive.resetDelay) drive.setHold(0, 0);
    }
    if(fifo.deferred.scheduled && fifo.deferred.counter > 0) fifo.deferred.counter -= slice;
    irq.acknowledgeAge = min(1000u, irq.acknowledgeAge + slice);
    counter.sector += slice;
    if(seekEvent) drive.seeking -= slice;
    if(drive.speedChange) drive.speedChange -= slice;
    clockAccumulator += slice;
    clocks -= slice;

    if(counter.sector == s32(sectorPeriod)) {
      counter.sector = 0;
      if(!seekEvent) drive.clockSector();
    }
    if(seekEvent && !drive.seeking) drive.finishSeek();
    if(drive.sessionChange.pending && !drive.sessionChange.remaining) {
      drive.sessionChange.pending = false;
      commandSetSession(true);
    }
    if(command.second.pending && command.second.counter <= 0) {
      auto operation = command.second.command;
      command.second = {};
      if(operation == 0x0a) {
        // Repeated Init must not starve the original completion.
        command.first = {};
        command.active = false;
        fifo.parameter.flush();
      }
      executeCommand(operation, true);
    }
    flushDeferredResponse();
    if(command.active && command.first.counter <= 0) {
      auto operation = command.first.command;
      command.first = {};
      command.active = false;
      executeCommand(operation);
      scheduleAsyncResponse();
    }
    if(clockAccumulator == 128) {
      clockAccumulator = 0;
      main();
    }
  }
}

auto Disc::power(bool reset) -> void {
  drive.lba = {};
  drive.physical = {};
  drive.seekInterval = 0;
  for(auto& v : drive.sector.data) v = 0;
  for(auto& v : drive.sector.subq) v = 0;
  drive.mode = {};
  drive.seeking = 0;
  drive.seekVisible = drive.streamStarting = false;
  drive.startingStatus = 0;
  drive.activeStatusCleared = false;
  drive.speedChange = 0;
  // SplitMix64 expansion of reference seed 0x4b435544; xoroshiro128++ advances per seek.
  drive.seekRandom[0] = 0xc004c195a51aa56d;
  drive.seekRandom[1] = 0x6e0f364c0d8dfa2c;
  drive.seekType = Drive::SeekType::None;
  drive.pendingOperation = Drive::PendingOperation::None;
  drive.sessionChange = {};
  drive.spinUp = 0;
  drive.shellOpening = 0;
  drive.resetDelay = 0;
  drive.headerValid = false;
  for(auto& value : drive.header) value = 0;
  audio.frames.flush();
  audio.sample = {};
  audio.mute = audio.muteADPCM = 0;
  //configure for stereo sound at 100% volume level
  audio.volume[0] = audio.volumeLatch[0] = 0x80;
  audio.volume[1] = audio.volumeLatch[1] = 0x00;
  audio.volume[2] = audio.volumeLatch[2] = 0x00;
  audio.volume[3] = audio.volumeLatch[3] = 0x80;
  cdda.playMode = CDDA::PlayMode::Normal;
  cdda.reportDelay = 0;
  cdda.reportFrame = 0xff;
  cdda.scanStep = 0;
  cdda.started = false;
  cdda.playTrack = 0;
  cdda.holdLBA = 0;
  cdda.autoPausePending = false;
  cdxa.filter = {};
  cdxa.current = {};
  for(auto& v : cdxa.previousSamples) v = 0;
  for(auto& channel : cdxa.resampleRing) for(auto& value : channel) value = 0;
  cdxa.resamplePosition = 0;
  cdxa.resampleStep = 6;
  command = {};
  irq = {};
  fifo.parameter.flush();
  fifo.response.flush();
  for(auto& value : fifo.responseLatch) value = 0;
  fifo.responsePosition = 0;
  fifo.data = {};
  for(auto& sector : fifo.sectors) sector = {};
  fifo.sectorRead = fifo.sectorWrite = 0;
  fifo.sectorPending = fifo.hostLoaded = false;
  fifo.sectorFile = fifo.sectorChannel = 0;
  fifo.sectorNeedsFilter = false;
  fifo.deferred = {};
  psr = {};
  ssr = {};
  ssr.motorOn = canReadMedia();
  ssr.shellOpen = lidOpen;
  io = {};
  counter.sector = 0;
  counter.audio = 0;
  clockAccumulator = 0;
}

}
