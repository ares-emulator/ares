auto APU::DMC::start() -> void {
  if(lengthCounter == 0) {
    readAddress = 0x4000 + (addressLatch << 6);
    lengthCounter = (lengthLatch << 4) + 1;
  }
}

auto APU::DMC::stop() -> void {
  lengthCounter = 0;
  dmaDelayCounter = 0;
  cpu.rdyLine(1);
  cpu.rdyAddress(false);
}

auto APU::DMC::clock() -> n8 {
  n8 result = dacLatch;

  if(dmaDelayCounter > 0) {
    dmaDelayCounter--;

    if(dmaDelayCounter == 1) {
      cpu.rdyAddress(true, 0x8000 | readAddress);
    } else if(dmaDelayCounter == 0) {
      cpu.rdyLine(1);
      cpu.rdyAddress(false);

      dmaBuffer = cpu.MDR;
      dmaBufferValid = true;
      lengthCounter--;
      readAddress++;

      if(lengthCounter == 0) {
        if(loopMode) {
          start();
        } else if(irqEnable) {
          irqPending = true;
          apu.setIRQ();
        }
      }
    }
  }

  if(--periodCounter == 0) {
    if(sampleValid) {
      s32 delta = (((sample >> bitCounter) & 1) << 2) - 2;
      u32 data = dacLatch + delta;
      if((data & 0x80) == 0) dacLatch = data;
    }

    if(++bitCounter == 0) {
      if(dmaBufferValid) {
        sampleValid = true;
        sample = dmaBuffer;
        dmaBufferValid = false;
      } else {
        sampleValid = false;
      }
    }

    periodCounter = Region::PAL() ? dmcPeriodTablePAL[period] : dmcPeriodTableNTSC[period];
  }

  if(lengthCounter > 0 && !dmaBufferValid && dmaDelayCounter == 0) {
    cpu.rdyLine(0);
    dmaDelayCounter = 4;
  }

  return result;
}
