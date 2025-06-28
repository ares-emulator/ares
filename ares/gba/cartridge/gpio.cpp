auto Cartridge::GPIO::readData() -> n4 {
  if(!readEnable) return 0;

  cpu.synchronize(cartridge.rtc);
  n4 data = 0;
  data |= cartridge.rtc.SCK() << 0;
  data |= cartridge.rtc.CS()  << 2;
  data |= cartridge.rtc.SIO() << 1;
  return data;
}

auto Cartridge::GPIO::readDirection() -> n4 {
  if(!readEnable) return 0;
  return (
      cartridge.rtc.csDirection  << 2
    | cartridge.rtc.sioDirection << 1
    | cartridge.rtc.sckDirection << 0
  );
}

auto Cartridge::GPIO::readControl() -> n1 {
  return readEnable;
}

auto Cartridge::GPIO::writeData(n4 data) -> void {
  cpu.synchronize(cartridge.rtc);
  cartridge.rtc.writeSCK(data >> 0);
  cartridge.rtc.writeCS( data >> 2);
  cartridge.rtc.writeSIO(data >> 1);
}

auto Cartridge::GPIO::writeDirection(n4 data) -> void {
  cartridge.rtc.csDirection  = data.bit(2);
  cartridge.rtc.sioDirection = data.bit(1);
  cartridge.rtc.sckDirection = data.bit(0);
}

auto Cartridge::GPIO::writeControl(n1 data) -> void {
  readEnable = data;
}
