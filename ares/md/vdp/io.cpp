auto VDP::read(n24 address, n16) -> n16 {
  switch(address & 0xc0001e) {

  //data port
  case 0xc00000: case 0xc00002: {
    return readDataPort();
  }

  //control port
  case 0xc00004: case 0xc00006: {
    return readControlPort();
  }

  //counter
  case 0xc00008: case 0xc0000a: case 0xc0000c: case 0xc0000e: {
    auto vcounter = state.vcounter;
    if(io.interlaceMode.bit(0)) {
      if(io.interlaceMode.bit(1)) vcounter <<= 1;
      vcounter.bit(0) = vcounter.bit(8);
    }
    return vcounter << 8 | (state.hdot >> 1) << 0;
  }

  }

  return 0x0000;
}

auto VDP::write(n24 address, n16 data) -> void {
  switch(address & 0xc0001e) {

  //data port
  case 0xc00000: case 0xc00002: {
    return writeDataPort(data);
  }

  //control port
  case 0xc00004: case 0xc00006: {
    return writeControlPort(data);
  }

  }
}

//

auto VDP::readDataPort() -> n16 {
  io.commandPending = false;

  auto address = io.address.bit(1,16);
  fifo.read(io.command, address << 1 | 0);
  fifo.read(io.command, address << 1 | 1);
  io.address += io.dataIncrement;

  while(!fifo.requests.empty()) cpu.wait(1);
  return fifo.response;
}

auto VDP::writeDataPort(n16 data) -> void {
  io.commandPending = false;

  //DMA VRAM fill
  if(dma.wait) {
    dma.wait = false;
    dma.filldata = data >> 8;
    //falls through to memory write
    //causes extra transfer to occur on VRAM fill operations
  }

  auto address = io.address.bit(1,16);
  if(io.address.bit(0)) data = data >> 8 | data << 8;
  fifo.write(io.command, address << 1 | 0, data >> 8);
  fifo.write(io.command, address << 1 | 1, data >> 0);
  io.address += io.dataIncrement;
}

//

auto VDP::readControlPort() -> n16 {
  io.commandPending = false;

  n16 result;
  result.bit( 0) = Region::PAL();
  result.bit( 1) = io.command.bit(5);  //DMA active
  result.bit( 2) = hsync();
  result.bit( 3) = vsync() || !io.displayEnable;
  result.bit( 4) = io.interlaceMode.bit(0) && field();
  result.bit( 5) = sprite.collision;
  result.bit( 6) = sprite.overflow;
  result.bit( 7) = io.vblankInterruptTriggered;
  result.bit( 8) = fifo.slots.full();
  result.bit( 9) = fifo.slots.empty();
  result.bit(10) = 1;  //constants (bits 10-15)
  result.bit(11) = 0;
  result.bit(12) = 1;
  result.bit(13) = 1;
  result.bit(14) = 0;
  result.bit(15) = 0;

  sprite.collision = 0;
  sprite.overflow  = 0;
  return result;
}

auto VDP::writeControlPort(n16 data) -> void {
  //command write (lo)
  if(io.commandPending) {
    io.commandPending = false;

    io.command.bit(2,5) = data.bit(4,7);
    io.address.bit(14,16) = data.bit(0,2);

    if(!dma.enable) io.command.bit(5) = 0;
    if(dma.mode == 3) dma.wait = false;
    return;
  }

  io.command.bit(0,1) = data.bit(14,15);
  io.address.bit(0,13) = data.bit(0,13);

  //command write (hi)
  if(data.bit(14,15) != 2) {
    io.commandPending = true;
    return;
  }

  //register write (d13 is ignored)
  switch(data.bit(8,12)) {

  //mode register 1
  case 0x00: {
    io.displayOverlayEnable  = data.bit(0);
    io.counterLatch          = data.bit(1);
    io.hblankInterruptEnable = data.bit(4);
    io.leftColumnBlank       = data.bit(5);
    return;
  }

  //mode register 2
  case 0x01: {
    io.videoMode             = data.bit(2);
    io.overscan              = data.bit(3);
    dma.enable               = data.bit(4);
    io.vblankInterruptEnable = data.bit(5);
    io.displayEnable         = data.bit(6);
    vram.mode                = data.bit(7);
    if(!dma.enable) io.command.bit(5) = 0;
    return;
  }

  //layer A name table location
  case 0x02: {
    layerA.nametableAddress.bit(12,15) = data.bit(3,6);
    return;
  }

  //window name table location
  case 0x03: {
    window.nametableAddress.bit(10,15) = data.bit(1,6);
    return;
  }

  //layer B name table location
  case 0x04: {
    layerB.nametableAddress.bit(12,15) = data.bit(0,3);
    return;
  }

  //sprite attribute table location
  case 0x05: {
    sprite.nametableAddress.bit(8,15) = data.bit(0,7);
    return;
  }

  //sprite pattern base address
  case 0x06: {
    sprite.generatorAddress.bit(15) = data.bit(5);
    return;
  }

  //background color
  case 0x07: {
    io.backgroundColor = data.bit(0,5);
    return;
  }

  //horizontal interrupt counter
  case 0x0a: {
    io.hblankInterruptCounter = data.bit(0,7);
    return;
  }

  //mode register 3
  case 0x0b: {
    layers.hscrollMode         = data.bit(0,1);
    layers.vscrollMode         = data.bit(2);
    io.externalInterruptEnable = data.bit(3);
    return;
  }

  //mode register 4
  case 0x0c: {
    io.displayWidth = data.bit(0) | data.bit(7) << 1;
    io.interlaceMode = data.bit(1,2);
    io.shadowHighlightEnable = data.bit(3);
    io.externalColorEnable = data.bit(4);
    io.hsync = data.bit(5);
    io.vsync = data.bit(6);
    return;
  }

  //horizontal scroll data location
  case 0x0d: {
    layers.hscrollAddress = data.bit(0,6) << 9;
    return;
  }

  //nametable pattern base address
  case 0x0e: {
    //bit(0) relocates plane A to the extended VRAM region.
    //bit(4) relocates plane B, but only when bit(0) is also set.
    layerA.generatorAddress.bit(15) = data.bit(0);
    layerB.generatorAddress.bit(15) = data.bit(4) && data.bit(0);
    return;
  }

  //data port auto-increment value
  case 0x0f: {
    io.dataIncrement = data.bit(0,7);
    return;
  }

  //layer size
  case 0x10: {
    layers.nametableWidth = data.bit(0,1);
    layers.nametableHeight = data.bit(4,5);
    return;
  }

  //window plane horizontal position
  case 0x11: {
    window.hoffset    = data.bit(0,4) << 4;
    window.hdirection = data.bit(7);
    return;
  }

  //window plane vertical position
  case 0x12: {
    window.voffset    = data.bit(0,4) << 3;
    window.vdirection = data.bit(7);
    return;
  }

  //DMA length
  case 0x13: {
    dma.length.bit(0,7) = data.bit(0,7);
    return;
  }

  //DMA length
  case 0x14: {
    dma.length.bit(8,15) = data.bit(0,7);
    return;
  }

  //DMA source
  case 0x15: {
    dma.source.bit(0,7) = data.bit(0,7);
    return;
  }

  //DMA source
  case 0x16: {
    dma.source.bit(8,15) = data.bit(0,7);
    return;
  }

  //DMA source
  case 0x17: {
    dma.source.bit(16,21) = data.bit(0,5);
    dma.mode = data.bit(6,7);
    dma.wait = dma.mode.bit(1);
    return;
  }

  //unused
  default: {
    return;
  }

  }
}
