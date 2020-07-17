auto Disc::status() -> u8 {
  uint8 data;
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

auto Disc::command(u8 operation) -> void {
//print("* CDC ", hex(operation, 2L), "\n");

  switch(operation) {
  case 0x01: return commandGetStatus();
  case 0x02: return commandSetLocation();
  case 0x03: return commandPlay();
  case 0x04: return commandFastForward();
  case 0x05: return commandRewind();
  case 0x06: return commandReadWithRetry();
  case 0x07: return commandReadWithoutRetry();
  case 0x08: return commandStop();
  case 0x09: return commandPause();
  case 0x0a: return commandInitialize();
  case 0x0b: return commandMute();
  case 0x0c: return commandUnmute();
  case 0x0e: return commandSetMode();
  case 0x13: return commandGetFirstAndLastTrackNumbers();
  case 0x14: return commandGetTrackStart();
  case 0x15: return commandSeekData();
  case 0x16: return commandSeekCDDA();
  case 0x19: return commandTest();
  case 0x1a: return commandGetID();
  }

//print("* CDC ", hex(operation, 2L), "\n");
}

auto Disc::commandTest() -> void {
  u8 operation = fifo.parameter.read();
//print("* CDC 19 ", hex(operation, 2L), "\n");

  switch(operation) {
  case 0x20: return commandTestControllerDate();
  }

//print("* CDC 19 ", hex(operation, 2L), "\n");
}

//0x01
auto Disc::commandGetStatus() -> void {
  fifo.response.flush();
  fifo.response.write(status());
  ssr.shellOpen = 0;

  irq.acknowledge.flag = 1;
  irq.poll();
}

//0x02
auto Disc::commandSetLocation() -> void {
  u8 minute = CD::BCD::decode(fifo.parameter.read());
  u8 second = CD::BCD::decode(fifo.parameter.read());
  u8 frame  = CD::BCD::decode(fifo.parameter.read());

  drive.lba.request = CD::MSF(minute, second, frame).toLBA();

  fifo.response.flush();
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

  if(trackID) {
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

  fifo.response.flush();
  fifo.response.write(status());

  irq.acknowledge.flag = 1;
  irq.poll();
}

//0x04
auto Disc::commandFastForward() -> void {
  cdda.playMode = CDDA::PlayMode::FastForward;

  fifo.response.flush();
  fifo.response.write(status());

  irq.acknowledge.flag = 1;
  irq.poll();
}

//0x05
auto Disc::commandRewind() -> void {
  cdda.playMode = CDDA::PlayMode::Rewind;

  fifo.response.flush();
  fifo.response.write(status());

  irq.acknowledge.flag = 1;
  irq.poll();
}

//0x06
auto Disc::commandReadWithRetry() -> void {
  drive.lba.current = drive.lba.request;
  ssr.reading = 1;

  fifo.response.flush();
  fifo.response.write(status());

  irq.acknowledge.flag = 1;
  irq.poll();
}

//0x07
auto Disc::commandReadWithoutRetry() -> void {
  //retries will never occur under emulation
  return commandReadWithRetry();
}

//0x08
auto Disc::commandStop() -> void {
  if(event.invocation == 0) {
    event.invocation = 1;
    event.counter = 25000;

    fifo.response.flush();
    fifo.response.write(status());

    irq.acknowledge.flag = 1;
    irq.poll();
    return;
  }

  if(event.invocation == 1) {
    ssr.playingCDDA = 0;

    fifo.response.flush();
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
    event.counter = 25000;

    fifo.response.flush();
    fifo.response.write(status());

    irq.acknowledge.flag = 1;
    irq.poll();
    return;
  }

  if(event.invocation == 1) {
    ssr.reading = 0;

    fifo.response.flush();
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
    event.counter = 25000;

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

    fifo.response.flush();
    fifo.response.write(status());

    irq.acknowledge.flag = 1;
    irq.poll();
    return;
  }

  if(event.invocation == 1) {
    fifo.response.flush();
    fifo.response.write(status());

    irq.complete.flag = 1;
    irq.poll();
    return;
  }
}

//0x0b
auto Disc::commandMute() -> void {
  //todo

  fifo.response.flush();
  fifo.response.write(status());

  irq.acknowledge.flag = 1;
  irq.poll();
}

//0x0c
auto Disc::commandUnmute() -> void {
  //todo

  fifo.response.flush();
  fifo.response.write(status());

  irq.acknowledge.flag = 1;
  irq.poll();
}

//0x0e
auto Disc::commandSetMode() -> void {
  uint8 data = fifo.parameter.read();
  drive.mode.cdda       = data.bit(0);
  drive.mode.autoPause  = data.bit(1);
  drive.mode.report     = data.bit(2);
  drive.mode.xaFilter   = data.bit(3);
  drive.mode.ignore     = data.bit(4);
  drive.mode.sectorSize = data.bit(5);
  drive.mode.xaADPCM    = data.bit(6);
  drive.mode.speed      = data.bit(7);

  fifo.response.flush();
  fifo.response.write(status());

  irq.acknowledge.flag = 1;
  irq.poll();
}

//0x13
auto Disc::commandGetFirstAndLastTrackNumbers() -> void {
  fifo.response.flush();
  fifo.response.write(status());
  fifo.response.write(CD::BCD::encode(session.firstTrack));
  fifo.response.write(CD::BCD::encode(session.lastTrack));

  irq.acknowledge.flag = 1;
  irq.poll();
}

//0x14
auto Disc::commandGetTrackStart() -> void {
  u8 trackID = 0;
  if(fifo.parameter.size()) {
    trackID = fifo.parameter.read();
  }

  int lba = 0;
  if(!trackID) {
    lba = session.leadOut.lba;
  } else if(auto track = session.track(trackID)) {
    if(auto index = track->index(1)) {
      lba = index->lba;
    }
  }
  auto [minute, second, frame] = CD::MSF::fromLBA(150 + lba);

  fifo.response.flush();
  fifo.response.write(status());
  fifo.response.write(CD::BCD::encode(minute));
  fifo.response.write(CD::BCD::encode(second));

  irq.acknowledge.flag = 1;
  irq.poll();
}

//0x15
auto Disc::commandSeekData() -> void {
  drive.lba.current = drive.lba.request;

  fifo.response.flush();
  fifo.response.write(status());

  irq.complete.flag = 1;
  irq.poll();
}

//0x16
auto Disc::commandSeekCDDA() -> void {
  drive.lba.current = drive.lba.request;
  ssr.playingCDDA = 0;

  fifo.response.flush();
  fifo.response.write(status());

  irq.complete.flag = 1;
  irq.poll();
}

//0x19 0x20
auto Disc::commandTestControllerDate() -> void {
  fifo.response.flush();
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
    event.counter = 25000;

    fifo.response.flush();
    fifo.response.write(status());

    irq.acknowledge.flag = 1;
    irq.poll();
    return;
  }

  if(event.invocation == 1) {
    if(region() == "NTSC-J") {
      ssr.idError = 0;

      fifo.response.flush();
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
    } else if(region() == "NTSC-U") {
      ssr.idError = 0;

      fifo.response.flush();
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
    } else if(region() == "PAL") {
      ssr.idError = 0;

      fifo.response.flush();
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
    } else {
      ssr.idError = 1;

      fifo.response.flush();
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
    }

    return;
  }
}
