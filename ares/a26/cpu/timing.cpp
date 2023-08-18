auto CPU::read(n16 address) -> n8 {
  step(1);

  if(io.rdyLine == 0) {
    while(io.rdyLine == 0) {
      step(1);
    }

    MDR = readBus(address);
    return MDR;
  }

  MDR = readBus(address);
  return MDR;
}

auto CPU::write(n16 address, n8 data) -> void {
  step(1);
  writeBus(address, MDR = data);
}

auto CPU::lastCycle() -> void {
}

auto CPU::nmi(n16& vector) -> void {

}

auto CPU::rdyLine(bool line) -> void {
  io.rdyLine = line;
}
