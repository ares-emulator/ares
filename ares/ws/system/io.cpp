auto System::portRead(uint16 address) -> uint8 {
  uint8 data;

  //DISP_MODE
  if(address == 0x0060) {
    data.bit(0) = io.unknown0;
    data.bit(1) = io.unknown1;
    data.bit(3) = io.unknown3;
    data.bit(5) = io.mode.bit(0);
    data.bit(6) = io.mode.bit(1);
    data.bit(7) = io.mode.bit(2);
    return data;
  }

  //IEEP_DATALO
  if(address == 0x00ba) {
    data = eeprom.read(EEPROM::DataLo);
    return data;
  }

  //IEEP_DATAHI
  if(address == 0x00bb) {
    data = eeprom.read(EEPROM::DataHi);
    return data;
  }

  //IEEP_ADDRLO
  if(address == 0x00bc) {
    data = eeprom.read(EEPROM::AddressLo);
    return data;
  }

  //IEEP_ADDRHI
  if(address == 0x00bd) {
    data = eeprom.read(EEPROM::AddressHi);
    return data;
  }

  //IEEP_STATUS
  if(address == 0x00be) {
    data = eeprom.read(EEPROM::Status);
    return data;
  }

  return data;
}

auto System::portWrite(uint16 address, uint8 data) -> void {
  //DISP_MODE
  if(address == 0x0060) {
    io.unknown0 = data.bit(0);
    io.unknown1 = data.bit(1);
    io.unknown3 = data.bit(3);
    io.mode     = data.bit(5,7);
    return;
  }

  //IEEP_DATALO
  if(address == 0x00ba) {
    eeprom.write(EEPROM::DataLo, data);
    return;
  }

  //IEEP_DATAHI
  if(address == 0x00bb) {
    eeprom.write(EEPROM::DataHi, data);
    return;
  }

  //IEEP_ADDRLO
  if(address == 0x00bc) {
    eeprom.write(EEPROM::AddressLo, data);
    return;
  }

  //IEEP_ADDRHI
  if(address == 0x00bd) {
    eeprom.write(EEPROM::AddressHi, data);
    return;
  }

  //IEEP_CMD
  if(address == 0x00be) {
    eeprom.write(EEPROM::Command, data);
    return;
  }
}
