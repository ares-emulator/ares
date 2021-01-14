auto Z80::yield() -> void {
  //freeze Z80, allow external access until relinquished
  if(bus->requested()) {
    bus->grant(true);
    while(bus->requested() && !synchronizing()) step(1);
    bus->grant(false);
  }
}

auto Z80::wait(uint clocks) -> void {
  yield();
  step(clocks);
}

auto Z80::opcode() -> uint8 {
  yield();
  step(4);
  return bus->read(r.pc++);
}

auto Z80::operand() -> uint8 {
  yield();
  step(3);
  return bus->read(r.pc++);
}

auto Z80::operands() -> uint16 {
  uint16 data = operand() << 0;
  return data | operand() << 8;
}

auto Z80::push(uint16 x) -> void {
  write(--SP, x >> 8);
  write(--SP, x >> 0);
}

auto Z80::pop() -> uint16 {
  uint16 data = read(SP++) << 0;
  return data | read(SP++) << 8;
}

auto Z80::displace(uint16& x) -> uint16 {
  if(&x != &r.ix.word && &x != &r.iy.word) return x;
  auto d = operand();
  wait(5);
  WZ = x + (int8)d;
  return WZ;
}

auto Z80::read(uint16 address) -> uint8 {
  yield();
  step(3);
  return bus->read(address);
}

auto Z80::write(uint16 address, uint8 data) -> void {
  yield();
  step(3);
  return bus->write(address, data);
}

auto Z80::in(uint16 address) -> uint8 {
  yield();
  step(4);
  return bus->in(address);
}

auto Z80::out(uint16 address, uint8 data) -> void {
  yield();
  step(4);
  return bus->out(address, data);
}
