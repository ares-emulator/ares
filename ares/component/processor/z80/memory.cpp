auto Z80::wait(u32 clocks) -> void {
  step(clocks);
}

auto Z80::opcode() -> n8 {
  step(4);
  return bus->read(PC++);
}

auto Z80::operand() -> n8 {
  step(3);
  return bus->read(PC++);
}

auto Z80::operands() -> n16 {
  n16    data = operand() << 0;
  return data | operand() << 8;
}

auto Z80::push(n16 x) -> void {
  write(--SP, x >> 8);
  write(--SP, x >> 0);
}

auto Z80::pop() -> n16 {
  n16    data = read(SP++) << 0;
  return data | read(SP++) << 8;
}

auto Z80::displace(n16& x) -> n16 {
  if(&x != &ix.word && &x != &iy.word) return x;
  auto d = operand();
  wait(5);
  WZ = x + (i8)d;
  return WZ;
}

auto Z80::read(n16 address) -> n8 {
  step(3);
  return bus->read(address);
}

auto Z80::write(n16 address, n8 data) -> void {
  step(3);
  return bus->write(address, data);
}

auto Z80::in(n16 address) -> n8 {
  step(4);
  return bus->in(address);
}

auto Z80::out(n16 address, n8 data) -> void {
  step(4);
  return bus->out(address, data);
}
