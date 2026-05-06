//DMA clock divider
inline auto CPU::dmaCounter() const -> u32 {
  return counter.cpu & 7;
}

//joypad auto-poll clock divider
inline auto CPU::joypadCounter() const -> u32 {
  return counter.cpu & 127;
}

auto CPU::step(u32 clocks) -> void {
  status.irqLock = 0;
  u32 ticks = clocks >> 1;
  while(ticks--) {
    counter.cpu += 2;
    tick();
    if(hcounter() & 2) nmiPoll(), irqPoll();
    if(joypadCounter() == 0) joypadEdge();
  }

  if(!status.dramRefresh && hcounter() >= status.dramRefreshPosition) {
    //note: pattern should technically be 5-3, 5-3, 5-3, 5-3, 5-3 per logic analyzer
    //result averages out the same as no coprocessor polls refresh() at > frequency()/2
    status.dramRefresh = 1; step(6); status.dramRefresh = 2; step(2); aluEdge();
    status.dramRefresh = 1; step(6); status.dramRefresh = 2; step(2); aluEdge();
    status.dramRefresh = 1; step(6); status.dramRefresh = 2; step(2); aluEdge();
    status.dramRefresh = 1; step(6); status.dramRefresh = 2; step(2); aluEdge();
    status.dramRefresh = 1; step(6); status.dramRefresh = 2; step(2); aluEdge();
  }

  if(!status.hdmaSetupTriggered && hcounter() >= status.hdmaSetupPosition) {
    status.hdmaSetupTriggered = 1;
    hdmaReset();
    if(hdmaEnable()) {
      status.hdmaPending = 1;
      status.hdmaMode = 0;
    }
  }

  if(!status.hdmaTriggered && hcounter() >= status.hdmaPosition) {
    status.hdmaTriggered = 1;
    if(hdmaActive()) {
      status.hdmaPending = 1;
      status.hdmaMode = 1;
    }
  }

  Thread::step(clocks);
  for(auto peripheral : peripherals) Thread::synchronize(*peripheral);

#if !defined(PROFILE_PERFORMANCE)
  for(auto coprocessor : coprocessors) Thread::synchronize(*coprocessor);
#endif
}

//called by ppu.tick() when Hcounter=0
auto CPU::scanline() -> void {
  //forcefully sync S-CPU to other processors, in case chips are not communicating
  Thread::synchronize(smp, ppu.thread());
  for(auto coprocessor : coprocessors) Thread::synchronize(*coprocessor);

  if(vcounter() == 0) {
    //HDMA setup triggers once every frame
    status.hdmaSetupPosition = (io.version == 1 ? 12 + 8 - dmaCounter() : 12 + dmaCounter());
    status.hdmaSetupTriggered = 0;

    status.autoJoypadCounter = 33;  //33 = inactive
  }

  //DRAM refresh occurs once every scanline
  if(io.version == 2) status.dramRefreshPosition = 530 + 8 - dmaCounter();
  status.dramRefresh = 0;

  //HDMA triggers once every visible scanline
  if(vcounter() < ppu.vdisp()) {
    status.hdmaPosition = 1104;
    status.hdmaTriggered = 0;
  }
}

alwaysinline auto CPU::aluEdge() -> void {
  if(alu.mpyctr) {
    alu.mpyctr--;
    if(io.rddiv & 1) io.rdmpy += alu.shift;
    io.rddiv >>= 1;
    alu.shift <<= 1;
  }

  if(alu.divctr) {
    alu.divctr--;
    io.rddiv <<= 1;
    alu.shift >>= 1;
    if(io.rdmpy >= alu.shift) {
      io.rdmpy -= alu.shift;
      io.rddiv |= 1;
    }
  }
}

alwaysinline auto CPU::dmaEdge() -> void {
  //H/DMA pending && DMA inactive?
  //.. Run one full CPU cycle
  //.. HDMA pending && HDMA enabled ? DMA sync + HDMA run
  //.. DMA pending && DMA enabled ? DMA sync + DMA run
  //.... HDMA during DMA && HDMA enabled ? DMA sync + HDMA run
  //.. Run one bus CPU cycle
  //.. CPU sync

  if(status.dmaActive) {
    if(status.hdmaPending) {
      status.hdmaPending = 0;
      if(hdmaEnable()) {
        if(!dmaEnable()) {
          step(counter.dma = 8 - dmaCounter());
        }
        status.hdmaMode == 0 ? hdmaSetup() : hdmaRun();
        if(!dmaEnable()) {
          step(status.clockCount - counter.dma % status.clockCount);
          status.dmaActive = 0;
        }
      }
    }

    if(status.dmaPending) {
      status.dmaPending = 0;
      if(dmaEnable()) {
        step(counter.dma = 8 - dmaCounter());
        dmaRun();
        step(status.clockCount - counter.dma % status.clockCount);
        status.dmaActive = 0;
      }
    }
  }

  if(!status.dmaActive) {
    if(status.dmaPending || status.hdmaPending) {
      status.dmaActive = 1;
    }
  }
}

//called every 128 clocks; see CPU::step()
alwaysinline auto CPU::joypadEdge() -> void {
  if(vcounter() == ppu.vdisp() && (counter.cpu & 255) == 0 && hcounter() >= 130 && hcounter() <= 384) {
    //begin new polling sequence
    status.autoJoypadCounter = 0;
  } else {
    //stop after polling has been completed for this frame
    if(status.autoJoypadCounter >= 33) return;

    status.autoJoypadCounter++;
  }

  if(status.autoJoypadCounter == 0) {
    //latch controller states on the first polling cycle
    //TODO: this may need to happen one cycle earlier
    status.autoJoypadLatch = io.autoJoypadPoll;
    controllerPort1.latch(status.autoJoypadLatch | status.cpuLatch);
    controllerPort2.latch(status.autoJoypadLatch | status.cpuLatch);
    if(io.autoJoypadPoll) {
      //shift registers are cleared to zero at start of auto-joypad polling
      io.joy1 = 0;
      io.joy2 = 0;
      io.joy3 = 0;
      io.joy4 = 0;
    }
  }

  if(status.autoJoypadCounter == 1) {
    //release latch and begin reading on the second cycle
    status.autoJoypadLatch = 0;
    controllerPort1.latch(status.autoJoypadLatch | status.cpuLatch);
    controllerPort2.latch(status.autoJoypadLatch | status.cpuLatch);
  }

  if(status.autoJoypadCounter != 1 && !io.autoJoypadPoll) {
    // if auto-joypad polling is disabled at this point skip the rest of the polling
    status.autoJoypadCounter = 33;
    return;
  }

  if(status.autoJoypadCounter >= 2) {
    //sixteen bits are shifted into joy{1-4}, one bit per 256 clocks
    //the bits are read on one 128-clock cycle and written on the next
    if ((status.autoJoypadCounter & 1) == 0) {
      status.autoJoypadPort1 = controllerPort1.data();
      status.autoJoypadPort2 = controllerPort2.data();
    } else {
      io.joy1 = io.joy1 << 1 | status.autoJoypadPort1.bit(0);
      io.joy2 = io.joy2 << 1 | status.autoJoypadPort2.bit(0);
      io.joy3 = io.joy3 << 1 | status.autoJoypadPort1.bit(1);
      io.joy4 = io.joy4 << 1 | status.autoJoypadPort2.bit(1);
    }
  }
}
