auto I8080::wait(u32 clocks) -> void {
  step(clocks);
}

auto I8080::opcode() -> n8 {
  step(4);
  return bus->read(PC++);
}

auto I8080::operand() -> n8 {
  step(3);
  return bus->read(PC++);
}

auto I8080::operands() -> n16 {
  n16    data = operand() << 0;
  return data | operand() << 8;
}

auto I8080::push(n16 x) -> void {
  write(--SP, x >> 8);
  write(--SP, x >> 0);
}

auto I8080::pop() -> n16 {
  n16    data = read(SP++) << 0;
  return data | read(SP++) << 8;
}

auto I8080::read(n16 address) -> n8 {
  step(3);
  return bus->read(address);
}

auto I8080::write(n16 address, n8 data) -> void {
  step(3);
  return bus->write(address, data);
}

auto I8080::in(n16 address) -> n8 {
  step(3);
  return bus->in(address);
}

auto I8080::out(n16 address, n8 data) -> void {
  step(3);
  return bus->out(address, data);
}
