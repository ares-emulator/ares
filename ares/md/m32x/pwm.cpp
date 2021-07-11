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
  if(mono) {
    lsample = lfifo.read(0);
    rsample = rfifo.read(0);
  } else {
    if(lmode == 0) lsample = 0;
    if(lmode == 1) lsample = lfifo.read(0);
    if(lmode == 2) lsample = rfifo.read(0);
    if(lmode == 3) lsample = 0;  //undefined

    if(rmode == 0) rsample = 0;
    if(rmode == 1) rsample = rfifo.read(0);
    if(rmode == 2) rsample = lfifo.read(0);
    if(rmode == 3) rsample = 0;  //undefined
  }

  if(timer) {
    if(++periods == timer) {
      periods = 0;
      m32x.shm.irq.pwm.active = 1;
      m32x.shs.irq.pwm.active = 1;
    }
  }

  u32 clocks = max(1, cycle);
  counter += clocks;
  while(counter >= 522) {
    counter -= 522;
    stream->frame(lsample / 2047.0, rsample / 2047.0);
  }

  step(clocks);
}

auto M32X::PWM::step(u32 clocks) -> void {
  Thread::step(clocks);
  Thread::synchronize(cpu);
}

auto M32X::PWM::power(bool reset) -> void {
  Thread::create(23'020'200, {&M32X::PWM::main, this});
  lfifo.flush();
  rfifo.flush();
}
