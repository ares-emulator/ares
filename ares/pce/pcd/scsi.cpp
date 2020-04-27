auto PCD::SCSI::clock() -> void {
  if(acknowledging && !--acknowledging) {
    pin.acknowledge = 0;
    update();
    if(pin.control) adpcm->dmaActive = 0;
  }
}

auto PCD::SCSI::clockSector() -> void {
  if(!drive->inData()) return;
  if(!response.end()) return;
  if(!drive->read()) return;

  response.reset();
  for(uint offset : range(2048)) {
    response.write(drive->sector[16 + offset]);
  }

  pin.control = 0;
  pin.input = 1;
  dataTransferCompleted = drive->inactive();
}

auto PCD::SCSI::clockSample() -> maybe<uint8> {
  if(pin.request && !pin.acknowledge && !pin.control && pin.input) {
    return readData();
  }
  return {};
}

auto PCD::SCSI::readData() -> uint8 {
  uint8 result = bus.data;

  if(pin.request && !pin.acknowledge && !pin.control && pin.input) {
    pin.acknowledge = 1;
    update();
    acknowledging = 20;
  }

  return result;
}

auto PCD::SCSI::update() -> void {
  if(pin.reset) {
    drive->setInactive();
    irq.ready.lower();
    irq.completed.lower();
    pin.busy = 0;
    pin.request = 0;
    pin.message = 0;
    pin.control = 0;
    pin.input = 0;
    pin.select = 0;
    bus.select = 0;
    return;
  }

  //release the bus when no longer requesting selection
  if(!pin.busy && !pin.select && bus.select) {
    bus.select = 0;
    pin.request = 0;
    pin.message = 0;
    pin.control = 0;
    pin.input = 0;
    irq.completed.lower();
  }

  //acquire the bus when requesting selection
  if(!bus.select && pin.select) {
    bus.select = 1;
    pin.busy = 1;
    pin.request = 1;
    pin.message = 0;
    pin.control = 1;
    pin.input = 0;
  }

  if(pin.busy) {
    if(pin.message) {
      if(pin.input) {
        messageInput();
      } else {
        messageOutput();
      }
    } else {
      if(pin.input) {
        dataInput();
      } else {
        dataOutput();
      }
    }
  }
}

//SCSI -> CPU
auto PCD::SCSI::messageInput() -> void {
  if(pin.request && pin.acknowledge) {
    pin.request = 0;
    messageSent = 1;
  }

  if(!pin.request && !pin.acknowledge && messageSent) {
    pin.busy = 0;
    messageSent = 0;
  }
}

//CPU -> SCSI
auto PCD::SCSI::messageOutput() -> void {
  if(pin.request && pin.acknowledge) {
    pin.request = 0;
  }
}

//SCSI -> CPU
auto PCD::SCSI::dataInput() -> void {
  if(pin.control) {
    if(pin.request && pin.acknowledge) {
      pin.request = 0;
      statusSent = 1;
    }

    if(!pin.request && !pin.acknowledge && statusSent) {
      statusSent = 0;
      if(messageAfterStatus) {
        messageAfterStatus = 0;
        pin.request = 1;
        pin.message = 1;
        bus.data = 0x00;
      }
    }
  } else {
    if(pin.request && pin.acknowledge) {
      pin.request = 0;
    }

    if(!pin.request && !pin.acknowledge) {
      if(!response.end()) {
        bus.data = response.read();
        pin.request = 1;
      } else {
        irq.ready.lower();
        if(dataTransferCompleted) {
          dataTransferCompleted = 0;
          irq.completed.raise();
          reply(Status::OK);
        }
      }
    }
  }
}

//CPU -> SCSI
auto PCD::SCSI::dataOutput() -> void {
  if(pin.request && pin.acknowledge) {
    pin.request = 0;
    request.write(bus.data);
  }

  if(!pin.request && !pin.acknowledge && request.writes) {
    auto command = request.data[0];
    auto writes = request.writes;
    bool executed = 0;

    switch(command) {
    case 0x00: if(writes >=  6) { executed = 1; commandTestUnitReady();           } break;
    case 0x08: if(writes >=  6) { executed = 1; commandReadData();                } break;
    case 0xd8: if(writes >= 10) { executed = 1; commandAudioSetStartPosition();   } break;
    case 0xd9: if(writes >= 10) { executed = 1; commandAudioSetStopPosition();    } break;
    case 0xda: if(writes >= 10) { executed = 1; commandAudioPause();              } break;
    case 0xdd: if(writes >= 10) { executed = 1; commandReadSubchannel();          } break;
    case 0xde: if(writes >= 10) { executed = 1; commandGetDirectoryInformation(); } break;
    default:                    { executed = 1; commandInvalid();                 } break;
    }

    if(executed == 0) {
      pin.request = 1;
    } else {
    //print("* request ");
    //for(uint index : range(request.writes)) print(hex(request.data[index], 2L), " ");
    //print("\n");
      request.reset();
    }
  }
}

auto PCD::SCSI::reply(Status status) -> void {
  pin.request = 1;
  pin.message = 0;
  pin.control = 1;
  pin.input = 1;

  if(status == Status::OK) bus.data = 0x00;
  if(status == Status::CheckCondition) bus.data = 0x01;

  messageAfterStatus = 1;
  messageSent = 0;
  statusSent = 0;
}

//0x00
auto PCD::SCSI::commandTestUnitReady() -> void {
  reply(Status::OK);
}

//0x08
auto PCD::SCSI::commandReadData() -> void {
  if(!pcd.fd) return reply(Status::CheckCondition);
  if(!request.data[4]) return reply(Status::OK);

  pcd.drive.start = request.data[1] << 16 | request.data[2] << 8 | request.data[3] << 0;
  pcd.drive.end = pcd.drive.start + request.data[4];
  drive->seekRead();

  irq.ready.raise();
}

//0xd8
auto PCD::SCSI::commandAudioSetStartPosition() -> void {
  if(!pcd.fd) return reply(Status::CheckCondition);

  maybe<int> lba;

  if(request.data[9].bit(6,7) == 0) {
    lba = request.data[3] << 16 | request.data[4] << 8 | request.data[5] << 0;
  }

  if(request.data[9].bit(6,7) == 1) {
    auto m = CD::BCD::decode(request.data[2]);
    auto s = CD::BCD::decode(request.data[3]);
    auto f = CD::BCD::decode(request.data[4]);
    lba = CD::MSF(m, s, f).toLBA();
  }

  if(request.data[9].bit(6,7) == 2) {
    auto trackID = CD::BCD::decode(request.data[2]);
    if(auto track = session->track(trackID)) {
      if(auto index = track->index(1)) {
        lba = index->lba;
      }
    }
  }

  if(!lba) return reply(Status::CheckCondition);

  pcd.drive.start = *lba;
  pcd.drive.end = session->leadOut.lba;
  if(auto trackID = session->inTrack(*lba)) {
    if(auto track = session->track(*trackID)) {
      if(auto index = track->index(track->lastIndex)) {
        pcd.drive.end = index->end;
      }
    }
  }

  if(request.data[1].bit(0) == 0) {
    drive->seekPause();
  }

  if(request.data[1].bit(0) == 1) {
    drive->seekPlay();
  }

  irq.completed.raise();
  reply(Status::OK);
}

//0xd9
auto PCD::SCSI::commandAudioSetStopPosition() -> void {
  if(!pcd.fd) return reply(Status::CheckCondition);

  maybe<int> lba;

  if(request.data[9].bit(6,7) == 0) {
    lba = request.data[3] << 16 | request.data[4] << 8 | request.data[5] << 0;
  }

  if(request.data[9].bit(6,7) == 1) {
    auto m = CD::BCD::decode(request.data[2]);
    auto s = CD::BCD::decode(request.data[3]);
    auto f = CD::BCD::decode(request.data[4]);
    lba = CD::MSF(m, s, f).toLBA();
  }

  if(request.data[9].bit(6,7) == 2) {
    auto trackID = CD::BCD::decode(request.data[2]);
    if(auto track = session->track(trackID)) {
      if(auto index = track->index(1)) {
        lba = index->lba;
      }
    }
  }

  if(!lba) return reply(Status::CheckCondition);

  pcd.drive.end = *lba;

  if(request.data[1].bit(0,1) == 0) {
    drive->setStopped();
  }

  if(request.data[1].bit(0,1) == 1) {
    drive->seekPlay();
    cdda->playMode = PCD::CDDA::PlayMode::Loop;
  }

  if(request.data[1].bit(0,1) == 2) {
    drive->seekPlay();
    cdda->playMode = PCD::CDDA::PlayMode::IRQ;
  }

  if(request.data[1].bit(0,1) == 3) {
    drive->seekPlay();
    cdda->playMode = PCD::CDDA::PlayMode::Once;
  }

  irq.completed.raise();
  reply(Status::OK);
}

//0xda
auto PCD::SCSI::commandAudioPause() -> void {
  if(!pcd.fd) return reply(Status::CheckCondition);
  if(!drive->inCDDA()) return reply(Status::CheckCondition);

  drive->setPaused();
  reply(Status::OK);
}

//0xdd
auto PCD::SCSI::commandReadSubchannel() -> void {
  if(!pcd.fd) return reply(Status::CheckCondition);

  response.reset();

  //0 = playing
  //1 = inactive
  //2 = paused
  //3 = stopped
  uint8 mode = 1;
  if(drive->playing()) mode = 0;
  if(drive->paused())  mode = 2;
  if(drive->stopped()) mode = 3;

  array_view<uint8_t> q{pcd.drive.sector + 2364, 12};
  response.write(mode);  //CDDA mode
  response.write(q[0]);  //control + address
  response.write(q[1]);  //track#
  response.write(q[2]);  //index#
  response.write(q[3]);  //minute (relative)
  response.write(q[4]);  //second (relative)
  response.write(q[5]);  //frame  (relative)
  response.write(q[7]);  //minute (absolute)
  response.write(q[8]);  //second (absolute)
  response.write(q[9]);  //frame  (absolute)

  pin.control = 0;
  pin.input = 1;
  dataTransferCompleted = 1;
}

//0xde
auto PCD::SCSI::commandGetDirectoryInformation() -> void {
  if(!pcd.fd) return reply(Status::CheckCondition);
  response.reset();

  if(request.data[1].bit(0,1) == 0) {
    response.write(CD::BCD::encode(session->firstTrack));
    response.write(CD::BCD::encode(session->lastTrack));
    response.write(0x00);
    response.write(0x00);
  }

  if(request.data[1].bit(0,1) == 1) {
    auto [minute, second, frame] = CD::MSF(session->leadOut.lba);
    response.write(CD::BCD::encode(minute));
    response.write(CD::BCD::encode(second));
    response.write(CD::BCD::encode(frame));
    response.write(0x00);
  }

  if(request.data[1].bit(0,1) == 2) {
    auto lba = session->leadOut.lba;
    auto mode = 0x01;
    auto trackID = CD::BCD::decode(request.data[2]);
    if(auto track = session->track(trackID)) {
      if(auto index = track->index(1)) {
        lba = index->lba;
        mode = track->control;
      }
    }
    auto [minute, second, frame] = CD::MSF::fromLBA(150 + lba);
    response.write(CD::BCD::encode(minute));
    response.write(CD::BCD::encode(second));
    response.write(CD::BCD::encode(frame));
    response.write(mode);
  }

  if(request.data[1].bit(0,1) == 3) {
    auto lba = session->leadOut.lba;
    auto mode = 0x01;
    auto trackID = CD::BCD::decode(request.data[2]);
    if(auto track = session->track(trackID)) {
      if(auto index = track->index(1)) {
        lba = index->lba;
        mode = track->control;
      }
    }
    auto [minute, second, frame] = CD::MSF::fromLBA(150 + lba);
    response.write(lba >> 16);
    response.write(lba >>  8);
    response.write(lba >>  0);
    response.write(mode);
  }

  pin.control = 0;
  pin.input = 1;
  dataTransferCompleted = 1;
}

auto PCD::SCSI::commandInvalid() -> void {
  reply(Status::CheckCondition);
}

auto PCD::SCSI::power() -> void {
  irq = {};
  pin = {};
  bus = {};
  acknowledging = 0;
  dataTransferCompleted = 0;
  messageAfterStatus = 0;
  messageSent = 0;
  statusSent = 0;
  request = {};
  response = {};
}
