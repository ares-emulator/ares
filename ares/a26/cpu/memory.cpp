inline auto CPU::readBus(n16 address) -> n8 {
  address &= 0x1fff;
  n8 data = cartridge.read(address);

  if(address.bit(12) == 0 && address.bit(7) == 0) {
    return tia.read(address & 0xf);
  }

  if(address.bit(12) == 0 && address.bit(9) == 0 && address.bit(7) == 1) {
    return riot.readRam(address & 0x7f);
  }

  if(address.bit(12) == 0 && address.bit(9) == 1 && address.bit(7) == 1) {
    return riot.readIo((address - 0x80) & 0x1f);
  }

  return data;
}

inline auto CPU::writeBus(n16 address, n8 data) -> void {
  address &= 0x1fff;
  cartridge.write(address, data);

  if(address.bit(12) == 0 && address.bit(7) == 0) {
    return tia.write(address & 0x3f, data);
  }

 if(address.bit(12) == 0 && address.bit(9) == 0 && address.bit(7) == 1) {
   return riot.writeRam(address & 0x7f, data);
 }

 if(address.bit(12) == 0 && address.bit(9) == 1 && address.bit(7) == 1) {
   return riot.writeIo((address - 0x80) & 0x1f, data);
 }

}

auto CPU::readDebugger(n16 address) -> n8 {
  return readBus(address);
}
