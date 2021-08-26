auto CPU::stop() -> void {
  idle();
  if(Model::SuperGameBoy()) {
    scheduler.exit(Event::Step);
  }
}

auto CPU::halt() -> void {
  idle();
  if(status.interruptLatch) r.halt = 0;
  if(Model::SuperGameBoy()) {
    scheduler.exit(Event::Step);
  }
}

auto CPU::haltBugTrigger() -> void {
  // halt bug is triggered when IME is off, and IE & IF != 0
  // does not properly emulate two halts in a rom
  if(!r.ime && status.interruptLatch != 0) { 
    r.haltBug = 1;
  }
}

auto CPU::idle() -> void {
  if(r.ei) r.ei = 0, r.ime = 1;
  step();
  step();
  status.interruptLatch = status.interruptFlag & status.interruptEnable;
  step();
  step();
}

auto CPU::read(n16 address) -> n8 {
  n8 data = 0xff;
  if(r.ei) r.ei = 0, r.ime = 1;
  data &= bus.read(0, address, data);
  step();
  data &= bus.read(1, address, data);
  step();
  data &= bus.read(2, address, data);
  status.interruptLatch = status.interruptFlag & status.interruptEnable;
  step();
  data &= bus.read(3, address, data);
  step();
  data &= bus.read(4, address, data);
  return data;
}

auto CPU::write(n16 address, n8 data) -> void {
  if(r.ei) r.ei = 0, r.ime = 1;
  bus.write(0, address, data);
  step();
  bus.write(1, address, data);
  step();
  bus.write(2, address, data);
  step();
  bus.write(3, address, data);
  step();
  bus.write(4, address, data);
  status.interruptLatch = status.interruptFlag & status.interruptEnable;
}

//VRAM DMA source can only be ROM or RAM
auto CPU::readDMA(n16 address, n8 data) -> n8 {
  if(address < 0x8000) return bus.read(address, data);  //0000-7fff
  if(address < 0xa000) return data;                     //8000-9fff
  if(address < 0xe000) return bus.read(address, data);  //a000-dfff
  return data;                                          //e000-ffff
}

//VRAM DMA target is always VRAM
auto CPU::writeDMA(n13 address, n8 data) -> void {
  return bus.write(0x8000 | address, data);
}

auto CPU::readDebugger(n16 address) -> n8 {
  return bus.read(address, 0xff);
}
