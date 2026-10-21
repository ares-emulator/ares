auto Disc::canReadDMA() -> bool {
  return io.sectorBufferReadRequest && !fifo.data.empty();
}

auto Disc::readDMA() -> u32 {
  u32 data = 0;
  data |= fifo.data.read(0) <<  0;
  data |= fifo.data.read(0) <<  8;
  data |= fifo.data.read(0) << 16;
  data |= fifo.data.read(0) << 24;
  return data;
}

auto Disc::readByte(u32 address) -> u32 {
  n8 data = 0;

  if(address == 0x1f80'1800) {
    data.bit(0) = io.index.bit(0);
    data.bit(1) = io.index.bit(1);
    data.bit(2) = !cdxa.samples.empty();
    data.bit(3) = fifo.parameter.empty();  //1 when empty
    data.bit(4) = !fifo.parameter.full();  //0 when full
    data.bit(5) = !fifo.response.empty();  //0 when empty
    data.bit(6) = io.sectorBufferReadRequest && !fifo.data.empty();      //0 when empty
    data.bit(7) = command.transfer.started; //command/parameter busy (0 = ready)
    return data;
  }

  //response FIFO
  if(address == 0x1f80'1801 && (io.index == 0 || io.index == 1 || io.index == 2 || io.index == 3)) {
    data = fifo.response.read(data);
    if(fifo.response.empty()) flushDeferredResponse();
    return data;
  }

  //data FIFO
  if(address == 0x1f80'1802 && (io.index == 0 || io.index == 1 || io.index == 2 || io.index == 3)) {
    return data = fifo.data.read(data);
  }

  //interrupt enable
  if(address == 0x1f80'1803 && (io.index == 0 || io.index == 2)) {
    data.bit(0) = irq.ready.enable;
    data.bit(1) = irq.complete.enable;
    data.bit(2) = irq.acknowledge.enable;
    data.bit(3) = irq.end.enable;
    data.bit(4) = irq.error.enable;
    data.bit(5) = 1;
    data.bit(6) = 1;
    data.bit(7) = 1;
    return data;
  }

  //interrupt flag
  if(address == 0x1f80'1803 && (io.index == 1 || io.index == 3)) {
    n3 flags = 0;
    if(irq.error.flag      ) flags = 5;
    if(irq.end.flag        ) flags = 4;
    if(irq.acknowledge.flag) flags = 3;
    if(irq.complete.flag   ) flags = 2;
    if(irq.ready.flag      ) flags = 1;
    data.bit(0,2) = flags;
    data.bit(5) = 1;
    data.bit(6) = 1;
    data.bit(7) = 1;
    return data;
  }

  debug(unhandled, "Disc::readByte(", hex(address, 8L), ") -> ", hex(data, 2L));
  return data;
}

auto Disc::readHalf(u32 address) -> u32 {
  n32 data;
  debug(unhandled, "Disc::readHalf(", hex(address, 8L), ") -> ", hex(data, 4L));
  return data;
}

auto Disc::readWord(u32 address) -> u32 {
  n32 data;
  debug(unhandled, "Disc::readWord(", hex(address, 8L), ") -> ", hex(data, 8L));
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
    if(!command.transfer.started) {
      command.transfer.started = 1;
      command.transfer.counter = 40'000;
    }

    command.transfer.command = data;
    ssr.error = 0;
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
    if(!fifo.parameter.full()) {
      fifo.parameter.write(data);
      return;
    }

    debug(unusual, "Disc::writeByte(): parameter FIFO full, data lost");
    return;
  }

  //interrupt enable
  if(address == 0x1f80'1802 && io.index == 1) {
    irq.ready.enable       = data.bit(0);
    irq.complete.enable    = data.bit(1);
    irq.acknowledge.enable = data.bit(2);
    irq.end.enable         = data.bit(3);
    irq.error.enable       = data.bit(4);
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
    io.soundMapEnable = data.bit(5);
    io.sectorBufferWriteRequest = data.bit(6);
    io.sectorBufferReadRequest = data.bit(7);
    return;
  }

  //interrupt flag
  if(address == 0x1f80'1803 && io.index == 1) {
    u8 flags = irq.error.flag ? 5 : irq.end.flag ? 4 : irq.acknowledge.flag ? 3 : irq.complete.flag ? 2 : irq.ready.flag ? 1 : 0;
    u8 remaining = flags & ~data.bit(0,2);
    bool acknowledged = flags != 0 && remaining == 0;
    irq.ready.flag       = remaining == 1;
    irq.complete.flag    = remaining == 2;
    irq.acknowledge.flag = remaining == 3;
    irq.end.flag         = remaining == 4;
    irq.error.flag       = remaining == 5;
    if(data.bit(6)) fifo.parameter.flush();
    irq.poll();
    if(acknowledged && (fifo.deferred.acknowledge.type != ResponseType::None
    || fifo.deferred.complete.type != ResponseType::None
    || fifo.deferred.ready.type != ResponseType::None)) {
      fifo.deferred.counter = 20'000;
    }
    flushDeferredResponse();
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
  debug(unhandled, "Disc::writeHalf(", hex(address, 8L), ", ", hex(data, 4L), ")");
}

auto Disc::writeWord(u32 address, u32 data) -> void {
  debug(unhandled, "Disc::writeWord(", hex(address, 8L), ", ", hex(data, 8L), ")");
}
