auto PCD::SCSI::clock() -> void {
  if(++counter == 122880) {
    counter = 0;
    clockRead();
  }
}

auto PCD::SCSI::clockRead() -> void {
  if(read.lba >= read.end) return;
  if(dataBuffer.index != dataBuffer.size) return;

print("* read ", read.lba, "\n");

  pcd.fd->seek((abs(pcd.session.leadIn.lba) + read.lba) * 2448 + 16);
  pcd.fd->read(dataBuffer.data, 2048);

  if(++read.lba == read.end) {
    dataTransferred = 1;
  } else {
    dataTransferred = 0;
  }

  control = 0;
  input = 1;

  dataBuffer.index = 0;
  dataBuffer.size = 2048;
}

auto PCD::SCSI::readData() -> uint8 {
  uint8 result = data;

  if(request && !acknowledge && !control) {
    if(input) {
      acknowledge = 1;
      update();
      acknowledge = 0;
      update();
    }
  }

  return result;
}

auto PCD::SCSI::update() -> void {
  if(!wasReset && reset) {
    busy = 0;
    request = 0;
    message = 0;
    control = 0;
    input = 0;
    select = 0;
    attention = 0;
  }
  wasReset = reset;

  if(!busy && !select && selected) {
    selected = 0;
    request = 0;
    message = 0;
    control = 0;
    input = 0;
    irq.completed.pending = 0;
  }

  if(!selected && select) {
    selected = 1;
    busy = 1;
    request = 1;
    message = 0;
    control = 1;
    input = 0;
  }

  if(busy) {
    if(message) {
      if(input) {
        messageInput();
      } else {
        messageOutput();
      }
    } else {
      if(input) {
        dataInput();
      } else {
        dataOutput();
      }
    }
  }
}

auto PCD::SCSI::messageInput() -> void {
  if(request && acknowledge) {
    request = 0;
    messageSent = 1;
  }

  if(!request && !acknowledge && messageSent) {
    busy = 0;
    messageSent = 0;
  }
}

auto PCD::SCSI::messageOutput() -> void {
  if(request && acknowledge) {
    request = 0;
  }
}

auto PCD::SCSI::dataInput() -> void {
  if(control) {
    if(request && acknowledge) {
      request = 0;
      statusSent = 1;
    }

    if(!request && !acknowledge && statusSent) {
      statusSent = 0;
      if(messageAfterStatus) {
        messageAfterStatus = 0;
        request = 1;
        message = 1;
        data = 0x00;
      }
    }
  } else {
    if(request && acknowledge) {
      request = 0;
    }

    if(!request && !acknowledge) {
      if(dataBuffer.index == dataBuffer.size) {
        irq.ready.pending = 0;
        if(dataTransferred) {
          dataTransferred = 0;
          replyStatusOK();
          irq.completed.pending = 1;
        }
      } else {
        data = dataBuffer.data[dataBuffer.index++];
        request = 1;
      }
    }
  }
}

auto PCD::SCSI::dataOutput() -> void {
  if(request && acknowledge) {
    request = 0;
    commandBuffer.data[commandBuffer.index++] = data;
if(commandBuffer.index == 1) print(hex(commandBuffer.data[0], 2L), "\n");
  }

  if(!request && !acknowledge && commandBuffer.index) {
    auto command = commandBuffer.data[0];
    auto index = commandBuffer.index;
    if(command == 0x00 && index ==  6) return commandTestUnitReady();
    if(command == 0x08 && index ==  6) return commandRead();
    if(command == 0xde && index == 10) return commandGetDirectoryInformation();
    if(command == 0xff && index ==  1) return commandIgnore();
    request = 1;
  }
}

auto PCD::SCSI::commandTestUnitReady() -> void {
  replyStatusOK();
  commandBuffer.index = 0;
}

auto PCD::SCSI::commandRead() -> void {
  if(!pcd.fd) return replyStatusError();
  if(!commandBuffer.data[4]) return replyStatusOK();

  read.lba = commandBuffer.data[1] << 16 | commandBuffer.data[2] << 8 | commandBuffer.data[3] << 0;
  read.end = read.lba + commandBuffer.data[4];

  commandBuffer.index = 0;

  irq.ready.pending = 1;
}

auto PCD::SCSI::commandGetDirectoryInformation() -> void {
  if(!pcd.fd) return replyStatusError();
  auto& session = pcd.session;

  if(commandBuffer.data[1] == 0x00) {
    dataBuffer.data[0] = CD::BCD::encode(session.firstTrack);
    dataBuffer.data[1] = CD::BCD::encode(session.lastTrack);
    dataBuffer.size = 2;
  }

  if(commandBuffer.data[1] == 0x01) {
    auto [minute, second, frame] = CD::MSF(session.leadOut.lba);
    dataBuffer.data[0] = CD::BCD::encode(minute);
    dataBuffer.data[1] = CD::BCD::encode(second);
    dataBuffer.data[2] = CD::BCD::encode(frame);
    dataBuffer.size = 3;
  }

  if(commandBuffer.data[1] == 0x02) {
    auto lba = session.leadOut.lba;
    auto mode = 0b0100;
    auto track = CD::BCD::decode(commandBuffer.data[2]);
    if(track < 100 && session.tracks[track]) {
      auto index = session.tracks[track].firstIndex;
      if(index != -1) {
        lba = session.tracks[track].indices[index].lba;
        mode = session.tracks[track].control;
        if(!session.tracks[track].isAudio()) {
          int pregap = session.tracks[track].pregap();
          if(pregap != CD::InvalidLBA) lba += pregap;
        }
      }
    }
    auto [minute, second, frame] = CD::MSF::fromLBA(lba + 150);
    dataBuffer.data[0] = CD::BCD::encode(minute);
    dataBuffer.data[1] = CD::BCD::encode(second);
    dataBuffer.data[2] = CD::BCD::encode(frame);
    dataBuffer.data[3] = mode;
    dataBuffer.size = 4;
  }

  control = 0;
  input = 1;

  commandBuffer.index = 0;
  dataBuffer.index = 0;
  dataTransferred = 1;
}

auto PCD::SCSI::commandIgnore() -> void {
  replyStatusOK();
  commandBuffer.index = 0;
}

auto PCD::SCSI::replyStatusOK() -> void {
  request = 1;
  message = 0;
  control = 1;
  input = 1;

  messageAfterStatus = 1;
  messageSent = 0;
  statusSent = 0;
  data = 0x00;
}

auto PCD::SCSI::replyStatusError() -> void {
  request = 1;
  message = 0;
  control = 1;
  input = 1;

  messageAfterStatus = 1;
  messageSent = 0;
  statusSent = 0;
  data = 0x01;
}
