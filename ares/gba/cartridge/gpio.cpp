auto Cartridge::GPIO::readData() -> n4 {
  if(!readEnable) return 0;

  cpu.synchronize(cartridge.rtc);
  n4 data = latch & direction;
  if(!direction.bit(0)) data.bit(0) = cartridge.rtc.SCK();
  if(!direction.bit(2)) data.bit(2) = cartridge.rtc.CS();
  if(!direction.bit(1)) data.bit(1) = cartridge.rtc.SIO();
  return data;
}

auto Cartridge::GPIO::readDirection() -> n4 {
  if(!readEnable) return 0;
  return direction;
}

auto Cartridge::GPIO::readControl() -> n1 {
  return readEnable;
}

auto Cartridge::GPIO::writeData(n4 data) -> void {
  cpu.synchronize(cartridge.rtc);
  latch = data;
  cartridge.rtc.writeSCK(latch >> 0);
  cartridge.rtc.writeCS( latch >> 2);
  cartridge.rtc.writeSIO(latch >> 1);
}

auto Cartridge::GPIO::writeDirection(n4 data) -> void {
  direction = data;
  cartridge.rtc.csDirection  = data.bit(2);
  cartridge.rtc.sioDirection = data.bit(1);
  cartridge.rtc.sckDirection = data.bit(0);
}

auto Cartridge::GPIO::writeControl(n1 data) -> void {
  readEnable = data;
}
