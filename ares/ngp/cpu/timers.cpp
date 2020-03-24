auto CPU::Prescaler::step(uint clocks) -> void {
  if(!enable) return;
  while(clocks--) {
    auto latch = counter++;

    // T1
    if(!latch.bit( 5) || counter.bit( 5)) continue;
    if(cpu.t0.mode == 1) cpu.t01.clockT0();
    if(cpu.t1.mode == 1) cpu.t01.clockT1();
    if(cpu.t2.mode == 1) cpu.t23.clockT2();
    if(cpu.t3.mode == 1) cpu.t23.clockT3();
    if(cpu.t4.mode == 1) cpu.t4.clock();
    if(cpu.t5.mode == 1) cpu.t5.clock();

    // T4
    if(!latch.bit( 7) || counter.bit( 7)) continue;
    if(cpu.t0.mode == 2) cpu.t01.clockT0();
    if(cpu.t2.mode == 2) cpu.t23.clockT2();
    if(cpu.t4.mode == 2) cpu.t4.clock();
    if(cpu.t5.mode == 2) cpu.t5.clock();

    // T16
    if(!latch.bit( 9) || counter.bit( 9)) continue;
    if(cpu.t0.mode == 3) cpu.t01.clockT0();
    if(cpu.t1.mode == 2) cpu.t01.clockT1();
    if(cpu.t2.mode == 3) cpu.t23.clockT2();
    if(cpu.t3.mode == 2) cpu.t23.clockT3();
    if(cpu.t4.mode == 3) cpu.t4.clock();
    if(cpu.t5.mode == 3) cpu.t5.clock();

    // T256
    if(!latch.bit(13) || counter.bit(13)) continue;
    if(cpu.t1.mode == 3) cpu.t01.clockT1();
    if(cpu.t3.mode == 3) cpu.t23.clockT3();
  }
}

//

auto CPU::TI0::operator=(bool value) -> void {
  if(latch == value) return;
  latch = value;
  if(cpu.t0.mode == 0 && latch == 1) cpu.t01.clockT0();
}

auto CPU::TI4::operator=(bool value) -> void {
  if(latch == value) return;
  latch = value;
  if(cpu.t4.captureMode == 1 && latch == 1) cpu.t4.captureTo1();
  if(cpu.t4.captureMode == 2 && latch == 1) cpu.t4.captureTo1();
  if(cpu.t4.captureMode == 2 && latch == 0) cpu.t4.captureTo2();
  if(cpu.t4.mode == 0 && latch == 1) cpu.t4.clock();
}

auto CPU::TI5::operator=(bool value) -> void {
  if(latch == value) return;
  latch = value;
  if(cpu.t4.captureMode == 1 && latch == 1) cpu.t4.captureTo1();
}

auto CPU::TI6::operator=(bool value) -> void {
  if(latch == value) return;
  latch = value;
  if(cpu.t5.captureMode == 1 && latch == 1) cpu.t5.captureTo3();
  if(cpu.t5.captureMode == 2 && latch == 1) cpu.t5.captureTo3();
  if(cpu.t5.captureMode == 2 && latch == 0) cpu.t5.captureTo4();
  if(cpu.t5.mode == 0 && latch == 1) cpu.t5.clock();
}

auto CPU::TI7::operator=(bool value) -> void {
  if(latch == value) return;
  latch = value;
  if(cpu.t5.captureMode == 1 && latch == 1) cpu.t5.captureTo4();
}

//

auto CPU::TO1::operator=(bool value) -> void {
  if(latch == value) return;
  latch = value;
}

auto CPU::TO3::operator=(bool value) -> void {
  if(latch == value) return;
  latch = value;
  if(latch) apu.irq.line = 1;
}

auto CPU::TO4::operator=(bool value) -> void {
  if(latch == value) return;
  latch = value;
}

auto CPU::TO5::operator=(bool value) -> void {
  if(latch == value) return;
  latch = value;
}

auto CPU::TO6::operator=(bool value) -> void {
  if(latch == value) return;
  latch = value;
}

auto CPU::TO7::operator=(bool value) -> void {
  if(latch == value) return;
  latch = value;
}

//

auto CPU::Timer0::disable() -> void {
  counter = 0;
  cpu.intt0 = 0;
}

auto CPU::Timer1::disable() -> void {
  counter = 0;
  cpu.intt1 = 0;
}

auto CPU::FlipFlop1::operator=(bool value) -> void {
  if(cpu.t4.captureMode == 3 && output == 0 && value == 1) cpu.t4.captureTo1();
  if(cpu.t4.captureMode == 3 && output == 1 && value == 0) cpu.t4.captureTo2();
  output = value;
  cpu.to1 = output;
}

auto CPU::Timer01::clockT0() -> void {
  if(!cpu.t0.enable) return;
  if(mode == 0) {
    ++cpu.t0.counter;
    bool matchT0 = cpu.t0.counter == cpu.t0.compare;
    if(matchT0) {
      cpu.intt0.trigger();
      cpu.t0.counter = 0;
      if(cpu.ff1.source == 0 && cpu.ff1.invert) cpu.ff1 = !cpu.ff1;
      if(cpu.t1.mode == 0) clockT1();
    }
  }
  if(mode == 1) {
    if(!++cpu.t0.counter) ++cpu.t1.counter;
    bool matchT0 = cpu.t0.counter == cpu.t0.compare;
    bool matchT1 = cpu.t1.counter == cpu.t1.compare;
    if(matchT0 && matchT1) {
      cpu.intt1.trigger();
      cpu.t0.counter = 0;
      cpu.t1.counter = 0;
      if(cpu.ff1.invert) cpu.ff1 = !cpu.ff1;
    }
  }
  if(mode == 2) {
    ++cpu.t0.counter;
    bool matchT0 = cpu.t0.counter == cpu.t0.compare;
    bool matchT1 = cpu.t0.counter == cpu.t1.compare;
    if(matchT0 || matchT1) {
      if(cpu.ff1.invert) cpu.ff1 = !cpu.ff1;
    }
    if(matchT0) {
      cpu.intt0.trigger();
    }
    if(matchT1) {
      cpu.intt1.trigger();
      cpu.t0.counter = 0;
      if(buffer.enable) cpu.t0.compare = buffer.compare;
    }
  }
  if(mode == 3) {
    ++cpu.t0.counter;
    if(pwm == 1) cpu.t0.counter = (uint6)cpu.t0.counter;
    if(pwm == 2) cpu.t0.counter = (uint7)cpu.t0.counter;
    if(pwm == 3) cpu.t0.counter = (uint8)cpu.t0.counter;
    bool matchT0  = cpu.t0.counter == cpu.t0.compare;
    bool matchPWM = cpu.t0.counter == 0;
    if(matchT0 || matchPWM) {
      cpu.intt0.trigger();
      cpu.t0.counter = 0;
      if(buffer.enable) cpu.t0.compare = buffer.compare;
      if(cpu.ff1.invert) cpu.ff1 = !cpu.ff1;
    }
  }
}

auto CPU::Timer01::clockT1() -> void {
  if(!cpu.t1.enable) return;
  if(mode == 0 || mode == 3) {
    ++cpu.t1.counter;
    bool matchT1 = cpu.t1.counter == cpu.t1.compare;
    if(matchT1) {
      cpu.intt1.trigger();
      cpu.t1.counter = 0;
      if(cpu.ff1.source == 1 && cpu.ff1.invert) cpu.ff1 = !cpu.ff1;
    }
  }
}

//

auto CPU::Timer2::disable() -> void {
  counter = 0;
  cpu.intt2 = 0;
}

auto CPU::Timer3::disable() -> void {
  counter = 0;
  cpu.intt3 = 0;
}

auto CPU::FlipFlop3::operator=(bool value) -> void {
  output = value;
  cpu.to3 = output;
}

auto CPU::Timer23::clockT2() -> void {
  if(!cpu.t2.enable) return;
  if(mode == 0) {
    ++cpu.t2.counter;
    bool matchT2 = cpu.t2.counter == cpu.t2.compare;
    if(matchT2) {
      cpu.intt2.trigger();
      cpu.t2.counter = 0;
      if(cpu.ff3.source == 0 && cpu.ff3.invert) cpu.ff3 = !cpu.ff3;
      if(cpu.t3.mode == 0) clockT3();
    }
  }
  if(mode == 1) {
    if(!++cpu.t2.counter) ++cpu.t3.counter;
    bool matchT2 = cpu.t2.counter == cpu.t2.compare;
    bool matchT3 = cpu.t3.counter == cpu.t3.compare;
    if(matchT2 && matchT3) {
      cpu.intt3.trigger();
      cpu.t2.counter = 0;
      cpu.t3.counter = 0;
      if(cpu.ff3.invert) cpu.ff3 = !cpu.ff3;
    }
  }
  if(mode == 2) {
    ++cpu.t2.counter;
    bool matchT2 = cpu.t2.counter == cpu.t2.compare;
    bool matchT3 = cpu.t2.counter == cpu.t3.compare;
    if(matchT2 || matchT3) {
      if(cpu.ff3.invert) cpu.ff3 = !cpu.ff3;
    }
    if(matchT2) {
      cpu.intt2.trigger();
    }
    if(matchT3) {
      cpu.intt3.trigger();
      cpu.t3.counter = 0;
      if(buffer.enable) cpu.t2.compare = buffer.compare;
    }
  }
  if(mode == 3) {
    ++cpu.t2.counter;
    if(pwm == 1) cpu.t2.counter = (uint6)cpu.t2.counter;
    if(pwm == 2) cpu.t2.counter = (uint7)cpu.t2.counter;
    if(pwm == 3) cpu.t2.counter = (uint8)cpu.t2.counter;
    bool matchT2  = cpu.t2.counter == cpu.t2.compare;
    bool matchPWM = cpu.t2.counter == 0;
    if(matchT2 || matchPWM) {
      cpu.intt2.trigger();
      cpu.t2.counter = 0;
      if(buffer.enable) cpu.t2.compare = buffer.compare;
      if(cpu.ff3.invert) cpu.ff3 = !cpu.ff3;
    }
  }
}

auto CPU::Timer23::clockT3() -> void {
  if(!cpu.t3.enable) return;
  if(mode == 0 || mode == 3) {
    ++cpu.t3.counter;
    bool matchT3 = cpu.t3.counter == cpu.t3.compare;
    if(matchT3) {
      cpu.intt3.trigger();
      cpu.t3.counter = 0;
      if(cpu.ff3.source == 1 && cpu.ff3.invert) cpu.ff3 = !cpu.ff3;
    }
  }
}

//

auto CPU::Timer4::disable() -> void {
  counter = 0;
  cpu.inttr4 = 0;
  cpu.inttr5 = 0;
}

auto CPU::Timer4::captureTo1() -> void {
  capture1 = counter;
  if(cpu.ff4.flipOnCapture1) cpu.ff4 = !cpu.ff4;
}

auto CPU::Timer4::captureTo2() -> void {
  capture2 = counter;
  if(cpu.ff4.flipOnCapture2) cpu.ff4 = !cpu.ff4;
  if(cpu.ff5.flipOnCapture2) cpu.ff5 = !cpu.ff5;
}

auto CPU::Timer4::clock() -> void {
  if(!enable) return;
  ++counter;
  if(counter == compare4) {
    cpu.inttr4.trigger();
    if(cpu.ff4.flipOnCompare4) cpu.ff4 = !cpu.ff4;
  }
  if(counter == compare5) {
    cpu.inttr5.trigger();
    if(cpu.ff4.flipOnCompare5) cpu.ff4 = !cpu.ff4;
    if(cpu.ff5.flipOnCompare5) cpu.ff5 = !cpu.ff5;
    if(buffer.enable) compare4 = buffer.compare;
    if(clearOnCompare5) counter = 0;
  }
}

auto CPU::FlipFlop4::operator=(bool value) -> void {
  output = value;
  cpu.to4 = output;
}

auto CPU::FlipFlop5::operator=(bool value) -> void {
  output = value;
  cpu.to5 = value;
}

//

auto CPU::Timer5::disable() -> void {
  counter = 0;
  cpu.inttr6 = 0;
  cpu.inttr7 = 0;
}

auto CPU::Timer5::captureTo3() -> void {
  capture3 = counter;
  if(cpu.ff6.flipOnCapture3) cpu.ff6 = !cpu.ff6;
}

auto CPU::Timer5::captureTo4() -> void {
  capture4 = counter;
  if(cpu.ff6.flipOnCapture4) cpu.ff6 = !cpu.ff6;
}

auto CPU::Timer5::clock() -> void {
  if(!enable) return;
  ++counter;
  if(counter == compare6) {
    cpu.inttr6.trigger();
    if(cpu.ff6.flipOnCompare6) cpu.ff6 = !cpu.ff6;
  }
  if(counter == compare7) {
    cpu.inttr7.trigger();
    if(cpu.ff6.flipOnCompare7) cpu.ff6 = !cpu.ff6;
    if(buffer.enable) compare6 = buffer.compare;
    if(clearOnCompare7) counter = 0;
  }
}

auto CPU::FlipFlop6::operator=(bool value) -> void {
  output = value;
  cpu.to6 = output;
}
