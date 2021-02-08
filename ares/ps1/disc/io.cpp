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
    data.bit(2) = 0;  //XA-ADPCM FIFO (0 = empty)
    data.bit(3) = fifo.parameter.empty();  //1 when empty
    data.bit(4) = !fifo.parameter.full();  //0 when full
    data.bit(5) = !fifo.response.empty();  //0 when empty
    data.bit(6) = !fifo.data.empty();      //0 when empty
    data.bit(7) = 0;  //command/parameter busy (0 = ready)
  }

  //response FIFO
  if(address == 0x1f80'1801 && (io.index == 0 || io.index == 1 || io.index == 2 || io.index == 3)) {
    data = fifo.response.read(data);
  }

  //data FIFO
  if(address == 0x1f80'1802 && (io.index == 0 || io.index == 1 || io.index == 2 || io.index == 3)) {
    data = fifo.data.read(data);
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
    data.bit(3) = irq.end.flag;
    data.bit(4) = irq.error.flag;
    data.bit(5) = 1;
    data.bit(6) = 1;
    data.bit(7) = 1;
  }

  return data;
}

auto Disc::readHalf(u32 address) -> u32 {
  debug(unverified, "Disc::readHalf(", hex(address, 8L), ")");
  n16    data = readByte(address & ~1 | 0) <<  0;
  return data | readByte(address & ~1 | 1) <<  8;
}

auto Disc::readWord(u32 address) -> u32 {
  debug(unverified, "Disc::readWord(", hex(address, 8L), ")");
  n32    data = readHalf(address & ~3 | 0) <<  0;
  return data | readHalf(address & ~3 | 2) << 16;
}

auto Disc::writeByte(u32 address, u32 value) -> void {
  n8 data = value;

  if(address == 0x1f80'1800) {
    io.index = data.bit(0,1);
  }

  //command register
  if(address == 0x1f80'1801 && io.index == 0) {
    if(event.counter) {
      debug(unimplemented, "Disc::writeByte(): ", hex(event.counter, 2L), "->", hex(data, 2L));
    }
    event.command = data;
    event.counter = 50'000;
    event.invocation = 0;
  }

  //sound map data output
  if(address == 0x1f80'1801 && io.index == 1) {
  }

  //sound map coding information
  if(address == 0x1f80'1801 && io.index == 2) {
  }

  //audio volume for right CD output to right SPU input
  if(address == 0x1f80'1801 && io.index == 3) {
    audio.volumeLatch[3] = data;
  }

  //parameter FIFO
  if(address == 0x1f80'1802 && io.index == 0) {
    if(!fifo.parameter.full()) fifo.parameter.write(data);
  }

  //interrupt enable
  if(address == 0x1f80'1802 && io.index == 1) {
    irq.ready.enable       = data.bit(0);
    irq.complete.enable    = data.bit(1);
    irq.acknowledge.enable = data.bit(2);
    irq.end.enable         = data.bit(3);
    irq.error.enable       = data.bit(4);
    irq.poll();
  }

  //audio volume for left CD output to left SPU input
  if(address == 0x1f80'1802 && io.index == 2) {
    audio.volumeLatch[0] = data;
  }

  //audio volume for right CD output to left SPU input
  if(address == 0x1f80'1802 && io.index == 3) {
    audio.volumeLatch[2] = data;
  }

  //request register
  if(address == 0x1f80'1803 && io.index == 0) {
  }

  //interrupt flag
  if(address == 0x1f80'1803 && io.index == 1) {
    if(data.bit(0,2) == 7) {
      if(0);
      else if(irq.ready.flag      ) irq.ready.flag       = 0;
      else if(irq.complete.flag   ) irq.complete.flag    = 0;
      else if(irq.acknowledge.flag) irq.acknowledge.flag = 0;
      else if(irq.end.flag        ) irq.end.flag         = 0;
      else if(irq.error.flag      ) irq.error.flag       = 0;
    }
    if(data.bit(3)) irq.end.flag   = 0;
    if(data.bit(4)) irq.error.flag = 0;
    if(data.bit(6)) fifo.parameter.flush();
    irq.poll();
  }

  //audio volume for left CD output to right SPU input
  if(address == 0x1f80'1803 && io.index == 2) {
    audio.volumeLatch[1] = data;
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
  }
}

auto Disc::writeHalf(u32 address, u32 data) -> void {
  debug(unverified, "Disc::writeHalf(", hex(address, 8L), ")");
  writeByte(address & ~1 | 0, data >>  0);
  writeByte(address & ~1 | 1, data >>  8);
}

auto Disc::writeWord(u32 address, u32 data) -> void {
  debug(unverified, "Disc::writeWord(", hex(address, 8L), ")");
  writeHalf(address & ~3 | 0, data >>  0);
  writeHalf(address & ~3 | 2, data >> 16);
}
