auto Disc::canReadDMA() -> bool {
  return io.sectorBufferReadRequest;
}

auto Disc::readDMA() -> u32 {
  u32 data = 0;
  data |= u32(readData()) <<  0;
  data |= u32(readData()) <<  8;
  data |= u32(readData()) << 16;
  data |= u32(readData()) << 24;
  return data;
}

auto Disc::readData() -> u8 {
  if(!io.sectorBufferReadRequest) return fifo.data.exhausted ? fifo.data.padding : 0;
  auto value = fifo.data.read();
  if(fifo.data.empty()) {
    io.sectorBufferReadRequest = 0;
    if(fifo.sectorPending && fifo.deferred.type == ResponseType::None) {
      fifo.deferred.type = ResponseType::Ready;
      fifo.deferred.data.write(status());
      scheduleAsyncResponse(5000);
    }
  }
  return value;
}

auto Disc::readByte(u32 address) -> u32 {
  n8 data = 0;

  if(address == 0x1f80'1800) {
    data.bit(0) = io.index.bit(0);
    data.bit(1) = io.index.bit(1);
    data.bit(2) = 0;  // ADPBUSY unsupported: reference fallback, not the actual XA decoder/queue state.
    data.bit(3) = fifo.parameter.empty();  //1 when empty
    data.bit(4) = !fifo.parameter.full();  //0 when full
    data.bit(5) = !fifo.response.empty();  //0 when empty
    data.bit(6) = io.sectorBufferReadRequest;
    data.bit(7) = command.first.pending;  //the second response does not assert command BUSY
    return data;
  }

  //response FIFO
  if(address == 0x1f80'1801 && (io.index == 0 || io.index == 1 || io.index == 2 || io.index == 3)) {
    if(!fifo.response.empty()) fifo.response.read();
    return fifo.responseLatch[fifo.responsePosition++];
  }

  //data FIFO
  if(address == 0x1f80'1802 && (io.index == 0 || io.index == 1 || io.index == 2 || io.index == 3)) {
    return readData();
  }

  //interrupt enable
  if(address == 0x1f80'1803 && (io.index == 0 || io.index == 2)) {
    return 0xe0 | irq.mask;
  }

  //interrupt flag
  if(address == 0x1f80'1803 && (io.index == 1 || io.index == 3)) {
    return 0xe0 | irq.flag;
  }

  debug(unhandled, "Disc::readByte(", hex(address, 8L), ") -> ", hex(data, 2L));
  return data;
}

auto Disc::readHalf(u32 address) -> u32 {
  u32 data = 0;
  for(u32 byte : range(2)) data |= readByte(address + (memory.cdrom.autoIncrement ? byte : 0)) << (byte * 8);
  return data;
}

auto Disc::readWord(u32 address) -> u32 {
  u32 data = 0;
  for(u32 byte : range(4)) data |= readByte(address + (memory.cdrom.autoIncrement ? byte : 0)) << (byte * 8);
  return data;
}

auto Disc::writeByte(u32 address, u32 value) -> void {
  n8 data = value;

  if(address == 0x1f80'1800) {
    io.index = data.bit(0,1);
    return;
  }

  //command register
  if(address == 0x1f80'1801 && io.index == 0) {
    beginCommand(data);
    return;
  }

  //sound map data output
  if(address == 0x1f80'1801 && io.index == 1) {
    debug(unimplemented, "Disc::writeByte(): sound map data output = ", hex(data, 2L));
    return;
  }

  //sound map coding information
  if(address == 0x1f80'1801 && io.index == 2) {
    debug(unimplemented, "Disc::writeByte(): sound map coding information = ", hex(data, 2L));
    return;
  }

  //audio volume for right CD output to right SPU input
  if(address == 0x1f80'1801 && io.index == 3) {
    audio.volumeLatch[3] = data;
    return;
  }

  //parameter FIFO
  if(address == 0x1f80'1802 && io.index == 0) {
    if(fifo.parameter.full()) fifo.parameter.read();  //reference overflow policy: retain the newest 16 bytes
    fifo.parameter.write(data);
    return;
  }

  //interrupt enable
  if(address == 0x1f80'1802 && io.index == 1) {
    irq.mask = data.bit(0,4);
    irq.poll();
    return;
  }

  //audio volume for left CD output to left SPU input
  if(address == 0x1f80'1802 && io.index == 2) {
    audio.volumeLatch[0] = data;
    return;
  }

  //audio volume for right CD output to left SPU input
  if(address == 0x1f80'1802 && io.index == 3) {
    audio.volumeLatch[2] = data;
    return;
  }

  //request register
  if(address == 0x1f80'1803 && io.index == 0) {
    bool wasReading = io.sectorBufferReadRequest;
    // SMEN/BFWR are retained but sound-map upload/playback is unsupported, as in the reference.
    io.soundMapEnable = data.bit(5);
    io.sectorBufferWriteRequest = data.bit(6);
    io.sectorBufferReadRequest = data.bit(7);
    if(io.sectorBufferReadRequest && !wasReading && !fifo.hostLoaded) {
      fifo.data = fifo.sectors[fifo.sectorRead];
      fifo.hostLoaded = true;
    }
    if(!io.sectorBufferReadRequest && !fifo.data.empty()) fifo.data.position = 0;
    return;
  }

  //interrupt flag
  if(address == 0x1f80'1803 && io.index == 1) {
    // SMADPCLR/CHPRST (bits5/7) remain unsupported reference no-ops; bit6 still clears parameters.
    auto previous = irq.flag;
    irq.flag &= ~data.bit(0,4);
    if(previous && !irq.flag) irq.acknowledgeAge = 0;
    if(previous && !irq.flag && fifo.deferred.scheduled && fifo.deferred.counter <= 0) {
      fifo.deferred.counter = 500;
    }
    if(data.bit(6)) fifo.parameter.flush();
    irq.poll();
    scheduleAsyncResponse();
    updateCommandEvent();
    return;
  }

  //audio volume for left CD output to right SPU input
  if(address == 0x1f80'1803 && io.index == 2) {
    audio.volumeLatch[1] = data;
    return;
  }

  //audio volume apply changes
  if(address == 0x1f80'1803 && io.index == 3) {
    audio.muteADPCM = data.bit(0);
    if(data.bit(5)) {
      audio.volume[0] = audio.volumeLatch[0];
      audio.volume[1] = audio.volumeLatch[1];
      audio.volume[2] = audio.volumeLatch[2];
      audio.volume[3] = audio.volumeLatch[3];
    }
    if(audio.muteADPCM) debug(unusual, "Disc::writeByte: ADPMUTE = 1");
    return;
  }

  debug(unhandled, "Disc::writeByte(", hex(address, 8L), ", ", hex(data, 2L), ")");
}

auto Disc::writeHalf(u32 address, u32 data) -> void {
  for(u32 byte : range(2)) writeByte(address + (memory.cdrom.autoIncrement ? byte : 0), data >> (byte * 8));
}

auto Disc::writeWord(u32 address, u32 data) -> void {
  for(u32 byte : range(4)) writeByte(address + (memory.cdrom.autoIncrement ? byte : 0), data >> (byte * 8));
}
