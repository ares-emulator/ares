auto CPU::Interrupts::poll() -> void {
  priority = 0;
  cpu.inttc3.poll(vector, priority);
  cpu.inttc2.poll(vector, priority);
  cpu.inttc1.poll(vector, priority);
  cpu.inttc0.poll(vector, priority);
  cpu.intad .poll(vector, priority);
  cpu.inttx1.poll(vector, priority);
  cpu.intrx1.poll(vector, priority);
  cpu.inttx0.poll(vector, priority);
  cpu.intrx0.poll(vector, priority);
  cpu.inttr7.poll(vector, priority);
  cpu.inttr6.poll(vector, priority);
  cpu.inttr5.poll(vector, priority);
  cpu.inttr4.poll(vector, priority);
  cpu.intt3 .poll(vector, priority);
  cpu.intt2 .poll(vector, priority);
  cpu.intt1 .poll(vector, priority);
  cpu.intt0 .poll(vector, priority);
  cpu.int7  .poll(vector, priority);
  cpu.int6  .poll(vector, priority);
  cpu.int5  .poll(vector, priority);
  cpu.int4  .poll(vector, priority);
  cpu.int0  .poll(vector, priority);
  cpu.intwd .poll(vector, priority);
  cpu.nmi   .poll(vector, priority);
}

auto CPU::Interrupts::fire() -> bool {
  if(!priority || priority < cpu.r.iff) return false;

  priority = 0;
  cpu.inttc3.fire(vector);
  cpu.inttc2.fire(vector);
  cpu.inttc1.fire(vector);
  cpu.inttc0.fire(vector);
  cpu.intad .fire(vector);
  cpu.inttx1.fire(vector);
  cpu.intrx1.fire(vector);
  cpu.inttx0.fire(vector);
  cpu.intrx0.fire(vector);
  cpu.inttr7.fire(vector);
  cpu.inttr6.fire(vector);
  cpu.inttr5.fire(vector);
  cpu.inttr4.fire(vector);
  cpu.intt3 .fire(vector);
  cpu.intt2 .fire(vector);
  cpu.intt1 .fire(vector);
  cpu.intt0 .fire(vector);
  cpu.int7  .fire(vector);
  cpu.int6  .fire(vector);
  cpu.int5  .fire(vector);
  cpu.int4  .fire(vector);
  cpu.int0  .fire(vector);
  cpu.intwd .fire(vector);
  cpu.nmi   .fire(vector);

  if(vector == cpu.dma0.vector) {
    if(cpu.dma(0)) { cpu.dma0.vector = 0; cpu.inttc0.pending = 1; }
  } else if(vector == cpu.dma1.vector) {
    if(cpu.dma(1)) { cpu.dma1.vector = 0; cpu.inttc1.pending = 1; }
  } else if(vector == cpu.dma2.vector) {
    if(cpu.dma(2)) { cpu.dma2.vector = 0; cpu.inttc2.pending = 1; }
  } else if(vector == cpu.dma3.vector) {
    if(cpu.dma(3)) { cpu.dma3.vector = 0; cpu.inttc3.pending = 1; }
  } else {
    cpu.r.iff = priority;
    if(cpu.r.iff != 7) cpu.r.iff++;
    cpu.interrupt(vector);
  }

  poll();
  return true;
}

auto CPU::Interrupt::operator=(bool value) -> void {
  set(value);
}

auto CPU::Interrupt::poll(uint8& vector, uint3& priority) -> void {
  if(!enable || !pending) return;
  if(!maskable) { priority = 7; vector = this->vector; return; }
  if(dmaAllowed && cpu.r.iff <= 6) {
    if(this->vector == cpu.dma0.vector) { priority = 6; vector = this->vector; return; }
    if(this->vector == cpu.dma1.vector) { priority = 6; vector = this->vector; return; }
    if(this->vector == cpu.dma2.vector) { priority = 6; vector = this->vector; return; }
    if(this->vector == cpu.dma3.vector) { priority = 6; vector = this->vector; return; }
  }
  if(this->priority == 0 || this->priority == 7) return;
  if(this->priority >= priority) { priority = this->priority; vector = this->vector; return; }
}

auto CPU::Interrupt::fire(uint8 vector) -> void {
  if(this->vector != vector) return;
  if(line == 0 && !level.low ) pending = 0;
  if(line == 1 && !level.high) pending = 0;
}

auto CPU::Interrupt::set(bool line) -> void {
  line ? raise() : lower();
}

auto CPU::Interrupt::raise() -> void {
  if(!enable || line == 1) return;
  line = 1;
  if(pending || !(level.high || edge.rising)) return;
  pending = 1;
  cpu.interrupts.poll();
}

auto CPU::Interrupt::lower() -> void {
  if(!enable || line == 0) return;
  line = 0;
  if(pending || !(level.low || edge.falling)) return;
  pending = 1;
  cpu.interrupts.poll();
}

auto CPU::Interrupt::trigger() -> void {
  if(pending) return;
  pending = 1;
  cpu.interrupts.poll();
}

auto CPU::Interrupt::clear() -> void {
  if(!pending) return;
  pending = 0;
  cpu.interrupts.poll();
}

auto CPU::Interrupt::setEnable(uint1 enable) -> void {
  if(this->enable == enable) return;
  this->enable = enable;
  cpu.interrupts.poll();
}

auto CPU::Interrupt::setPriority(uint3 priority) -> void {
  if(this->priority == priority) return;
  this->priority = priority;
  cpu.interrupts.poll();
}

auto CPU::Interrupt::power(uint8 vector) -> void {
  this->vector = vector;

  //set up defaults that apply to most vectors
  //CPU::power() will perform specialization as needed later
  dmaAllowed = 1;
  enable = 1;
  maskable = 1;
  priority = 0;
  line = 0;
  pending = 0;
  level.high = 0;
  level.low = 0;
  edge.rising = 1;
  edge.falling = 0;
}
