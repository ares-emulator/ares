auto CPU::read(n16 address) -> n8 {
  step(1);

  if(io.rdyLine == 0) {
    while(io.rdyLine == 0) {
      step(1);
    }

    io.openBus = readBus(address);
    return io.openBus;
  }

  io.openBus = readBus(address);
  return io.openBus;
}

auto CPU::write(n16 address, n8 data) -> void {
  step(1);
  writeBus(address, io.openBus = data);
}

auto CPU::rdyLine(bool line) -> void {
  io.rdyLine = line;
}
