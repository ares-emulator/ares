auto CPU::read(n1 upper, n1 lower, n24 address, n16 data) -> n16 {
  //BIOS vectors
  if(address <= 0x00007f && io.vectorSelect == 0) {
    return system.bios[address >> 1];
  }

  //cartridge program ROM
  if(address <= 0x0fffff) {
    return cartridge.prom[address >> 1];
  }

  //work RAM
  if(address <= 0x1fffff) {
    return system.wram[address >> 1];
  }

  //cartridge program ROM (banked)
  if(address <= 0x2fffff) {
    address = 0x100000 | n20(address);
    return cartridge.prom[address >> 1];
  }

  //I/O registers
  if(address <= 0x3fffff) {
    return readIO(upper, lower, address, data);
  }

  //palette RAM
  if(address <= 0x7fffff) {
    address = gpu.io.pramBank << 13 | n13(address);
    return gpu.pram[address >> 1];
  }

  //memory card
  if(address <= 0xbfffff) {
    return data;
  }

  //BIOS
  if(address <= 0xcfffff) {
    return system.bios[address >> 1];
  }

  //backup RAM (MVS only)
  if(address <= 0xdfffff) {
    if(Model::NeoGeoMVS()) return system.sram[address >> 1];
    return data;
  }

  //CD-ROM
  if(address <= 0xffffff) {
    return data;
  }

  return data;
}

auto CPU::write(n1 upper, n1 lower, n24 address, n16 data) -> void {
  //cartridge program ROM (read-only)
  if(address <= 0x0fffff) {
    return;
  }

  //work RAM
  if(address <= 0x1fffff) {
    if(upper) system.wram[address >> 1].byte(1) = data.byte(1);
    if(lower) system.wram[address >> 1].byte(0) = data.byte(0);
    return;
  }

  //cartridge program ROM (banked) (read-only)
  if(address <= 0x2fffff) {
    return;
  }

  //I/O registers
  if(address <= 0x3fffff) {
    return writeIO(upper, lower, address, data);
  }

  //palette RAM
  if(address <= 0x7fffff) {
    address = gpu.io.pramBank << 13 | n13(address);
    gpu.pram[address >> 1] = data;
    return;
  }

  //memory card
  if(address <= 0xbfffff) {
    return;
  }

  //BIOS
  if(address <= 0xcfffff) {
    return;
  }

  //backup RAM (MVS only)
  if(address <= 0xdfffff) {
    if(Model::NeoGeoMVS()) system.sram[address >> 1] = data;
    return;
  }

  //CD-ROM
  if(address <= 0xffffff) {
    return;
  }
}

auto CPU::readIO(n1 upper, n1 lower, n24 address, n16 data) -> n16 {
  //REG_P1CNT
  if(address == 0x300000 && upper) {
    data.byte(1) = controllerPort1.readButtons();
  }

  //REG_DIPSW
  if(address == 0x300000 && lower) {
    data.bit(0)   = 0;  //settings mode
    data.bit(1)   = 0;  //0 = 1 chute; 1 = 2 chutes
    data.bit(2)   = 0;  //0 = normal controller; 1 = mahjong keyboard
    data.bit(3,4) = 0;  //communication ID code
    data.bit(5)   = 0;  //enable multiplayer
    data.bit(6)   = 0;  //freeplay
    data.bit(7)   = 0;  //freeze
  }

  //REG_SYSTYPE
  if(address == 0x300080 && lower) {
    data.bit(6) = 0;  //0 = 2 slots; 1 = 4 or 6 slots
    data.bit(7) = 1;  //test button
  }

  //REG_SOUND
  if(address == 0x320000 && upper) {
    data.byte(1) = apu.communication.output;
  }

  //REG_STATUS_A
  if(address == 0x320000 && lower) {
    data.bit(0) = 0;  //coin 1
    data.bit(1) = 0;  //coin 2
    data.bit(2) = 1;  //service button
    data.bit(3) = 0;  //coin 3
    data.bit(4) = 0;  //coin 4
    data.bit(5) = 0;  //0 = 4-slot; 1 = 6-slot
    data.bit(6) = 0;  //RTC time pulse
    data.bit(7) = 0;  //RTC data bit
  }

  //REG_P2CNT
  if(address == 0x340000 && upper) {
    data.byte(1) = controllerPort2.readButtons();
  }

  //REG_STATUS_B
  if(address == 0x380000 && upper) {
    data.bit( 8, 9) = controllerPort1.readControls();
    data.bit(10,11) = controllerPort2.readControls();
    data.bit(12,13) = 0b11;  //0b00 = memory card inserted
    data.bit(14)    = system.io.memoryCardLock != 0;
    data.bit(15)    = Model::NeoGeoMVS();  //0 = AES; 1 = MVS
  }

  //REG_VRAMADDR
  if(address == 0x3c0000 || address == 0x3c0008) {
    data = gpu.vram[gpu.io.vramAddress];
  }

  //REG_VRAMRW
  if(address == 0x3c0002 || address == 0x3c000a) {
    data = gpu.vram[gpu.io.vramAddress];
  }

  //REG_VRAMMOD
  if(address == 0x3c0004 || address == 0x3c000c) {
    data = gpu.io.vramIncrement;
  }

  //REG_LSPCMODE
  if(address == 0x3c0006 || address == 0x3c000e) {
    data.bit(0, 2) = gpu.io.autoAnimationCounter;
    data.bit(3)    = 0;  //0 = 60hz; 1 = 50hz
    data.bit(4, 6) = 0;  //unused
    data.bit(7,15) = gpu.io.vcounter + 248;
  }

  return data;
}

auto CPU::writeIO(n1 upper, n1 lower, n24 address, n16 data) -> void {
  //REG_DIPSW
  if(address == 0x300000 && lower) {
    //todo: kick watchdog
  }

  //REG_SOUND
  if(address == 0x320000 && upper) {
    apu.communication.input = data.byte(1);
    apu.io.nmiLine = 1;
  }

  //REG_POUTPUT
  if(address == 0x380000 && lower) {
    controllerPort1.writeOutputs(data.bit(0,2));
    controllerPort2.writeOutputs(data.bit(3,5));
  }

  //REG_CRDBANK
  if(address == 0x380010 && lower) {
    system.io.memoryCardBank = data.bit(0,2);
  }

  //REG_POUTPUT (mirror) (AES only)
  //REG_SLOT (MVS only)
  if(address == 0x380020 && lower) {
    if(Model::NeoGeoAES()) {
      controllerPort1.writeOutputs(data.bit(0,2));
      controllerPort2.writeOutputs(data.bit(3,5));
    }
    if(Model::NeoGeoMVS()) {
      system.io.slotSelect = data.bit(0,2);
    }
  }

  //REG_LEDLATCHES
  if(address == 0x380030 && lower) {
    system.io.ledMarquee = data.bit(3);
    system.io.ledLatch1  = data.bit(4);
    system.io.ledLatch2  = data.bit(5);
  }

  //REG_LEDDATA
  if(address == 0x380040 && lower) {
    system.io.ledData = data.bit(0,7);
  }

  //REG_RTCCTRL
  if(address == 0x380050 && lower) {
    //rtc.din    = data.bit(0);
    //rtc.clock  = data.bit(1);
    //rtc.strobe = data.bit(2);
  }

  //REG_RESETCC1
  if(address == 0x380060 && lower) {
    //todo: float coin counter 1
  }

  //REG_RESETCC2
  if(address == 0x380062 && lower) {
    //todo: float coin counter 2
  }

  //REG_RESETCL1
  if(address == 0x380064 && lower) {
    //todo: float coin lockout 1
  }

  //REG_RESETCL2
  if(address == 0x380066 && lower) {
    //todo: float coin lockout 2
  }

  //REG_SETCC1
  if(address == 0x3800e0 && lower) {
    //todo: sink coin counter 1
  }

  //REG_SETCC2
  if(address == 0x3800e2 && lower) {
    //todo: sink coin counter 2
  }

  //REG_SETCL1
  if(address == 0x3800e4 && lower) {
    //todo: sink coin lockout 1
  }

  //REG_SETCL2
  if(address == 0x3800e6 && lower) {
    //todo: sink coin lockout 2
  }

  //REG_NOSHADOW
  if(address == 0x3a0000 && lower) {
    gpu.io.shadow = 0;
  }

  //REG_SWPBIOS
  if(address == 0x3a0002 && lower) {
    io.vectorSelect = 0;
  }

  //REG_CRDUNLOCK1
  if(address == 0x3a0004 && lower) {
    system.io.memoryCardLock.bit(0) = 0;
  }

  //REG_CRDLOCK2
  if(address == 0x3a0006 && lower) {
    system.io.memoryCardLock.bit(1) = 1;
  }

  //REG_CRDREGSEL
  if(address == 0x3a0008 && lower) {
    system.io.memoryCardRegisterSelect = 1;
  }

  //REG_BRDFIX
  if(address == 0x3a000a && lower) {
    io.fixSelect = 0;
  }

  //REG_SRAMLOCK
  if(address == 0x3a000c && lower) {
    system.io.sramLock = 1;
  }

  //REG_PALBANK1
  if(address == 0x3a000e && lower) {
    gpu.io.pramBank = 1;
  }

  //REG_SHADOW
  if(address == 0x3a0010 && lower) {
    gpu.io.shadow = 1;
  }

  //REG_SWPROM
  if(address == 0x3a0012 && lower) {
    io.vectorSelect = 1;
  }

  //REG_CRDLOCK1
  if(address == 0x3a0014 && lower) {
    system.io.memoryCardLock.bit(0) = 1;
  }

  //REG_CRDUNLOCK2
  if(address == 0x3a0016 && lower) {
    system.io.memoryCardLock.bit(1) = 0;
  }

  //REG_CRDNORMAL
  if(address == 0x3a0018 && lower) {
    system.io.memoryCardRegisterSelect = 0;
  }

  //REG_CRTFIX
  if(address == 0x3a001a && lower) {
    io.fixSelect = 1;
  }

  //REG_SRAMUNLOCK
  if(address == 0x3a001c && lower) {
    system.io.sramLock = 0;
  }

  //REG_PALBANK0
  if(address == 0x3a001e && lower) {
    gpu.io.pramBank = 0;
  }

  //REG_VRAMADDR
  if(address == 0x3c0000) {
    if(upper) gpu.io.vramAddress.byte(1) = data.byte(1);
    if(lower) gpu.io.vramAddress.byte(0) = data.byte(0);
  }

  //REG_VRAMRW
  if(address == 0x3c0002) {
    if(upper) gpu.vram[gpu.io.vramAddress].byte(1) = data.byte(1);
    if(lower) gpu.vram[gpu.io.vramAddress].byte(0) = data.byte(0);
    gpu.io.vramAddress.bit(0,14) += gpu.io.vramIncrement;
  }

  //REG_VRAMMOD
  if(address == 0x3c0004) {
    if(upper) gpu.io.vramIncrement.byte(1) = data.byte(1);
    if(lower) gpu.io.vramIncrement.byte(0) = data.byte(0);
  }

  //REG_LSPCMODE
  if(address == 0x3c0006) {
    if(lower) {
      gpu.io.autoAnimationDisable = data.bit(3);
      gpu.io.timerInterruptEnable = data.bit(4);
      gpu.io.timerReloadOnChange  = data.bit(5);
      gpu.io.timerReloadOnVblank  = data.bit(6);
      gpu.io.timerReloadOnZero    = data.bit(7);
    }
    if(upper) {
      gpu.io.autoAnimationSpeed   = data.bit(8,15);
    }
  }

  //REG_TIMERHIGH
  if(address == 0x3c0008) {
    if(upper) gpu.io.timerReload.byte(3) = data.byte(1);
    if(lower) gpu.io.timerReload.byte(2) = data.byte(0);
  }

  //REG_TIMERLOW
  if(address == 0x3c000a) {
    if(upper) gpu.io.timerReload.byte(1) = data.byte(1);
    if(lower) gpu.io.timerReload.byte(0) = data.byte(0);
    if(gpu.io.timerReloadOnChange) {
      gpu.io.timerCounter = gpu.io.timerReload;
    }
  }

  //REG_IRQACK
  if(address == 0x3c000c) {
    if(lower) {
      gpu.irq.powerAcknowledge  = data.bit(0);
      gpu.irq.timerAcknowledge  = data.bit(1);
      gpu.irq.vblankAcknowledge = data.bit(2);
    }
  }

  //REG_TIMERSTOP
  if(address == 0x3c000e) {
    if(lower) {
      gpu.io.timerStopPAL = data.bit(0);
    }
  }

  return;
}
