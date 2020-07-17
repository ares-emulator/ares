auto Disc::readDMA() -> u32 {
  u32 data = 0;
  if(!fifo.data.empty()) data |= fifo.data.read() <<  0;
  if(!fifo.data.empty()) data |= fifo.data.read() <<  8;
  if(!fifo.data.empty()) data |= fifo.data.read() << 16;
  if(!fifo.data.empty()) data |= fifo.data.read() << 24;
  return data;
}

auto Disc::readByte(u32 address) -> u32 {
  uint8 data = 0;

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
    if(!fifo.response.empty()) data = fifo.response.read();
  }

  //data FIFO
  if(address == 0x1f80'1802 && (io.index == 0 || io.index == 1 || io.index == 2 || io.index == 3)) {
    if(!fifo.data.empty()) data = fifo.data.read();
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
    uint3 flags = 0;
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

//print("* r", hex(address, 8L), ":", io.index, "=", hex(data, 2L), "\n");

  return data;
}

auto Disc::readHalf(u32 address) -> u32 {
  uint16 data = readByte(address & ~1 | 0) <<  0;
  return data | readByte(address & ~1 | 1) <<  8;
}

auto Disc::readWord(u32 address) -> u32 {
  uint32 data = readHalf(address & ~3 | 0) <<  0;
  return data | readHalf(address & ~3 | 2) << 16;
}

auto Disc::writeByte(u32 address, u32 value) -> void {
  uint8 data = value;

//print("* w", hex(address, 8L), ":", io.index, "=", hex(data, 2L), "\n");

  if(address == 0x1f80'1800) {
    io.index = data.bit(0,1);
  }

  //command register
  if(address == 0x1f80'1801 && io.index == 0) {
    event.command = data;
    event.counter = 25000;
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
    cdda.volumeLatch[3] = data;
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
    cdda.volumeLatch[0] = data;
  }

  //audio volume for right CD output to left SPU input
  if(address == 0x1f80'1802 && io.index == 3) {
    cdda.volumeLatch[2] = data;
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
    cdda.volumeLatch[1] = data;
  }

  //audio volume apply changes
  if(address == 0x1f80'1803 && io.index == 3) {
    adpcm.mute = data.bit(0);
    if(data.bit(5)) {
      cdda.volume[0] = cdda.volumeLatch[0];
      cdda.volume[1] = cdda.volumeLatch[1];
      cdda.volume[2] = cdda.volumeLatch[2];
      cdda.volume[3] = cdda.volumeLatch[3];
    }
  }
}

auto Disc::writeHalf(u32 address, u32 data) -> void {
  writeByte(address & ~1 | 0, data >>  0);
  writeByte(address & ~1 | 1, data >>  8);
}

auto Disc::writeWord(u32 address, u32 data) -> void {
  writeHalf(address & ~3 | 0, data >>  0);
  writeHalf(address & ~3 | 2, data >> 16);
}
