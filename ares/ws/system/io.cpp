auto System::readIO(n16 address) -> n8 {
  n8 data;

  switch(address) {

  case 0x0060:  //DISP_MODE
    data.bit(0) = io.unknown0;
    data.bit(1) = io.unknown1;
    data.bit(3) = io.unknown3;
    data.bit(5) = io.mode.bit(0);
    data.bit(6) = io.mode.bit(1);
    data.bit(7) = io.mode.bit(2);
    break;

  case 0x00ba:  //IEEP_DATALO
    data = eeprom.read(EEPROM::DataLo);
    break;

  case 0x00bb:  //IEEP_DATAHI
    data = eeprom.read(EEPROM::DataHi);
    break;

  case 0x00bc:  //IEEP_ADDRLO
    data = eeprom.read(EEPROM::AddressLo);
    break;

  case 0x00bd:  //IEEP_ADDRHI
    data = eeprom.read(EEPROM::AddressHi);
    break;

  case 0x00be:  //IEEP_STATUS
    data = eeprom.read(EEPROM::Status);
    break;

  }

  return data;
}

auto System::writeIO(n16 address, n8 data) -> void {
  switch(address) {

  case 0x0060:  //DISP_MODE
    io.unknown0 = data.bit(0);
    io.unknown1 = data.bit(1);
    io.unknown3 = data.bit(3);
    io.mode     = data.bit(5,7);
    break;

  case 0x00ba:  //IEEP_DATALO
    eeprom.write(EEPROM::DataLo, data);
    break;

  case 0x00bb:  //IEEP_DATAHI
    eeprom.write(EEPROM::DataHi, data);
    break;

  case 0x00bc:  //IEEP_ADDRLO
    eeprom.write(EEPROM::AddressLo, data);
    break;

  case 0x00bd:  //IEEP_ADDRHI
    eeprom.write(EEPROM::AddressHi, data);
    break;

  case 0x00be:  //IEEP_CMD
    eeprom.write(EEPROM::Command, data);
    break;

  }

  return;
}
