auto Disc::status() -> u8 {
  n8 data;
  data.bit(0) = ssr.error;
  data.bit(1) = ssr.motorOn;
  data.bit(2) = ssr.seekError;
  data.bit(3) = ssr.idError;
  data.bit(4) = ssr.shellOpen;
  data.bit(5) = ssr.reading;
  data.bit(6) = ssr.seeking;
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

auto Disc::command(u8 operation) -> void {
  fifo.response.flush();
  debugger.commandPrologue(operation);

  switch(operation) {
  case 0x00: commandInvalid(); break;  //Sync
  case 0x01: commandGetStatus(); break;
  case 0x02: commandSetLocation(); break;
  case 0x03: commandPlay(); break;
  case 0x04: commandFastForward(); break;
  case 0x05: commandRewind(); break;
  case 0x06: commandReadWithRetry(); break;
  case 0x07: commandMotorOn(); break;
  case 0x08: commandStop(); break;
  case 0x09: commandPause(); break;
  case 0x0a: commandInitialize(); break;
  case 0x0b: commandMute(); break;
  case 0x0c: commandUnmute(); break;
  case 0x0d: commandSetFilter(); break;
  case 0x0e: commandSetMode(); break;
  case 0x0f: commandGetParam(); break;
  case 0x10: commandGetLocationReading(); break;
  case 0x11: commandGetLocationPlaying(); break;
  case 0x12: commandSetSession(); break;
  case 0x13: commandGetFirstAndLastTrackNumbers(); break;
  case 0x14: commandGetTrackStart(); break;
  case 0x15: commandSeekData(); break;
  case 0x16: commandSeekCDDA(); break;
  case 0x17: commandInvalid(); break;  //SetClock
  case 0x18: commandInvalid(); break;  //GetClock
  case 0x19: commandTest(); break;
  case 0x1a: commandGetID(); break;
  case 0x1b: commandReadWithoutRetry(); break;
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
  fifo.response.write(0x11);
  fifo.response.write(0x40);

  irq.error.flag = 1;
  irq.poll();
}

//0x01
auto Disc::commandGetStatus() -> void {
  fifo.response.write(status());

  //when no disc is inserted, act like the drive door is open
  //not entirely accurate, but allows for disc-swapping to work
  //TODO: Expose Open/Close to the GUI?
  ssr.shellOpen = noDisc();

  irq.acknowledge.flag = 1;
  irq.poll();
}

//0x02
auto Disc::commandSetLocation() -> void {
  u8 minute = BCD::decode(fifo.parameter.read(0));
  u8 second = BCD::decode(fifo.parameter.read(0));
  u8 frame  = BCD::decode(fifo.parameter.read(0));

  drive.lba.request = CD::MSF(minute, second, frame).toLBA();

  fifo.response.write(status());

  irq.acknowledge.flag = 1;
  irq.poll();
}

//0x03
auto Disc::commandPlay() -> void {
  maybe<u8> trackID;
  if(fifo.parameter.size()) {
    trackID = fifo.parameter.read();
  }

  //uses SetLocation address unless a valid track# is provided
  drive.lba.current = drive.lba.request;

  if(trackID && *trackID) {
    if(auto track = session.track(*trackID)) {
      if(auto index = track->index(1)) {
        drive.lba.current = index->lba;
      }
    }
  }

  ssr.reading = 1;
  ssr.playingCDDA = 1;
  cdda.playMode = CDDA::PlayMode::Normal;
  counter.report = 33'868'800 / 75;

  fifo.response.write(status());

  irq.acknowledge.flag = 1;
  irq.poll();
}

//0x04
auto Disc::commandFastForward() -> void {
  cdda.playMode = CDDA::PlayMode::FastForward;

  fifo.response.write(status());

  irq.acknowledge.flag = 1;
  irq.poll();
}

//0x05
auto Disc::commandRewind() -> void {
  cdda.playMode = CDDA::PlayMode::Rewind;

  fifo.response.write(status());

  irq.acknowledge.flag = 1;
  irq.poll();
}

//0x06
auto Disc::commandReadWithRetry() -> void {
  drive.seeking = 2 << drive.mode.speed;
  drive.lba.current = drive.lba.request;
  ssr.reading = 1;
  ssr.playingCDDA = 0;
  fifo.response.write(status());

  irq.acknowledge.flag = 1;
  irq.poll();
}

//0x07
auto Disc::commandMotorOn() -> void {
  if(event.invocation == 0) {
    event.invocation = 1;
    event.counter = 50'000;

    fifo.response.write(status());

    irq.acknowledge.flag = 1;
    irq.poll();
    return;
  }

  if(event.invocation == 1) {
    fifo.response.write(status());
    irq.complete.flag = 1;
    irq.poll();
    return;
  }
}

//0x08
auto Disc::commandStop() -> void {
  if(event.invocation == 0) {
    event.invocation = 1;
    event.counter = 50'000;

    fifo.response.write(status());

    irq.acknowledge.flag = 1;
    irq.poll();
    return;
  }

  if(event.invocation == 1) {
    ssr.playingCDDA = 0;

    fifo.response.write(status());

    irq.complete.flag = 1;
    irq.poll();
    return;
  }
}

//0x09
auto Disc::commandPause() -> void {
  if(event.invocation == 0) {
    event.invocation = 1;
    event.counter = 50'000;

    ssr.reading = 0;

    fifo.response.write(status());

    irq.acknowledge.flag = 1;
    irq.poll();
    return;
  }

  if(event.invocation == 1) {
    fifo.response.write(status());

    irq.complete.flag = 1;
    irq.poll();
    return;
  }
}

//0x0a
auto Disc::commandInitialize() -> void {
  if(event.invocation == 0) {
    event.invocation = 1;
    event.counter = 475'000;

    drive.mode.cdda       = 0;
    drive.mode.autoPause  = 0;
    drive.mode.report     = 0;
    drive.mode.xaFilter   = 0;
    drive.mode.ignore     = 0;
    drive.mode.sectorSize = 0;
    drive.mode.xaADPCM    = 0;
    drive.mode.speed      = 0;

    ssr.motorOn = 1;
    ssr.reading = 0;
    ssr.seeking = 0;

    fifo.response.write(status());

    irq.acknowledge.flag = 1;
    irq.poll();
    return;
  }

  if(event.invocation == 1) {
    fifo.response.write(status());

    irq.complete.flag = 1;
    irq.poll();
    return;
  }
}

//0x0b
auto Disc::commandMute() -> void {
  audio.mute = 1;

  fifo.response.write(status());

  irq.acknowledge.flag = 1;
  irq.poll();
}

//0x0c
auto Disc::commandUnmute() -> void {
  audio.mute = 0;

  fifo.response.write(status());

  irq.acknowledge.flag = 1;
  irq.poll();
}

//0x0d
auto Disc::commandSetFilter() -> void {
  cdxa.filter.file = fifo.parameter.read(0);
  cdxa.filter.channel = fifo.parameter.read(0);

  fifo.response.write(status());

  irq.acknowledge.flag = 1;
  irq.poll();
}

//0x0e
auto Disc::commandSetMode() -> void {
  n8 data = fifo.parameter.read(0);
  drive.mode.cdda       = data.bit(0);
  drive.mode.autoPause  = data.bit(1);
  drive.mode.report     = data.bit(2);
  drive.mode.xaFilter   = data.bit(3);
  drive.mode.ignore     = data.bit(4);
  drive.mode.sectorSize = data.bit(5);
  drive.mode.xaADPCM    = data.bit(6);
  drive.mode.speed      = data.bit(7);

  fifo.response.write(status());

  irq.acknowledge.flag = 1;
  irq.poll();
}

//0x0f
auto Disc::commandGetParam() -> void {
  fifo.response.write(status());
  fifo.response.write(mode());
  fifo.response.write(0);
  fifo.response.write(cdxa.filter.file);
  fifo.response.write(cdxa.filter.channel);

  irq.acknowledge.flag = 1;
  irq.poll();
}

//0x10
auto Disc::commandGetLocationReading() -> void {
  for(auto offset : range(8)) {
    fifo.response.write(drive.sector.data[12 + offset]);
  }

  irq.acknowledge.flag = 1;
  irq.poll();
}

//0x11
auto Disc::commandGetLocationPlaying() -> void {
  s32 lba = drive.lba.current;
  s32 lbaTrack = 0;
  s32 lbaTrackID = 0;
  s32 lbaIndexID = 0;
  if(auto trackID = session.inTrack(lba)) {
    lbaTrackID = *trackID;
    if(auto track = session.track(*trackID)) {
      if(auto indexID = track->inIndex(lba)) {
        lbaIndexID = *indexID;
      }
      if(auto index = track->index(1)) {
        lbaTrack = index->lba;
      }
    }
  }
  auto [relativeMinute, relativeSecond, relativeFrame] = CD::MSF::fromLBA(lba - lbaTrack);
  auto [absoluteMinute, absoluteSecond, absoluteFrame] = CD::MSF::fromLBA(lba);

  fifo.response.write(lbaTrackID);
  fifo.response.write(lbaIndexID);
  fifo.response.write(BCD::encode(relativeMinute));
  fifo.response.write(BCD::encode(relativeSecond));
  fifo.response.write(BCD::encode(relativeFrame));
  fifo.response.write(BCD::encode(absoluteMinute));
  fifo.response.write(BCD::encode(absoluteSecond));
  fifo.response.write(BCD::encode(absoluteFrame));

  irq.acknowledge.flag = 1;
  irq.poll();
}

//0x12
auto Disc::commandSetSession() -> void {
  if(event.invocation == 0) {
    event.invocation = 1;
    event.counter = 50'000;

    u8 session = fifo.parameter.read(0);
    if(session != 1) {
      debug(unimplemented, "Disc::commandSetSession(): session != 1");
    }

    ssr.seeking = 1;
    fifo.response.write(status());

    irq.acknowledge.flag = 1;
    irq.poll();
    return;
  }

  if(event.invocation == 1) {
    ssr.seeking = 0;
    fifo.response.write(status());

    irq.complete.flag = 1;
    irq.poll();
    return;
  }
}

//0x13
auto Disc::commandGetFirstAndLastTrackNumbers() -> void {
  fifo.response.write(status());
  fifo.response.write(BCD::encode(session.firstTrack));
  fifo.response.write(BCD::encode(session.lastTrack));

  irq.acknowledge.flag = 1;
  irq.poll();
}

//0x14
auto Disc::commandGetTrackStart() -> void {
  u8 trackID = fifo.parameter.read(0);

  s32 lba = 0;
  if(!trackID) {
    lba = session.leadOut.lba;
  } else if(auto track = session.track(trackID)) {
    if(auto index = track->index(1)) {
      lba = index->lba;
    }
  }
  auto [minute, second, frame] = CD::MSF::fromLBA(150 + lba);

  fifo.response.write(status());
  fifo.response.write(BCD::encode(minute));
  fifo.response.write(BCD::encode(second));

  irq.acknowledge.flag = 1;
  irq.poll();
}

//0x15
auto Disc::commandSeekData() -> void {
  if(event.invocation == 0) {
    event.invocation = 1;
    event.counter = 50'000;     // TODO: Calculate seek time

    fifo.response.write(status());

    irq.acknowledge.flag = 1;
    irq.poll();
    return;
  }

  if(event.invocation == 1) {
    drive.lba.current = drive.lba.request;
    ssr.playingCDDA = 0;
    ssr.reading = 0;

    fifo.response.write(status());

    irq.complete.flag = 1;
    irq.poll();

    return;
  }
}

//0x16
auto Disc::commandSeekCDDA() -> void {
  // TODO: use subchannel data to seek rather than LBA
  commandSeekData();
}

//0x19 0x04
auto Disc::commandTestStartReadSCEX() -> void {
  fifo.response.write(status());
  irq.acknowledge.flag = 1;
  irq.poll();
}

//0x19 0x05
auto Disc::commandTestStopReadSCEX() -> void {
  // Report no SCEX string found to appease mod-chip detection
  fifo.response.write(0);
  fifo.response.write(0);
  irq.acknowledge.flag = 1;
  irq.poll();
}

//0x19 0x20
auto Disc::commandTestControllerDate() -> void {
  fifo.response.write(0x95);
  fifo.response.write(0x05);
  fifo.response.write(0x16);
  fifo.response.write(0xc1);

  irq.acknowledge.flag = 1;
  irq.poll();
}

//0x1a
auto Disc::commandGetID() -> void {
  if(event.invocation == 0) {
    event.invocation = 1;
    event.counter = 50'000;

    fifo.response.write(status());

    irq.acknowledge.flag = 1;
    irq.poll();
    return;
  }

  if(event.invocation == 1) {
    if(noDisc()) {
      ssr.idError = 1;

      fifo.response.write(0x40);
      fifo.response.write(0x00);
      fifo.response.write(0x00);
      fifo.response.write(0x00);
      fifo.response.write(0x00);
      fifo.response.write(0x00);
      fifo.response.write(0x00);
      fifo.response.write(0x00);

      irq.error.flag = 1;
      irq.poll();
    } else if(region().find("NTSC-J") && Region::NTSCJ()) {
      ssr.idError = 0;

      fifo.response.write(status());
      fifo.response.write(0x00);
      fifo.response.write(0x20);
      fifo.response.write(0x00);
      fifo.response.write('S');
      fifo.response.write('C');
      fifo.response.write('E');
      fifo.response.write('I');

      irq.complete.flag = 1;
      irq.poll();
    } else if(region().find("NTSC-U") && Region::NTSCU()) {
      ssr.idError = 0;

      fifo.response.write(status());
      fifo.response.write(0x00);
      fifo.response.write(0x20);
      fifo.response.write(0x00);
      fifo.response.write('S');
      fifo.response.write('C');
      fifo.response.write('E');
      fifo.response.write('A');

      irq.complete.flag = 1;
      irq.poll();
    } else if(region().find("PAL") && Region::PAL()) {
      ssr.idError = 0;

      fifo.response.write(status());
      fifo.response.write(0x00);
      fifo.response.write(0x20);
      fifo.response.write(0x00);
      fifo.response.write('S');
      fifo.response.write('C');
      fifo.response.write('E');
      fifo.response.write('E');

      irq.complete.flag = 1;
      irq.poll();
    } else if(audioCD()) {
      ssr.idError = 1;

      fifo.response.write(status());
      fifo.response.write(0x90);
      fifo.response.write(0x00);
      fifo.response.write(0x00);
      fifo.response.write(0x00);
      fifo.response.write(0x00);
      fifo.response.write(0x00);
      fifo.response.write(0x00);

      irq.error.flag = 1;
      irq.poll();
    } else {
      //invalid disc inserted
      ssr.idError = 1;

      fifo.response.write(status());
      fifo.response.write(0x80);
      fifo.response.write(0x20);
      fifo.response.write(0x00);
      fifo.response.write(0x00);
      fifo.response.write(0x00);
      fifo.response.write(0x00);
      fifo.response.write(0x00);

      irq.error.flag = 1;
      irq.poll();
    }

    return;
  }
}

//0x1b
auto Disc::commandReadWithoutRetry() -> void {
  //retries will never occur under emulation
  return commandReadWithRetry();
}

//0x1e
auto Disc::commandReadToc() -> void {
  if(event.invocation == 0) {
    event.invocation = 1;
    event.counter = 475'000;

    fifo.response.write(status());

    irq.acknowledge.flag = 1;
    irq.poll();
    return;
  }

  if(event.invocation == 1) {
    fifo.response.write(status());

    irq.complete.flag = 1;
    irq.poll();
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
