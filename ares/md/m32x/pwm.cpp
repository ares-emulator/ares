auto M32X::PWM::load(Node::Object parent) -> void {
  stream = parent->append<Node::Audio::Stream>("PWM");
  stream->setChannels(2);
  stream->setFrequency(44'100);
}

auto M32X::PWM::unload(Node::Object parent) -> void {
  parent->remove(stream);
  stream.reset();
}

auto M32X::PWM::main() -> void {
  n12 clocks = cycle-1;
  if(clocks && (lmode.bit(0)^lmode.bit(1) || rmode.bit(0)^rmode.bit(1))) {
    if(lmode == 1) lsample = lfifo.read();
    if(lmode == 2) lsample = rfifo.read();

    if(rmode == 1) rsample = rfifo.read();
    if(rmode == 2) rsample = lfifo.read();

    lfifoLatch.bit(14) = lfifo.empty();
    rfifoLatch.bit(14) = rfifo.empty();
    mfifoLatch.bit(14) = lfifoLatch.bit(14) & rfifoLatch.bit(14);

    if(periods++ == n4(timer-1)) {
      periods = 0;
      m32x.shm.irq.pwm.active = 1;
      m32x.shs.irq.pwm.active = 1;
      m32x.shm.dmac.dreq[1] = dreqIRQ;
      m32x.shs.dmac.dreq[1] = dreqIRQ;
    }
  }

  counter += max(1, clocks);
  while(counter >= 522) {
    counter -= 522;
    auto left = cycle > 0 ? lsample / (f32)cycle : 0;
    auto right = cycle > 0 ? rsample / (f32)cycle : 0;
    stream->frame(left, right);
  }

  step(clocks);
}

auto M32X::PWM::step(u32 clocks) -> void {
  Thread::step(clocks);
  Thread::synchronize(cpu);
}

auto M32X::PWM::power(bool reset) -> void {
  Thread::create((system.frequency() / 7.0) * 3.0, {&M32X::PWM::main, this});
  lmode = 0;
  rmode = 0;
  mono = 0;
  dreqIRQ = 0;
  timer = 0;
  cycle = 0;
  periods = 0;
  counter = 0;
  lsample = 0;
  rsample = 0;
  lfifo.flush();
  rfifo.flush();
  lfifoLatch = 0x4000; // empty
  rfifoLatch = 0x4000; // empty
  mfifoLatch = 0x4000; // empty
}
