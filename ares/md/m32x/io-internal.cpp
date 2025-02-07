auto M32X::readInternalIO(n1 upper, n1 lower, n29 address, n16 data) -> n16 {
  if(shm.active()) shm.internalStep(1); if(shs.active()) shs.internalStep(1);

  //interrupt mask
  if(address == 0x4000) {
    if(shm.active()) {
      data.bit(0) = shm.irq.pwm.enable;
      data.bit(1) = shm.irq.cmd.enable;
      data.bit(2) = shm.irq.hint.enable;
      data.bit(3) = shm.irq.vint.enable;
    }
    if(shs.active()) {
      data.bit(0) = shs.irq.pwm.enable;
      data.bit(1) = shs.irq.cmd.enable;
      data.bit(2) = shs.irq.hint.enable;
      data.bit(3) = shs.irq.vint.enable;
    }
    data.bit( 7) = io.hintVblank;
    data.bit( 8) = !(bool)cartridge.child->pak;  //0 = cartridge connected
    data.bit( 9) = io.adapterEnable;
    data.bit(15) = vdp.framebufferAccess;
  }

  //hcount
  if(address == 0x4004) {
    data.byte(0) = io.hperiod;
    if(shm.active()) shm.syncOtherSh2();
    if(shs.active()) shs.syncOtherSh2();
  }

  //dreq control register
  if(address == 0x4006) {
    data.bit( 0) = dreq.vram;
    data.bit( 1) = dreq.dma;
    data.bit( 2) = dreq.active;
    data.bit(14) = dreq.fifo.empty();
    data.bit(15) = dreq.fifo.full();
  }

  //68K to SH2 DREQ source address
  if(address == 0x4008) {
    data.byte(0) = dreq.source.byte(2);
  }
  if(address == 0x400a) {
    data.byte(1) = dreq.source.byte(1);
    data.byte(0) = dreq.source.byte(0);
  }

  //68K to SH2 DREQ target address
  if(address == 0x400c) {
    data.byte(0) = dreq.target.byte(2);
  }
  if(address == 0x400e) {
    data.byte(1) = dreq.target.byte(1);
    data.byte(0) = dreq.target.byte(0);
  }

  //68K to SH2 DREQ length
  if(address == 0x4010) {
    data.byte(1) = dreq.length.byte(1);
    data.byte(0) = dreq.length.byte(0);
  }

  //FIFO
  if(address == 0x4012) {
    if(shm.active()) { shm.syncM68k(); }
    if(shs.active()) { shs.syncM68k(); }
    data = dreq.fifo.read(data);
    shm.dmac.dreq[0] = !dreq.fifo.empty();
    shs.dmac.dreq[0] = !dreq.fifo.empty();
  }

  //communication
  if(address >= 0x4020 && address <= 0x402f) {
    data = communication[address >> 1 & 7];
    if(shm.active()) { shm.syncAll(); }
    if(shs.active()) { shs.syncAll(); }
  }

  //PWM control
  if(address == 0x4030) {
    data.bit(0,1)  = pwm.lmode;
    data.bit(2,3)  = pwm.rmode;
    data.bit(4)    = pwm.mono;
    data.bit(7)    = pwm.dreqIRQ;
    data.bit(8,11) = pwm.timer;
  }

  //PWM cycle
  if(address == 0x4032) {
    data.bit(0,11) = pwm.cycle;
  }

  //PWM left channel pulse width
  if(address == 0x4034) {
    data.bit(0,12) = pwm.lfifoLatch;
    data.bit(14)   = pwm.lfifo.empty();
    data.bit(15)   = pwm.lfifo.full();
  }

  //PWM right channel pulse width
  if(address == 0x4036) {
    data.bit(0,12) = pwm.rfifoLatch;
    data.bit(14)   = pwm.rfifo.empty();
    data.bit(15)   = pwm.rfifo.full();
  }

  //PWM mono pulse width
  if(address == 0x4038) {
    data.bit(0,12) = pwm.mfifoLatch;
    data.bit(14)   = pwm.lfifo.empty() && pwm.rfifo.empty();
    data.bit(15)   = pwm.lfifo.full()  || pwm.rfifo.full();
  }

  //bitmap mode
  if(address == 0x4100) {
    data.bit( 0) = vdp.mode.bit(0);
    data.bit( 1) = vdp.mode.bit(1);
    data.bit( 6) = vdp.lines;
    data.bit( 7) = vdp.priority;
    data.bit(15) = !Region::PAL();
  }

  //packed pixel control
  if(address == 0x4102) {  
    data.bit(0) = vdp.dotshift;
  }

  //autofill length
  if(address == 0x4104) {
    data.byte(0) = vdp.autofillLength;
  }

  //autofill address
  if(address == 0x4106) {
    data = vdp.autofillAddress;
  }

  //autofill data
  if(address == 0x4108) {
    data = vdp.autofillData;
  }

  //frame buffer control
  if(address == 0x410a) {
    if(shm.active()) shm.syncM68k();
    if(shs.active()) shs.syncM68k();
    data.bit( 0) = vdp.framebufferActive;
    data.bit( 1) = MegaDrive::vdp.refreshing()
                || vdp.framebufferEngaged(); // FEN: frame buffer engaged
    data.bit(13) = !vdp.paletteEngaged();    // PEN: can access palette
    data.bit(14) = vdp.hblank;
    data.bit(15) = vdp.vblank;
  }

  //palette
  if(address >= 0x4200 && address <= 0x43ff) {
    if(!vdp.framebufferAccess) return data;
    while(vdp.paletteEngaged()) {
      if(shm.active()) { shm.internalStep(1); shm.syncAll(true); }
      if(shs.active()) { shs.internalStep(1); shs.syncAll(true); }
    }
    if(shm.active()) shm.internalStep(4); if(shs.active()) shs.internalStep(4);
    data = vdp.cram[address >> 1 & 0xff];
  }

  return data;
}

auto M32X::writeInternalIO(n1 upper, n1 lower, n29 address, n16 data) -> void {
  if(shm.active()) shm.internalStep(1); if(shs.active()) shs.internalStep(1);

  //interrupt mask
  if(address == 0x4000) {
    if(lower && shm.active()) {
      shm.irq.pwm.enable  = data.bit(0);
      shm.irq.cmd.enable  = data.bit(1);
      shm.irq.hint.enable = data.bit(2);
      shm.irq.vint.enable = data.bit(3);
    }
    if(lower && shs.active()) {
      shs.irq.pwm.enable  = data.bit(0);
      shs.irq.cmd.enable  = data.bit(1);
      shs.irq.hint.enable = data.bit(2);
      shs.irq.vint.enable = data.bit(3);
    }
    if(lower) {
      io.hintVblank = data.bit(7);
    }
    if(upper) {
      vdp.framebufferAccess = data.bit(15);
    }
  }

  //stand by change
  if(address == 0x4002) {
  }

  //hcount
  if(address == 0x4004) {
    if(lower) {
      io.hperiod  = data.byte(0);
      io.hcounter = 0;
    }
  }

  //vres interrupt clear
  if(address == 0x4014) {
    if(shm.active()) shm.irq.vres.active = 0;
    if(shs.active()) shs.irq.vres.active = 0;
  }

  //vint interrupt clear
  if(address == 0x4016) {
    if(shm.active()) shm.irq.vint.active = 0;
    if(shs.active()) shs.irq.vint.active = 0;
  }

  //hint interrupt clear
  if(address == 0x4018) {
    if(shm.active()) shm.irq.hint.active = 0;
    if(shs.active()) shs.irq.hint.active = 0;
  }

  //cmd interrupt clear
  if(address == 0x401a) {
    if(shm.active()) shm.irq.cmd.active = 0;
    if(shs.active()) shs.irq.cmd.active = 0;
  }

  //pwm interrupt clear
  if(address == 0x401c) {
    if(shm.active()) shm.irq.pwm.active = 0;
    if(shs.active()) shs.irq.pwm.active = 0;
  }

  //communication
  if(address >= 0x4020 && address <= 0x402f) {
    if(shm.active()) { shm.syncAll(); }
    if(shs.active()) { shs.syncAll(); }
    if(upper) communication[address >> 1 & 7].byte(1) = data.byte(1);
    if(lower) communication[address >> 1 & 7].byte(0) = data.byte(0);
  }

  //PWM control
  if(address == 0x4030) {
    if(lower) {
      pwm.lmode   = data.bit(0,1);
      pwm.rmode   = data.bit(2,3);
      pwm.mono    = data.bit(4);
      pwm.dreqIRQ = data.bit(7);
      if(!pwm.dreqIRQ) {
        shm.dmac.dreq[1] = 0;
        shs.dmac.dreq[1] = 0;
      }
    }
    if(upper) {
      pwm.timer = data.bit(8,11);
      pwm.periods = 0;
    }
  }

  //PWM cycle
  if(address == 0x4032) {
    if(lower) pwm.cycle.bit(0, 7) = data.bit(0, 7);
    if(upper) pwm.cycle.bit(8,11) = data.bit(8,11);
  }

  //PWM left channel pulse width
  if(address == 0x4034) {
    if(upper) pwm.lfifoLatch.bit(8,11) = data.bit(8,11);
    if(lower) {
      pwm.lfifoLatch.byte(0) = data.byte(0);
      pwm.lfifo.write(pwm.lfifoLatch);
    }
  }

  //PWM right channel pulse width
  if(address == 0x4036) {
    if(upper) pwm.rfifoLatch.bit(8,11) = data.bit(8,11);
    if(lower) {
      pwm.rfifoLatch.byte(0) = data.byte(0);
      pwm.rfifo.write(pwm.rfifoLatch);
    }
  }

  //PWM mono pulse width
  if(address == 0x4038) {
    if(upper) pwm.mfifoLatch.bit(8,11) = data.bit(8,11);
    if(lower) {
      pwm.mfifoLatch.byte(0) = data.byte(0);
      pwm.lfifo.write(pwm.mfifoLatch);
      pwm.rfifo.write(pwm.mfifoLatch);
    }
  }

  //bitmap mode
  if(address == 0x4100) {
    if (!vdp.framebufferAccess) return;
    if(lower) {
      vdp.mode     = data.bit(0,1);
      vdp.lines    = data.bit(6);
      vdp.priority = data.bit(7);
    }
  }

  //packed pixel control
  if(address == 0x4102) {
    if (!vdp.framebufferAccess) return;
    if(lower) {
      vdp.dotshift = data.bit(0);
    }
  }

  //autofill length
  if(address == 0x4104) {
    if (!vdp.framebufferAccess) return;
    if(lower) {
      vdp.autofillLength = data.byte(0);
    }
  }

  //autofill address
  if(address == 0x4106) {
    if (!vdp.framebufferAccess) return;
    if(upper) vdp.autofillAddress.byte(1) = data.byte(1);
    if(lower) vdp.autofillAddress.byte(0) = data.byte(0);
  }

  //autofill data
  if(address == 0x4108) {
    if (!vdp.framebufferAccess) return;
    if(upper) vdp.autofillData.byte(1) = data.byte(1);
    if(lower) vdp.autofillData.byte(0) = data.byte(0);
    vdp.fill();
  }

  //frame buffer control
  if(address == 0x410a) {
    if (!vdp.framebufferAccess) return;
    if(lower) {
      vdp.selectFramebuffer(data.bit(0));
    }
  }

  //palette
  if(address >= 0x4200 && address <= 0x43ff) {
    if(!vdp.framebufferAccess) return;
    while(vdp.paletteEngaged()) {
      if(shm.active()) { shm.internalStep(1); shm.syncAll(true); }
      if(shs.active()) { shs.internalStep(1); shs.syncAll(true); }
    }
    if(shm.active()) shm.internalStep(4); if(shs.active()) shs.internalStep(4);
    if(upper) vdp.cram[address >> 1 & 0xff].byte(1) = data.byte(1);
    if(lower) vdp.cram[address >> 1 & 0xff].byte(0) = data.byte(0);
  }
}
