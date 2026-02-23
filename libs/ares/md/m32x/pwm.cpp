auto M32X::PWM::load(Node::Object parent) -> void {
  stream = parent->append<Node::Audio::Stream>("PWM");
  stream->setChannels(2);
  updateFrequency();
}

auto M32X::PWM::unload(Node::Object parent) -> void {
  parent->remove(stream);
  stream.reset();
}

auto M32X::PWM::main() -> void {
    n12 clocks = cycle - 1;
    if(clocks == 0) clocks++;

    //check if cycle rate has changed and update sample rate accordingly
    static n12 previousCycle = cycle;
    if (cycle != previousCycle) {
      previousCycle = cycle;
      updateFrequency();
    }

    // skip action if cycle == 1 (illegal setting: "PWM will no longer operate")
    if (cycle != 1 && (lmode.bit(0) ^ lmode.bit(1) || rmode.bit(0) ^ rmode.bit(1))) {
      lsample = lfifo.read();
      rsample = rfifo.read();

      if (periods++ == n4(timer - 1)) {
        periods = 0;
        m32x.shm.irq.pwm.active = 1;
        m32x.shs.irq.pwm.active = 1;
        m32x.shm.dmac.dreq[1] = dreqIRQ;
        m32x.shs.dmac.dreq[1] = dreqIRQ;
      }
    }

    i12 outL, outR;

    if (lmode == 0) outL = 0;
    if (lmode == 1) outL = lsample;
    if (lmode == 2) outL = rsample;
    if (lmode == 3) outL = 0; // illegal mode

    if (rmode == 0) outR = 0;
    if (rmode == 1) outR = rsample;
    if (rmode == 2) outR = lsample;
    if (rmode == 3) outR = 0; // illegal mode

    auto left  = outL / (f32)clocks;
    auto right = outR / (f32)clocks;

    // filter DC offset to normalize output and limit popping
    left  = dcfilter_l.process(left);
    right = dcfilter_r.process(right);

    // handle clipping due to improper settings
    if(abs(left)  > 1.0) left  /= abs(left);
    if(abs(right) > 1.0) right /= abs(right);

    stream->frame(left, right);

    step(clocks);
}

auto M32X::PWM::step(u32 clocks) -> void {
  Thread::step(clocks);
  Thread::synchronize(cpu);
}

auto M32X::PWM::power(bool reset) -> void {
  Thread::create((system.frequency() / 7.0) * 3.0, std::bind_front(&M32X::PWM::main, this));
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
  lfifoLatch = 0;
  rfifoLatch = 0;
  mfifoLatch = 0;
  updateFrequency();
  dcfilter_l.reset();
  dcfilter_r.reset();
}

auto M32X::PWM::updateFrequency() -> void {
  n12 clocks = cycle - 1;
  if(clocks == 0) clocks++;
  stream->setFrequency((system.frequency() / 7.0) * 3.0 / clocks);
}
