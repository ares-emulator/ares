auto APU::DMC::start() -> void {
  if(lengthCounter == 0) {
    readAddress = 0x4000 + (addressLatch << 6);
    lengthCounter = (lengthLatch << 4) + 1;

    if (!dmaBufferValid)
      cpu.dmcDMAPending();
  }
}

auto APU::DMC::stop() -> void {
  lengthCounter = 0;
  dmaDelayCounter = 0;
}

auto APU::DMC::clock() -> n8 {
  n8 result = dacLatch;

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

        if (lengthCounter > 0)
          cpu.dmcDMAPending();
      } else {
        sampleValid = false;
      }
    }

    periodCounter = Region::PAL() ? dmcPeriodTablePAL[period] : dmcPeriodTableNTSC[period];
  }

  return result;
}

auto APU::DMC::power(bool reset) -> void {
  lengthCounter = 0;
  periodCounter = Region::PAL() ? dmcPeriodTablePAL[0] : dmcPeriodTableNTSC[0];
  dmaDelayCounter = 0;
  irqPending = 0;
  period = 0;
  irqEnable = 0;
  loopMode = 0;
  dacLatch = 0;
  addressLatch = 0;
  lengthLatch = 0;
  readAddress = 0;
  bitCounter = 0;
  dmaBufferValid = 0;
  dmaBuffer = 0;
  sampleValid = 0;
  sample = 0;
}

auto APU::DMC::setDMABuffer(n8 data) -> void {
  dmaBuffer = data;
  dmaBufferValid = true;
  lengthCounter--;
  readAddress++;

  if (lengthCounter == 0) {
    if (loopMode) {
      start();
    } else if (irqEnable) {
      irqPending = true;
      apu.setIRQ();
    }
  }
}
