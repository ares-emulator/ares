auto Disc::status() -> u8 {
  n8 data;
  data.bit(0) = ssr.error;
  data.bit(1) = ssr.motorOn;
  data.bit(2) = ssr.seekError;
  data.bit(3) = ssr.idError;
  data.bit(4) = ssr.shellOpen;
  data.bit(5) = ssr.reading;
  data.bit(6) = drive.seeking != 0 && drive.seekDelay == 0;
  data.bit(7) = ssr.playingCDDA;
  return data;
}

auto Disc::mode() -> u8 {
  n8 mode;
  mode.bit(0) = drive.mode.cdda;
  mode.bit(1) = drive.mode.autoPause;
  mode.bit(2) = drive.mode.report;
  mode.bit(3) = drive.mode.xaFilter;
  mode.bit(4) = drive.mode.ignore;
  mode.bit(5) = drive.mode.sectorSize;
  mode.bit(6) = drive.mode.xaADPCM;
  mode.bit(7) = drive.mode.speed;
  return mode;
}

auto Disc::error(u8 code) -> void {
  ssr.error = 1;
  queueResponse(ResponseType::Error, {status(), code});
}

auto Disc::ack() -> void {
  ssr.error = 0;
  ssr.seekError = 0;
  queueResponse(ResponseType::Acknowledge, {status()});
}

auto Disc::queueResponse(ResponseType type, std::initializer_list<u8> response) -> void {
  if(!irq.pending()) {
    fifo.response.flush();
    for (auto n : response) fifo.response.write(n);

    switch(type) {
    case ResponseType::Ready:       irq.ready.flag       = 1; break;
    case ResponseType::Complete:    irq.complete.flag    = 1; break;
    case ResponseType::Acknowledge: irq.acknowledge.flag = 1; break;
    case ResponseType::End:         irq.end.flag         = 1; break;
    case ResponseType::Error:       irq.error.flag       = 1; break;
    default: break;
    }

    irq.poll();
    return;
  }

  auto defer = [&](FIFO::DeferredData& slot) {
    slot.type = type;
    slot.data.flush();
    for(auto n : response) slot.data.write(n);
  };


  switch(type) {
  case ResponseType::Ready: if(fifo.deferred.ready.type == ResponseType::None) defer(fifo.deferred.ready); break;
  case ResponseType::Complete: defer(fifo.deferred.complete); break;
  case ResponseType::Acknowledge: case ResponseType::End: case ResponseType::Error: defer(fifo.deferred.acknowledge); break;
  default: break;
  }
}

auto Disc::flushDeferredResponse() -> void {
  if(irq.pending()) return;

  auto deliver = [&](FIFO::DeferredData& slot) -> bool {
    if(slot.type == ResponseType::None) return false;

    fifo.response.flush();
    for(u32 n : range(slot.data.size())) fifo.response.write(slot.data.read(0));

    auto type = slot.type;
    slot.type = ResponseType::None;

    switch(type) {
    case ResponseType::Ready:       irq.ready.flag       = 1; break;
    case ResponseType::Complete:    irq.complete.flag    = 1; break;
    case ResponseType::Acknowledge: irq.acknowledge.flag = 1; break;
    case ResponseType::End:         irq.end.flag         = 1; break;
    case ResponseType::Error:       irq.error.flag       = 1; break;
    default: break;
    }

    irq.poll();
    return true;
  };

  if(deliver(fifo.deferred.acknowledge)) return;
  if(deliver(fifo.deferred.complete)) return;
  if(deliver(fifo.deferred.ready)) return;
}

auto Disc::executeCommand(u8 operation) -> void {
  debugger.commandPrologue(operation);

  switch(operation) {
  case 0x00: commandInvalid(); break;  //Sync
  case 0x01: commandNop(); break;
  case 0x02: commandSetLoc(); break;
  case 0x03: commandPlay(); break;
  case 0x04: commandForward(); break;
  case 0x05: commandBackward(); break;
  case 0x06: commandReadN(); break;
  case 0x07: commandStandby(); break;
  case 0x08: commandStop(); break;
  case 0x09: commandPause(); break;
  case 0x0a: commandInit(); break;
  case 0x0b: commandMute(); break;
  case 0x0c: commandDemute(); break;
  case 0x0d: commandSetFilter(); break;
  case 0x0e: commandSetMode(); break;
  case 0x0f: commandGetParam(); break;
  case 0x10: commandGetlocL(); break;
  case 0x11: commandGetlocP(); break;
  case 0x12: commandSetSession(); break;
  case 0x13: commandGetTN(); break;
  case 0x14: commandGetTD(); break;
  case 0x15: commandSeekL(); break;
  case 0x16: commandSeekP(); break;
  case 0x17: commandInvalid(); break;  //SetClock
  case 0x18: commandInvalid(); break;  //GetClock
  case 0x19: commandTest(); break;
  case 0x1a: commandGetID(); break;
  case 0x1b: commandReadS(); break;
  case 0x1e: commandReadToc(); break;
  case range48(0x20, 0x4f): commandInvalid(); break;
  case range8(0x50, 0x57): commandInvalid(); break;  //secret unlock commands
  default:
    if(operation >= 0x58 && operation <= 0xff) commandInvalid();
    else commandUnimplemented(operation);
    break;
  }

  fifo.parameter.flush();
  debugger.commandEpilogue(operation);
}

//0x19
auto Disc::commandTest() -> void {
  u8 operation = 0x19;
  u8 suboperation = fifo.parameter.read(0);
  debugger.commandPrologue(operation, suboperation);

  switch(suboperation) {
  case 0x04: commandTestStartReadSCEX(); break;
  case 0x05: commandTestStopReadSCEX(); break;
  case 0x20: commandTestControllerDate(); break;
  default: commandUnimplemented(operation, suboperation); break;
  }

  debugger.commandEpilogue(operation, suboperation);
}

//0x00
auto Disc::commandInvalid() -> void {
  error(ErrorCode_InvalidCommand);
}

//0x01
auto Disc::commandNop() -> void {
  if(!fifo.parameter.empty()) {
    error(ErrorCode_InvalidParameterCount);
    return;
  }

  ack();

  //when no disc is inserted, act like the drive door is open
  //not entirely accurate, but allows for disc-swapping to work
  //TODO: Expose Open/Close to the GUI?
  ssr.shellOpen = noDisc();
}

//0x02
auto Disc::commandSetLoc() -> void {
  if(fifo.parameter.size() != 3) {
    error(ErrorCode_InvalidParameterCount);
    return;
  }

  for(auto n : range(3)) {
    if(!BCD::valid(fifo.parameter.peek(n))) {
      error(ErrorCode_InvalidParameterValue);
      return;
    }
  }

  u8 minute = BCD::decode(fifo.parameter.read(0));
  u8 second = BCD::decode(fifo.parameter.read(0));
  u8 frame  = BCD::decode(fifo.parameter.read(0));

  if(minute > 99 || second > 59 || frame > 74) {
    error(ErrorCode_InvalidParameterValue);
    return;
  }

  ack();

  drive.lba.request = CD::MSF(minute, second, frame).toLBA();
  drive.lba.pending = 1;
}

//0x03
auto Disc::commandPlay() -> void {
  maybe<u8> trackID;
  if(fifo.parameter.size()) {
    trackID = fifo.parameter.read();
    if(fifo.parameter.size()) {
      error(ErrorCode_InvalidParameterCount);
      return;
    }
  }

  if(trackID && *trackID) {
    if(auto track = session.track(BCD::decode(*trackID))) {
      if(auto index = track->index(1)) {
        drive.lba.request = index->lba;
        drive.lba.pending = 0;
      } else {
        error(ErrorCode_InvalidParameterValue);
        return;
      }
    } else {
      error(ErrorCode_InvalidParameterValue);
      return;
    }

    ssr.reading = 0;
    ssr.playingCDDA = 0;

    drive.seekType = Disc::Drive::SeekType::SeekP;
    drive.pendingOperation = Disc::Drive::PendingOperation::Play;
    drive.seekRetries = 0;
    drive.seeking = drive.distance();
    drive.seekDelay = 3 << drive.mode.speed;

    ack();
    return;
  }

  if(drive.lba.pending) {
    ssr.reading = 0;
    ssr.playingCDDA = 0;

    drive.seekType = Disc::Drive::SeekType::SeekP;
    drive.pendingOperation = Disc::Drive::PendingOperation::Play;
    drive.seekRetries = 0;
    drive.seeking = drive.distance();
    drive.seekDelay = 3 << drive.mode.speed;
    drive.lba.pending = 0;

    ack();
    return;
  }

  ssr.reading = 1;
  ssr.playingCDDA = 1;
  cdda.playMode = CDDA::PlayMode::Normal;
  counter.report = system.frequency() / 75;

  ack();
}


//0x04
auto Disc::commandForward() -> void {
  cdda.playMode = CDDA::PlayMode::FastForward;

  ack();
}

//0x05
auto Disc::commandBackward() -> void {
  cdda.playMode = CDDA::PlayMode::Rewind;

  ack();
}

//0x06
auto Disc::commandReadN() -> void {
  if(drive.lba.pending) {
    ssr.reading = 0;
    ssr.playingCDDA = 0;

    drive.seekType = Drive::SeekType::SeekL;
    drive.pendingOperation = Drive::PendingOperation::Read;
    drive.seekRetries = 0;
    drive.seeking = drive.distance();
    drive.seekDelay = 3 << drive.mode.speed;
    drive.lba.pending = 0;

    ack();
    return;
  }

  ssr.reading = 1;
  ssr.playingCDDA = 0;
  ack();
}

//0x07
auto Disc::commandStandby() -> void {
  if(command.current.invocation == 0) {
    command.current.invocation = 1;
    command.current.counter = 50'000;

    ack();
    return;
  }

  if(command.current.invocation == 1) {
    queueResponse(ResponseType::Complete, {status()});
    command.current.invocation = 0;
    return;
  }
}

//0x08
auto Disc::commandStop() -> void {
  if(command.current.invocation == 0) {
    command.current.invocation = 1;
    command.current.counter = 50'000;

    ack();
    return;
  }

  if(command.current.invocation == 1) {
    ssr.playingCDDA = 0;

    queueResponse(ResponseType::Complete, {status()});
    command.current.invocation = 0;
    return;
  }
}

//0x09
auto Disc::commandPause() -> void {
  if(command.current.invocation == 0) {
    command.current.invocation = 1;
    command.current.counter = system.frequency() * (drive.mode.speed ? 35 : 70) / 1000;
    ack();
    ssr.reading = 0;
    return;
  }

  if(command.current.invocation == 1) {
    queueResponse(ResponseType::Complete, {status()});
    command.current.invocation = 0;
    return;
  }
}

//0x0a
auto Disc::commandInit() -> void {
  if(command.current.invocation == 0) {
    if(!fifo.parameter.empty()) {
      error(ErrorCode_InvalidParameterCount);
      return;
    }

    command.current.invocation = 1;
    command.current.counter = 4'000'000;  // ~118ms

    drive.mode.cdda       = 0;
    drive.mode.autoPause  = 0;
    drive.mode.report     = 0;
    drive.mode.xaFilter   = 0;
    drive.mode.ignore     = 0;
    drive.mode.sectorSize = 0;
    drive.mode.xaADPCM    = 0;
    drive.mode.speed      = 0;
    drive.recentlyReset   = 75 << drive.mode.speed;
    drive.lba = {};

    ssr.motorOn = 1;
    ssr.reading = 0;
    ssr.shellOpen = noDisc();
    ack();
    return;
  }

  if(command.current.invocation == 1) {
    queueResponse(ResponseType::Complete, {status()});
    command.current.invocation = 0;
    return;
  }
}

//0x0b
auto Disc::commandMute() -> void {
  audio.mute = 1;

  ack();
}

//0x0c
auto Disc::commandDemute() -> void {
  audio.mute = 0;

  ack();
}

//0x0d
auto Disc::commandSetFilter() -> void {
  cdxa.filter.file = fifo.parameter.read(0);
  cdxa.filter.channel = fifo.parameter.read(0);

  ack();
}

//0x0e
auto Disc::commandSetMode() -> void {
  if(fifo.parameter.size() != 1) {
    error(ErrorCode_InvalidParameterCount);
    return;
  }

  n8 data = fifo.parameter.read(0);

  if(data.bit(7) != drive.mode.speed) {
    drive.seekDelay = 80 << drive.mode.speed;
  }

  drive.mode.cdda       = data.bit(0);
  drive.mode.autoPause  = data.bit(1);
  drive.mode.report     = data.bit(2);
  drive.mode.xaFilter   = data.bit(3);
  drive.mode.ignore     = data.bit(4);
  drive.mode.sectorSize = data.bit(5);
  drive.mode.xaADPCM    = data.bit(6);
  drive.mode.speed      = data.bit(7);

  ack();
}

//0x0f
auto Disc::commandGetParam() -> void {
  queueResponse(ResponseType::Acknowledge, {
    status(),
    mode(),
    0,
    cdxa.filter.file,
    cdxa.filter.channel
  });
}

//0x10
auto Disc::commandGetlocL() -> void {
  if(!fifo.parameter.empty()) {
    error(ErrorCode_InvalidParameterCount);
    return;
  }

  if(!drive.recentlyReset && (!ssr.reading || ssr.playingCDDA)) {
    error(ErrorCode_CannotRespondYet);
    return;
  }

  queueResponse(ResponseType::Acknowledge, {
    drive.sector.data[12 + 0],
    drive.sector.data[12 + 1],
    drive.sector.data[12 + 2],
    drive.sector.data[12 + 3],
    drive.sector.data[12 + 4],
    drive.sector.data[12 + 5],
    drive.sector.data[12 + 6],
    drive.sector.data[12 + 7],
  });
}

//0x11
auto Disc::commandGetlocP() -> void {
  queueResponse(ResponseType::Acknowledge, {
    drive.sector.subq[1], //track
    drive.sector.subq[2], //index
    drive.sector.subq[3], //mm
    drive.sector.subq[4], //ss
    drive.sector.subq[5], //sect/frame
    drive.sector.subq[7], //amm
    drive.sector.subq[8], //ass
    drive.sector.subq[9], //asect/frame
  });
}

//0x12
auto Disc::commandSetSession() -> void {
  if(command.current.invocation == 0) {
    command.current.invocation = 1;
    command.current.counter = 50'000;

    u8 session = fifo.parameter.read(0);
    if(session != 1) {
      debug(unimplemented, "Disc::commandSetSession(): session != 1");
    }

    ack();
    return;
  }

  if(command.current.invocation == 1) {
    ack();
    command.current.invocation = 0;
    return;
  }
}

//0x13
auto Disc::commandGetTN() -> void {
  if(!fifo.parameter.empty()) {
    error(ErrorCode_InvalidParameterCount);
    return;
  }

  queueResponse(ResponseType::Acknowledge, {
    status(),
    BCD::encode(session.firstTrack),
    BCD::encode(session.lastTrack)
  });
}

//0x14
auto Disc::commandGetTD() -> void {
  if(fifo.parameter.size() != 1) {
    error(ErrorCode_InvalidParameterCount);
    return;
  }

  if(!BCD::valid(fifo.parameter.peek(0))) {
    error(ErrorCode_InvalidParameterValue);
    return;
  }

  u8 trackID = BCD::decode(fifo.parameter.read(0));
  auto track = trackID ? session.track(trackID) : session.track(session.lastTrack);
  if(!track) { error(ErrorCode_InvalidParameterValue); return; }

  auto index = track->index(1);
  if(!index) { error(ErrorCode_InvalidParameterValue); return; }

  s32 lba = trackID ? index->lba : index->end;
  auto msf = CD::MSF::fromABA(CD::LBAtoABA(lba));

  queueResponse(ResponseType::Acknowledge, {
    status(),
    BCD::encode(msf.minute),
    BCD::encode(msf.second)
  });
}

//0x15
auto Disc::commandSeekL() -> void {
  if(!fifo.parameter.empty()) { error(ErrorCode_InvalidParameterCount); return; }

  ack();

  ssr.reading = 0;
  ssr.playingCDDA = 0;

  drive.seekType = Drive::SeekType::SeekL;
  drive.pendingOperation = Drive::PendingOperation::None;
  drive.seekRetries = 0;
  drive.seeking = drive.distance();
  drive.seekDelay = 3 << drive.mode.speed;
  drive.lba.pending = 0;
}

//0x16
auto Disc::commandSeekP() -> void {
  if(!fifo.parameter.empty()) { error(ErrorCode_InvalidParameterCount); return; }

  ack();

  ssr.reading = 0;
  ssr.playingCDDA = 0;

  drive.seekType = Drive::SeekType::SeekP;
  drive.pendingOperation = Drive::PendingOperation::None;
  drive.seekRetries = 0;
  drive.seeking = drive.distance();
  drive.seekDelay = 3 << drive.mode.speed;
  drive.lba.pending = 0;
}

//0x19 0x04
auto Disc::commandTestStartReadSCEX() -> void {
  ack();
}

//0x19 0x05
auto Disc::commandTestStopReadSCEX() -> void {
  // Report no SCEX string found to appease mod-chip detection
  queueResponse(ResponseType::Acknowledge, {0, 0});
}

//0x19 0x20
auto Disc::commandTestControllerDate() -> void {
  queueResponse(ResponseType::Acknowledge, {0x95, 0x05, 0x16, 0xC1 });
}

//0x1a
auto Disc::commandGetID() -> void {
  if(command.current.invocation == 0) {
    if(!fifo.parameter.empty()) {
      error(ErrorCode_InvalidParameterCount);
      return;
    }

    command.current.invocation = 1;
    command.current.counter = 50'000;

    ack();
    return;
  }

  if(command.current.invocation == 1) {
    if(noDisc()) {
      ssr.idError = 1;
      queueResponse(ResponseType::Error, {0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00});
    } else if(region().find("NTSC-J") && Region::NTSCJ()) {
      ssr.idError = 0;
      queueResponse(ResponseType::Complete, {status(), 0x00, 0x20,0x00, 'S', 'C', 'E', 'I'});
    } else if(region().find("NTSC-U") && Region::NTSCU()) {
      ssr.idError = 0;
      queueResponse(ResponseType::Complete, {status(), 0x00, 0x20,0x00, 'S', 'C', 'E', 'A'});
    } else if(region().find("PAL") && Region::PAL()) {
      ssr.idError = 0;
      queueResponse(ResponseType::Complete, {status(), 0x00, 0x20,0x00, 'S', 'C', 'E', 'E'});
    } else if(audioCD()) {
      ssr.idError = 1;
      queueResponse(ResponseType::Error, {0x90, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00});
    } else {
      //invalid disc inserted
      ssr.idError = 1;
      queueResponse(ResponseType::Error, {0x80, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00});
    }

    command.current.invocation = 0;
    return;
  }
}

//0x1b
auto Disc::commandReadS() -> void {
  //retries will never occur under emulation
  return commandReadN();
}

//0x1e
auto Disc::commandReadToc() -> void {
  if(command.current.invocation == 0) {
    command.current.invocation = 1;
    command.current.counter = 475'000;

    ack();
    return;
  }

  if(command.current.invocation == 1) {
    ack();
    command.current.invocation = 0;
    return;
  }
}

auto Disc::commandUnimplemented(u8 operation, maybe<u8> suboperation) -> void {
  if(!suboperation) {
    debug(unimplemented, "Disc::command($", hex(operation, 2L), ")");
  } else {
    debug(unimplemented, "Disc::command($", hex(operation, 2L), ", $", hex(*suboperation, 2L));
  }
}
