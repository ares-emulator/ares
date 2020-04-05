auto VDC::hsync() -> void {
  timing.hstate = HDS;
  timing.hoffset = 0;

  latch.horizontalDisplayStart = max(2, timing.horizontalDisplayStart) + 1 << 3;
  latch.horizontalDisplayWidth = timing.horizontalDisplayWidth + 1 << 3;
  latch.horizontalDisplayEnd = timing.horizontalDisplayEnd + 1 << 3;
  latch.horizontalSyncWidth = timing.horizontalSyncWidth + 1 << 3;

  background.latch.characterMode = background.characterMode;
}

auto VDC::vsync() -> void {
  timing.vstate = VSW;
  timing.voffset = 0;

  latch.verticalSyncWidth = timing.verticalSyncWidth + 1;
  latch.verticalDisplayStart = timing.verticalDisplayStart + 2;
  latch.verticalDisplayWidth = timing.verticalDisplayWidth + 1;
  latch.verticalDisplayEnd = timing.verticalDisplayEnd;

  //ensure that Vblank IRQs always occur each frame
  if(latch.verticalSyncWidth + latch.verticalDisplayStart + latch.verticalDisplayWidth > 261) {
    latch.verticalDisplayWidth = 261 - latch.verticalSyncWidth - latch.verticalDisplayStart;
  }

  latch.burstMode = !background.enable && !sprite.enable;
}

auto VDC::hclock() -> void {
  output = 0x100;  //blanking area backdrop color

  if(timing.vstate == VDW && timing.hstate == HDW && !burstMode()) {
    background.run(timing.hoffset, timing.voffset);
    sprite.run(timing.hoffset, timing.voffset);

    if(sprite.color && sprite.priority) {
      output = sprite.color << 0 | sprite.palette << 4 | 1 << 8;
    } else if(background.color) {
      output = background.color << 0 | background.palette << 4 | 0 << 8;
    } else if(sprite.color) {
      output = sprite.color << 0 | sprite.palette << 4 | 1 << 8;
    } else {
      output = 0x000;  //active display backdrop color
    }
  }

  timing.hoffset++;
  switch(timing.hstate) {
  case HDS:
    if(timing.hoffset >= latch.horizontalDisplayStart) {
      timing.hstate = HDW;
      timing.hoffset = 0;
      background.scanline(timing.voffset);
      sprite.scanline(timing.voffset);
    } break;
  case HDW:
    //this is said to happen 4 cycles before HDW ends;
    //however, anything less than 40 cycles causes severe graphics issues in games
    if(timing.hoffset == latch.horizontalDisplayWidth - 40) {
      if(timing.coincidence++ == io.coincidence) irq.raise(IRQ::Line::Coincidence);
    }
    if(timing.hoffset >= latch.horizontalDisplayWidth) {
      timing.hstate = HDE;
      timing.hoffset = 0;
    } break;
  case HDE:
    if(timing.hoffset >= latch.horizontalDisplayEnd) {
      timing.hstate = HSW;
      timing.hoffset = 0;
    } break;
  case HSW:
    if(timing.hoffset >= latch.horizontalSyncWidth) {
      timing.hstate = HDS;
      timing.hoffset = 0;
    } break;
  }
}

auto VDC::vclock() -> void {
  timing.voffset++;
  switch(timing.vstate) {
  case VSW:
    if(timing.voffset >= latch.verticalSyncWidth) {
      timing.vstate = VDS;
      timing.voffset = 0;
    } break;
  case VDS:
    if(timing.voffset >= latch.verticalDisplayStart) {
      timing.vstate = VDW;
      timing.voffset = 0;
      timing.coincidence = 64;
    } break;
  case VDW:
    if(timing.voffset >= latch.verticalDisplayWidth) {
      timing.vstate = VCR;
      timing.voffset = 0;
      background.latch.vramMode = background.vramMode;
      sprite.latch.vramMode = sprite.vramMode;
      irq.raise(IRQ::Line::Vblank);
      dma.satbStart();
    } break;
  case VCR:
    if(timing.voffset >= latch.verticalDisplayEnd) {
      timing.vstate = VSW;
      timing.voffset = 0;
    } break;
  }
}

auto VDC::read(uint2 address) -> uint8 {
  if(!burstMode()) cpu.idle();  //penalty cycle
  uint8 data = 0x00;

  if(address == 0x0) {
    //SR
    data.bit(0) = irq.collision.pending;
    data.bit(1) = irq.overflow.pending;
    data.bit(2) = irq.coincidence.pending;
    data.bit(3) = irq.transferSATB.pending;
    data.bit(4) = irq.transferVRAM.pending;
    data.bit(5) = irq.vblank.pending;
    data.bit(6) = 0;  //busy
    irq.lower();
    return data;
  }

  if(address == 0x1) {
    //unmapped
    return data;
  }

  uint1 a0 = address.bit(0);

  if(io.address == 0x02) {
    //VRR
    data = vram.dataRead.byte(a0);
    if(a0 == 1) {
      vram.addressRead += vram.addressIncrement;
      while(dma.vramActive || dma.satbActive) cpu.idle();
      vram.dataRead = vram.read(vram.addressRead);
    }
    return data;
  }

  return data;
}

auto VDC::write(uint2 address, uint8 data) -> void {
  if(!burstMode()) cpu.idle();  //penalty cycle

  if(address == 0x0) {
    //AR
    io.address = data.bit(0,4);
    return;
  }

  if(address == 0x1) {
    //unmapped
    return;
  }

  uint1 a0 = address.bit(0);

  if(io.address == 0x00) {
    //MAWR
    vram.addressWrite.byte(a0) = data;
    return;
  }

  if(io.address == 0x01) {
    //MARR
    vram.addressRead.byte(a0) = data;
    while(dma.vramActive || dma.satbActive) cpu.idle();
    vram.dataRead = vram.read(vram.addressRead);
    return;
  }

  if(io.address == 0x02) {
    //VWR
    vram.dataWrite.byte(a0) = data;
    if(a0 == 1) {
      while(dma.vramActive || dma.satbActive) cpu.idle();
      vram.write(vram.addressWrite, vram.dataWrite);
      vram.addressWrite += vram.addressIncrement;
    }
    return;
  }

  if(io.address == 0x05) {
    //CR
    if(a0 == 0) {
      irq.collision.enable   = data.bit(0);
      irq.overflow.enable    = data.bit(1);
      irq.coincidence.enable = data.bit(2);
      irq.vblank.enable      = data.bit(3);
      io.externalSync        = data.bit(4,5);
      sprite.enable          = data.bit(6);
      background.enable      = data.bit(7);
      irq.poll();
    }
    if(a0 == 1) {
      io.displayOutput = data.bit(0,1);
      io.dramRefresh   = data.bit(2);
      if(data.bit(3,4) == 0) vram.addressIncrement = 0x01;
      if(data.bit(3,4) == 1) vram.addressIncrement = 0x20;
      if(data.bit(3,4) == 2) vram.addressIncrement = 0x40;
      if(data.bit(3,4) == 3) vram.addressIncrement = 0x80;
    }
    return;
  }

  if(io.address == 0x06) {
    //RCR
    if(a0 == 0) io.coincidence.bit(0,7) = data.bit(0,7);
    if(a0 == 1) io.coincidence.bit(8,9) = data.bit(0,1);
    return;
  }

  if(io.address == 0x07) {
    //BXR
    if(a0 == 0) background.hscroll.bit(0,7) = data.bit(0,7);
    if(a0 == 1) background.hscroll.bit(8,9) = data.bit(0,1);
    return;
  }

  if(io.address == 0x08) {
    //BYR
    if(a0 == 0) background.vscroll.bit(0,7) = data.bit(0,7);
    if(a0 == 1) background.vscroll.bit(8)   = data.bit(0);
    background.vcounter = background.vscroll;  //updated on both even and odd writes
    return;
  }

  if(io.address == 0x09) {
    //MWR
    if(a0 == 1) return;
    background.vramMode = data.bit(0,1);
    sprite.vramMode     = data.bit(2,3);
    if(data.bit(4,5) == 0) background.width =  32;
    if(data.bit(4,5) == 1) background.width =  64;
    if(data.bit(4,5) == 2) background.width = 128;
    if(data.bit(4,5) == 3) background.width = 128;
    if(data.bit(6) == 0) background.height = 32;
    if(data.bit(6) == 1) background.height = 64;
    background.characterMode = data.bit(7);
    return;
  }

  if(io.address == 0x0a) {
    //HSR
    if(a0 == 0) timing.horizontalSyncWidth    = data.bit(0,4);
    if(a0 == 1) timing.horizontalDisplayStart = data.bit(0,6);
    return;
  }

  if(io.address == 0x0b) {
    //HDR
    if(a0 == 0) timing.horizontalDisplayWidth = data.bit(0,6);
    if(a0 == 1) timing.horizontalDisplayEnd   = data.bit(0,6);
    return;
  }

  if(io.address == 0x0c) {
    //VPR
    if(a0 == 0) timing.verticalSyncWidth    = data.bit(0,4);
    if(a0 == 1) timing.verticalDisplayStart = data.bit(0,7);
    return;
  }

  if(io.address == 0x0d) {
    //VDR
    if(a0 == 0) timing.verticalDisplayWidth.bit(0,7) = data.bit(0,7);
    if(a0 == 1) timing.verticalDisplayWidth.bit(8)   = data.bit(0);
    return;
  }

  if(io.address == 0x0e) {
    //VCR
    if(a0 == 0) timing.verticalDisplayEnd = data.bit(0,7);
    return;
  }

  if(io.address == 0x0f) {
    //DCR
    if(a0 == 1) return;
    irq.transferSATB.enable = data.bit(0);
    irq.transferVRAM.enable = data.bit(1);
    dma.sourceIncrementMode = data.bit(2);
    dma.targetIncrementMode = data.bit(3);
    dma.satbRepeat          = data.bit(4);
    irq.poll();
    return;
  }

  if(io.address == 0x10) {
    //SOUR
    dma.source.byte(a0) = data;
    return;
  }

  if(io.address == 0x11) {
    //DESR
    dma.target.byte(a0) = data;
    return;
  }

  if(io.address == 0x12) {
    //LENR
    dma.length.byte(a0) = data;
    if(a0 == 1) dma.vramStart();
    return;
  }

  if(io.address == 0x13) {
    //DVSSR
    dma.satbSource.byte(a0) = data;
    dma.satbQueue();
    return;
  }
}

auto VDC::power() -> void {
  random.entropy(Random::Entropy::High);

  output = 0;

  for(auto& data : vram.memory) data = 0;
  vram.addressRead = 0;
  vram.addressWrite = 0;
  vram.addressIncrement = 0x01;
  vram.dataRead = 0;
  vram.dataWrite = 0;
  for(auto& data : satb.memory) data = random();
  irq = {};
  dma = {};
  timing = {};
  background = {};
  sprite = {};

  dma.vdc = *this;
  background.vdc = *this;
  sprite.vdc = *this;
}

inline auto VDC::VRAM::read(uint16 address) const -> uint16 {
  if(address.bit(15)) return 0x0000;  //todo: random data?
  return memory[address];
}

inline auto VDC::VRAM::write(uint16 address, uint16 data) -> void {
  if(address.bit(15)) return;
  memory[address] = data;
}

inline auto VDC::SATB::read(uint8 address) const -> uint16 {
  return memory[address];
}

inline auto VDC::SATB::write(uint8 address, uint16 data) -> void {
  memory[address] = data;
}
