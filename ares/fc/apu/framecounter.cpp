auto APU::FrameCounter::main() -> void {
  --counter;
  odd = !odd;

  if (delay && --delayCounter == 0) {
    delay = false;
    step = 0;
    counter = getPeriod() + 2;
    if (mode == Freq48Hz) {
      apu.clockHalfFrame();
    }
  }

  if (counter != 0)
    return;

  switch(step) {
    case 0:
      step = 1;
      apu.clockQuarterFrame();
      break;
    case 1:
      step = 2;
      apu.clockHalfFrame();
      break;
    case 2:
      step = 3;
      apu.clockQuarterFrame();
      break;
    case 3:
      step = 4;
      if (mode == Freq60Hz && irqInhibit == 0)
        irqPending = 1;
      break;
    case 4:
      step = 5;
      apu.clockHalfFrame();
      if (mode == Freq60Hz && irqInhibit == 0) {
        irqPending = 1;
        apu.setIRQ();
      }
      break;
    case 5:
      step = 0;
      if (mode == Freq60Hz && irqInhibit == 0)
        irqPending = 1;
      break;
  }

  counter = getPeriod();
}

auto APU::FrameCounter::power(bool reset) -> void {
  irqInhibit = 0;
  if (!reset) mode = Freq60Hz;

  irqPending = 0;
  step = 0;
  counter = getPeriod();

  odd = true;
  delay = true;
  delayCounter = 1;
}

auto APU::FrameCounter::write(n8 data) -> void {
  mode = data.bit(7);
  delay = true;

  // If the write occurs during an APU cycle,
  // the effects occur 3 CPU cycles after the
  // $4017 write cycle, and if the write occurs
  // between APU cycles, the effects occurs 4 CPU
  // cycles after the write cycle.
  delayCounter = odd ? 1 : 2;

  irqInhibit = data.bit(6);
  if (irqInhibit) irqPending = 0;
}
