auto APU::FrameCounter::main() -> void {
  --counter;
  odd = !odd;

  if(!odd) {
    //IRQ is reset on next GET cycle
    if(delayIRQ) {
      delayIRQ = false;
      irqPending = 0;
      apu.setIRQ();
    }

    //counter is reloaded on GET cycle after next
    if(delayCounter[0]) {
      step = 0;
      counter = getPeriod();
      if(mode == Freq48Hz) apu.clockHalfFrame();
    }
    delayCounter[0] = delayCounter[1];
    delayCounter[1] = false;
  }

  if(counter != 0)  return;

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
      if(mode == Freq60Hz) irqPending = 1;
      break;
    case 4:
      step = 5;
      apu.clockHalfFrame();
      if(mode == Freq60Hz) {
        irqPending = 1;
        apu.setIRQ();
      }
      break;
    case 5:
      step = 0;
      if(mode == Freq60Hz) {
        irqPending = ~irqInhibit;
        apu.setIRQ();
      }
      break;
  }

  counter = getPeriod();
}

auto APU::FrameCounter::power(bool reset) -> void {
  irqInhibit = 0;
  if(!reset) mode = Freq60Hz;

  irqPending = 0;
  step = 0;
  counter = getPeriod();

  odd = true;
  delayIRQ = false;
  delayCounter[0] = false;
  delayCounter[1] = false;
}

auto APU::FrameCounter::write(n8 data) -> void {
  mode = data.bit(7);
  delayCounter[1] = true;

  irqInhibit = data.bit(6);
  if(irqInhibit) {
    irqPending = 0;
    apu.setIRQ();
  }
}
