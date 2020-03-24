auto NECDSP::read(uint24 address, uint8) -> uint8 {
  cpu.synchronize(*this);
  if(address.bit(0)) {
    return uPD96050::readSR();
  } else {
    return uPD96050::readDR();
  }
}

auto NECDSP::write(uint24 address, uint8 data) -> void {
  cpu.synchronize(*this);
  if(address.bit(0)) {
    return uPD96050::writeSR(data);
  } else {
    return uPD96050::writeDR(data);
  }
}

auto NECDSP::readRAM(uint24 address, uint8) -> uint8 {
  cpu.synchronize(*this);
  return uPD96050::readDP(address);
}

auto NECDSP::writeRAM(uint24 address, uint8 data) -> void {
  cpu.synchronize(*this);
  return uPD96050::writeDP(address, data);
}
