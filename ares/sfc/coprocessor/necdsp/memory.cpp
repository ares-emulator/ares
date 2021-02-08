auto NECDSP::read(n24 address, n8) -> n8 {
  cpu.synchronize(*this);
  if(address.bit(0)) {
    return uPD96050::readSR();
  } else {
    return uPD96050::readDR();
  }
}

auto NECDSP::write(n24 address, n8 data) -> void {
  cpu.synchronize(*this);
  if(address.bit(0)) {
    return uPD96050::writeSR(data);
  } else {
    return uPD96050::writeDR(data);
  }
}

auto NECDSP::readRAM(n24 address, n8) -> n8 {
  cpu.synchronize(*this);
  return uPD96050::readDP(address);
}

auto NECDSP::writeRAM(n24 address, n8 data) -> void {
  cpu.synchronize(*this);
  return uPD96050::writeDP(address, data);
}
