auto VDP::read(n1 upper, n1 lower, n24 address, n16 data) -> n16 {
  switch(address) {

  //data port
  case 0xc00000 ... 0xc00003: {
    return readDataPort();
  }

  //control port
  case 0xc00004 ... 0xc00007: {
    return readControlPort();
  }

  //counters
  case 0xc00008 ... 0xc0000f: {
    if(io.counterLatch) return state.counterLatchValue;
    auto vcounter = state.vcounter;
    if(io.interlaceMode.bit(0)) {
      if(io.interlaceMode.bit(1)) vcounter <<= 1;
      vcounter.bit(0) = vcounter.bit(8);
    }
    return vcounter << 8 | hcounter() << 0;
  }

  //PSG
  case 0xc00010 ... 0xc00017: {
    //reading from the PSG should deadlock the machine
    return data;
  }

  //test address port (write-only)
  case 0xc00018 ... 0xc0001b: {
    return data;
  }

  //test data port
  case 0xc0001c ... 0xc0001f: {
    switch(test.address) {

    //unknown
    case 0x8 ... 0xf: {
      return data;
    }

    }
    return data;
  }

  }

  return data;
}

auto VDP::write(n1 upper, n1 lower, n24 address, n16 data) -> void {
  switch(address) {

  //data port
  case 0xc00000 ... 0xc00003: {
    return writeDataPort(data);
  }

  //control port
  case 0xc00004 ... 0xc00007: {
    return writeControlPort(data);
  }

  //counters (read-only)
  case 0xc00008 ... 0xc0000f: {
    return;
  }

  //PSG
  case 0xc00010 ... 0xc00017: {
    if(!lower) return;  //byte writes to even PSG registers have no effect
    return psg.write(data);
  }

  //test address port
  case 0xc00018 ... 0xc0001b: {
    test.address = data.bit(0,3);
    return;
  }

  //test data port
  case 0xc0001c ... 0xc0001f: {
    switch(test.address) {

    case 0x0: {
      //bit(0,5) is unknown
      dac.test.disableLayers    = data.bit(6);
      dac.test.forceLayer       = data.bit(7,8);
      psg.test.volumeOverride   = data.bit(9);
      psg.test.volumeChannel    = data.bit(10,11);
      sprite.test.disablePhase1 = data.bit(12);
      sprite.test.disablePhase2 = data.bit(13);
      sprite.test.disablePhase3 = data.bit(14);
      return;
    }

    case 0x1: {
      //bit(0) affects Z80 clock (ZCLK)
      //bit(1-15) is unknown
      return;
    }

    case 0x2: {
      //appears to reset video signal generator whenever any value is written
      return;
    }

    //unknown
    case 0x3 ... 0x7: {
      return;
    }

    }

    return;
  }

  }
}

//

auto VDP::readDataPort() -> n16 {
  command.latch = 0;
  command.ready = 0;

  while(!prefetch.full()) {
    if(cpu.active()) cpu.wait(1);
    if(apu.active()) apu.step(1);
  }
  command.address += command.increment;
  command.ready = 0;
  prefetch.read(command.target, command.address);
  return prefetch.slot.data;
}

auto VDP::writeDataPort(n16 data) -> void {
  command.latch = 0;
  command.ready = 1;

  fifo.write(command.target, command.address, data);
  command.address += command.increment;
}

//

auto VDP::readControlPort() -> n16 {
  command.latch = 0;

  n16 result;
  result.bit( 0) = Region::PAL();
  result.bit( 1) = command.pending;
  result.bit( 2) = hblank();
  result.bit( 3) = vblank() || !io.displayEnable;
  result.bit( 4) = io.interlaceMode.bit(0) && field();
  result.bit( 5) = sprite.collision;
  result.bit( 6) = sprite.overflow;
  result.bit( 7) = irq.vblank.pending;
  result.bit( 8) = fifo.full();
  result.bit( 9) = fifo.empty();
  result.bit(10) = 1;  //constants (bits 10-15)
  result.bit(11) = 0;  //todo: should these bits be open bus instead?
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
  if(command.latch) {
    command.latch = 0;

    command.address.bit(14,16) = data.bit(0,2);
    command.target.bit(2,3)    = data.bit(4,5);
    command.ready              = data.bit(6) | command.target.bit(0);
    command.pending           |= data.bit(7) & dma.enable;

    prefetch.read(command.target, command.address);

    dma.wait = dma.mode == 2;
    dma.synchronize();
    return;
  }

  command.target.bit(0,1)   = data.bit(14,15);
  command.ready             = 1;

  //command write (hi)
  if(data.bit(14,15) != 2) {
    command.address.bit(0,13) = data.bit(0,13);
    command.latch = 1;
    return;
  }

  debugger.io(n5(data >> 8), n8(data));

  //register write (d13 is ignored)
  switch(data.bit(8,12)) {

  //mode register 1
  case 0x00: {
    io.displayOverlayEnable  = data.bit(0);
    if(!io.counterLatch && data.bit(1)) state.counterLatchValue = read(1,1,0xC00008,0);
    io.counterLatch          = data.bit(1);
    io.videoMode4            = data.bit(2);
    irq.hblank.enable        = data.bit(4);
    io.leftColumnBlank       = data.bit(5);

    irq.poll();
    if(!io.videoMode4) debug(unimplemented, "[VDP] M4=0");
    return;
  }

  //mode register 2
  case 0x01: {
    io.videoMode5      = data.bit(2);
    io.overscan        = data.bit(3);
    dma.enable         = data.bit(4);
    irq.vblank.enable  = data.bit(5);
    io.displayEnable   = data.bit(6);
    vram.mode          = data.bit(7);

    irq.poll();
    if(!io.videoMode5) debug(unimplemented, "[VDP] M5=0");
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
    irq.hblank.frequency = data.bit(0,7);
    return;
  }

  //mode register 3
  case 0x0b: {
    layers.hscrollMode  = data.bit(0,1);
    layers.vscrollMode  = data.bit(2);
    irq.external.enable = data.bit(3);

    irq.poll();
    return;
  }

  //mode register 4
  case 0x0c: {
    io.displayWidth          = data.bit(0);
    io.interlaceMode         = data.bit(1,2);
    io.shadowHighlightEnable = data.bit(3);
    io.externalColorEnable   = data.bit(4);
    io.hsync                 = data.bit(5);
    io.vsync                 = data.bit(6);
    io.clockSelect           = data.bit(7);
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
    command.increment = data.bit(0,7);
    return;
  }

  //layer size
  case 0x10: {
    layers.nametableWidth  = data.bit(0,1);
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
    dma.mode              = data.bit(6,7);
    return;
  }

  //unused
  default: {
    return;
  }

  }
}
