auto CPU::read(n1 upper, n1 lower, n24 address, n16 data) -> n16 {
  //NEO-E0
  if(io.vectorSelect == 0) {
    if((address & 0xffff80) == 0x000000 || (address & 0xffff80) == 0xc00000) {
      address ^= 0xc00000;  //swap BIOS and cartridge interrupt vectors
    }
  }

  if(auto result = platform->cheat(address)) return *result;

  if(!Model::NeoGeoCD()) {
    //cartridge program ROM
    if (address <= 0x0fffff) {
      return cartridge.readP(upper, lower, address, data);
    }

    //work RAM
    if (address <= 0x1fffff) {
      return system.wram[address >> 1];
    }

    //cartridge program ROM (banked)
    if (address <= 0x2fffff) {
      return cartridge.readP(upper, lower, address, data);
    }
  } else if (address <= 0x1fffff) {
    return system.wram[address >> 1];
  }

  //I/O registers
  if(address <= 0x3fffff) {
    return readIO(upper, lower, address, data);
  }

  //palette RAM
  if(address <= 0x7fffff) {
    address = lspc.io.pramBank << 13 | n13(address);
    return lspc.pram[address >> 1];
  }

  //memory card
  if(address <= 0xbfffff) {
    data.byte(0) = 0xff;
    data.byte(1) = cardSlot.read(address >> 1);
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

  if(Model::NeoGeoCD()) {
    if (address <= 0xefffff) {
      switch (system.io.uploadZone) {
        case 0: return system.spriteRam[(address & 0xfffff) >> 1];
        case 1: return system.pcmRam[(address & 0xfffff) >> 1];
        case 4: return apu.ram[(address & 0x1ffff) + upper];
        case 5: return system.fixRam[(address & 0x3ffff) >> 1];
      }
    }

    //CD-ROM
    if (address <= 0xffffff) {
      return readIO(upper, lower, address, data);
    }
  }

  return data;
}

auto CPU::write(n1 upper, n1 lower, n24 address, n16 data) -> void {
  if(!Model::NeoGeoCD()) {
    //cartridge program ROM
    if (address <= 0x0fffff) {
      return cartridge.writeP(upper, lower, address, data);
    }

    //work RAM
    if (address <= 0x1fffff) {
      if (upper) system.wram[address >> 1].byte(1) = data.byte(1);
      if (lower) system.wram[address >> 1].byte(0) = data.byte(0);
      return;
    }

    //cartridge program ROM (banked)
    if (address <= 0x2fffff) {
      return cartridge.writeP(upper, lower, address, data);
    }
  } else if (address <= 0x1fffff) {
    if (upper) system.wram[address >> 1].byte(1) = data.byte(1);
    if (lower) system.wram[address >> 1].byte(0) = data.byte(0);
    return;
  }

  //I/O registers
  if(address <= 0x3fffff) {
    return writeIO(upper, lower, address, data);
  }

  //palette RAM
  if(address <= 0x7fffff) {
    address = lspc.io.pramBank << 13 | n13(address);
    lspc.pram[address >> 1] = data;
    return;
  }

  //memory card
  if(address <= 0xbfffff) {
    if(lower) cardSlot.write(address >> 1, data);
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

  if(Model::NeoGeoCD()) {
    if(address <= 0xefffff) {
      switch(system.io.uploadZone) {
        case 0: address &= 0xfffff;
                address.bit(20, 21) = system.io.spriteUploadBank;
                if(upper) system.spriteRam[(address) >> 1].byte(1) = data.byte(1);
                if(lower) system.spriteRam[(address) >> 1].byte(0) = data.byte(0);
                return;
        case 1: address &= 0xfffff;
                address.bit(19) = system.io.pcmUploadBank;
                address >>= 1;
                if(lower) system.pcmRam[address] = data.byte(0);
                return;
        case 4: address >>= 1;
                if(lower) apu.ram[(address)] = data.byte(0);
                return;
        case 5: address >>= 1;
                if(lower) system.fixRam[address].byte(0) = data.byte(0);
                return;
      }
    }

    //CD-ROM
    if (address <= 0xffffff) {
      return writeIO(upper, lower, address, data);
    }
  }
}

auto CPU::readIO(n1 upper, n1 lower, n24 address, n16 data) -> n16 {
  //REG_P1CNT
  if((address & 0xfe0000) == 0x300000 && upper) {
    data.byte(1) = controllerPort1.readButtons();
    return data;
  }

  //REG_DIPSW
  if((address & 0xfe0080) == 0x300000 && lower) {
    data.bit(0)   = 1;  //settings mode
    data.bit(1)   = 1;  //0 = 1 chute; 1 = 2 chutes (TODO: Verify)
    data.bit(2)   = 1;  //1 = normal controller; 0 = mahjong keyboard
    data.bit(3,4) = 1;  //communication ID code
    data.bit(5)   = 1;  //enable multiplayer
    data.bit(6)   = 1;  //freeplay
    data.bit(7)   = 1;  //freeze
    return data;
  }

  //REG_SYSTYPE
  if((address & 0xfe0080) == 0x300080 && lower) {
    data.bit(6) = 0;  //0 = 2 slots; 1 = 4 or 6 slots
    data.bit(7) = 1;  //test button
    return data;
  }

  //REG_SOUND
  if((address & 0xfe0000) == 0x320000 && upper) {
    data.byte(1) = apu.communication.output;
    return data;
  }

  //REG_STATUS_A
  if((address & 0xfe0000) == 0x320000 && lower) {
    data.bit(0) = Model::NeoGeoMVS();  //coin 1 (MVS: active low, AES: always 0)
    data.bit(1) = Model::NeoGeoMVS();  //coin 2 (MVS: active low, AES: always 0)
    data.bit(2) = 1;  //service button
    data.bit(3) = Model::NeoGeoMVS();  //coin 3 (MVS: active low, AES: always 0)
    data.bit(4) = Model::NeoGeoMVS();  //coin 3 (MVS: active low, AES: always 0)
    data.bit(5) = 0;  //0 = 4-slot; 1 = 6-slot
    data.bit(6) = 0;  //RTC time pulse
    data.bit(7) = 0;  //RTC data bit
    return data;
  }

  //REG_P2CNT
  if((address & 0xfe0000) == 0x340000 && upper) {
    data.byte(1) = controllerPort2.readButtons();
    return data;
  }

  //REG_STATUS_B
  if((address & 0xfe0000) == 0x380000 && upper) {
    data.bit( 8, 9) = controllerPort1.readControls();
    data.bit(10,11) = controllerPort2.readControls();
    data.bit(12,13) = 0b11;  //0b00 = memory card inserted
    data.bit(14)    = cardSlot.lock != 0;
    data.bit(15)    = Model::NeoGeoMVS();  //0 = AES; 1 = MVS
    return data;
  }

  //REG_VRAMADDR
  if((address & 0xfe0006) == 0x3c0000) {
    return data = lspc.vram[lspc.io.vramAddress];;
  }

  //REG_VRAMRW
  if((address & 0xfe0006) == 0x3c0002) {
    return data = lspc.vram[lspc.io.vramAddress];
  }

  //REG_VRAMMOD
  if((address & 0xfe0006) == 0x3c0004) {
    return data = lspc.io.vramIncrement;
  }

  //REG_LSPCMODE
  if((address & 0xfe0006) == 0x3c0006) {
    data.bit(0, 2) = lspc.animation.frame;
    data.bit(3)    = 0;  //0 = 60hz; 1 = 50hz
    data.bit(4, 6) = 0;  //unused
    data.bit(7,15) = lspc.io.vcounter + 248;
    return data;
  }

  debug(unimplemented, "readIO: ", hex(address, 6L));
  return data;
}

auto CPU::writeIO(n1 upper, n1 lower, n24 address, n16 data) -> void {
  //REG_DIPSW
  if((address & 0xfe0080) == 0x300000 && lower) {
    //todo: kick watchdog
    return;
  }

  //REG_SOUND
  if((address & 0xfe0000) == 0x320000 && upper) {
    apu.communication.input = data.byte(1);
    apu.nmi.pending = 1;
    return;
  }

  //REG_POUTPUT
  if((address & 0xfe0070) == 0x380000 && lower) {
    controllerPort1.writeOutputs(data.bit(0,2));
    controllerPort2.writeOutputs(data.bit(3,5));
    return;
  }

  //REG_CRDBANK
  if((address & 0xfe0070) == 0x380010 && lower) {
    cardSlot.bank = data.bit(0,2);
    return;
  }

  //REG_POUTPUT (mirror) (AES only)
  //REG_SLOT (MVS only)
  if((address & 0xfe00f0) == 0x380020 && lower) {
    if(Model::NeoGeoAES()) {
      controllerPort1.writeOutputs(data.bit(0,2));
      controllerPort2.writeOutputs(data.bit(3,5));
    }
    if(Model::NeoGeoMVS()) {
      system.io.slotSelect = data.bit(0,2);
    }
    return;
  }

  //REG_LEDLATCHES
  if((address & 0xfe00f0) == 0x380030 && lower) {
    system.io.ledMarquee = data.bit(3);
    system.io.ledLatch1  = data.bit(4);
    system.io.ledLatch2  = data.bit(5);
    return;
  }

  //REG_LEDDATA
  if((address & 0xfe00f0) == 0x380040 && lower) {
    system.io.ledData = data.bit(0,7);
    return;
  }

  //REG_RTCCTRL
  if((address & 0xfe00f0) == 0x380050 && lower) {
    //rtc.din    = data.bit(0);
    //rtc.clock  = data.bit(1);
    //rtc.strobe = data.bit(2);
    return;
  }

  //REG_RESETCC1
  if((address & 0xfe00f6) == 0x380060 && lower) {
    //todo: float coin counter 1
    return;
  }

  //REG_RESETCC2
  if((address & 0xfe00f6) == 0x380062 && lower) {
    //todo: float coin counter 2
    return;
  }

  //REG_RESETCL1
  if((address & 0xfe00f6) == 0x380064 && lower) {
    //todo: float coin lockout 1
    return;
  }

  //REG_RESETCL2
  if((address & 0xfe00f6) == 0x380066 && lower) {
    //todo: float coin lockout 2
    return;
  }

  //REG_SETCC1
  if((address & 0xfe00f6) == 0x3800e0 && lower) {
    //todo: sink coin counter 1
    return;
  }

  //REG_SETCC2
  if((address & 0xfe00f6) == 0x3800e2 && lower) {
    //todo: sink coin counter 2
    return;
  }

  //REG_SETCL1
  if((address & 0xfe00f6) == 0x3800e4 && lower) {
    //todo: sink coin lockout 1
    return;
  }

  //REG_SETCL2
  if((address & 0xfe00f6) == 0x3800e6 && lower) {
    //todo: sink coin lockout 2
    return;
  }

  //REG_NOSHADOW
  if((address & 0xfe001e) == 0x3a0000 && lower) {
    lspc.io.shadow = 0;
    return;
  }

  //REG_SWPBIOS
  if((address & 0xfe001e) == 0x3a0002 && lower) {
    io.vectorSelect = 0;
    return;
  }

  //REG_CRDUNLOCK1
  if((address & 0xfe001e) == 0x3a0004 && lower) {
    cardSlot.lock.bit(0) = 0;
    return;
  }

  //REG_CRDLOCK2
  if((address & 0xfe001e) == 0x3a0006 && lower) {
    cardSlot.lock.bit(1) = 1;
    return;
  }

  //REG_CRDREGSEL
  if((address & 0xfe001e) == 0x3a0008 && lower) {
    cardSlot.select = 1;
    return;
  }

  //REG_BRDFIX
  if((address & 0xfe001e) == 0x3a000a && lower) {
    io.fixSelect = 0;
    return;
  }

  //REG_SRAMLOCK
  if((address & 0xfe001e) == 0x3a000c && lower) {
    system.io.sramLock = 1;
    return;
  }

  //REG_PALBANK1
  if((address & 0xfe001e) == 0x3a000e && lower) {
    lspc.io.pramBank = 1;
    return;
  }

  //REG_SHADOW
  if((address & 0xfe001e) == 0x3a0010 && lower) {
    lspc.io.shadow = 1;
    return;
  }

  //REG_SWPROM
  if((address & 0xfe001e) == 0x3a0012 && lower) {
    io.vectorSelect = 1;
    return;
  }

  //REG_CRDLOCK1
  if((address & 0xfe001e) == 0x3a0014 && lower) {
    cardSlot.lock.bit(0) = 1;
    return;
  }

  //REG_CRDUNLOCK2
  if((address & 0xfe001e) == 0x3a0016 && lower) {
    cardSlot.lock.bit(1) = 0;
    return;
  }

  //REG_CRDNORMAL
  if((address & 0xfe001e) == 0x3a0018 && lower) {
    cardSlot.select = 0;
    return;
  }

  //REG_CRTFIX
  if((address & 0xfe001e) == 0x3a001a && lower) {
    io.fixSelect = 1;
    return;
  }

  //REG_SRAMUNLOCK
  if((address & 0xfe001e) == 0x3a001c && lower) {
    system.io.sramLock = 0;
    return;
  }

  //REG_PALBANK0
  if((address & 0xfe001e) == 0x3a001e && lower) {
    lspc.io.pramBank = 0;
    return;
  }

  //REG_VRAMADDR
  if((address & 0xfe000e) == 0x3c0000) {
    if(upper) lspc.io.vramAddress.byte(1) = data.byte(1);
    if(lower) lspc.io.vramAddress.byte(0) = data.byte(0);
    return;
  }

  //REG_VRAMRW
  if((address & 0xfe000e) == 0x3c0002) {
    if(upper) lspc.vram[lspc.io.vramAddress].byte(1) = data.byte(1);
    if(lower) lspc.vram[lspc.io.vramAddress].byte(0) = data.byte(0);
    lspc.io.vramAddress.bit(0,14) += lspc.io.vramIncrement;
    return;
  }

  //REG_VRAMMOD
  if((address & 0xfe000e) == 0x3c0004) {
    if(upper) lspc.io.vramIncrement.byte(1) = data.byte(1);
    if(lower) lspc.io.vramIncrement.byte(0) = data.byte(0);
    return;
  }

  //REG_LSPCMODE
  if((address & 0xfe000e) == 0x3c0006) {
    if(lower) {
      lspc.animation.disable     = data.bit(3);
      lspc.timer.interruptEnable = data.bit(4);
      lspc.timer.reloadOnChange  = data.bit(5);
      lspc.timer.reloadOnVblank  = data.bit(6);
      lspc.timer.reloadOnZero    = data.bit(7);
    }
    if(upper) {
      lspc.animation.speed       = data.bit(8,15);
    }
    return;
  }

  //REG_TIMERHIGH
  if((address & 0xfe000e) == 0x3c0008) {
    if(upper) lspc.timer.reload.byte(3) = data.byte(1);
    if(lower) lspc.timer.reload.byte(2) = data.byte(0);
    return;
  }

  //REG_TIMERLOW
  if((address & 0xfe000e) == 0x3c000a) {
    if(upper) lspc.timer.reload.byte(1) = data.byte(1);
    if(lower) lspc.timer.reload.byte(0) = data.byte(0);
    if(lspc.timer.reloadOnChange) {
      lspc.timer.counter = lspc.timer.reload;
    }
    return;
  }

  //REG_IRQACK
  if((address & 0xfe000e) == 0x3c000c) {
    if(lower) {
      lspc.irq.powerAcknowledge  = data.bit(0);
      lspc.irq.timerAcknowledge  = data.bit(1);
      lspc.irq.vblankAcknowledge = data.bit(2);
    }
    return;
  }

  //REG_TIMERSTOP
  if((address & 0xfe000e) == 0x3c000e) {
    if(lower) {
      lspc.timer.stopPAL = data.bit(0);
    }
    return;
  }

  if(!Model::NeoGeoCD()) return;

  //REG_TRANSAREA
  if((address & 0xfffe) == 0x0104) {
    system.io.uploadZone = data;
    print("upload zone: ", data, " (", system.io.uploadZone, ")\n");
    return;
  }

  //REG_Z80RST
  if((address & 0xfffe) == 0x0182) {
    // TODO: do we need to pay attention to the value here to enable/disable reset or just trigger it
    apu.restart();
    return;
  }

  //REG_SPRBANK
  if((address & 0xfffe) == 0x01a0) {
    system.io.spriteUploadBank = data.bit(0,1);
    return;
  }

  //REG_PCMBANK
  if((address & 0xfffe) == 0x01a2) {
    system.io.pcmUploadBank = data.bit(0);
    return;
  }

  debug(unimplemented, "writeIO: ", hex(address, 6L), " = ", hex(data, 4L));

  return;
}
