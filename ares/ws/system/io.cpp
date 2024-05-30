auto System::readIO(n16 address) -> n8 {
  n8 data;

  switch(address) {

  case 0x0060:  //DISP_MODE
    data.bit(0)    = cpu.io.cartridgeClock;
    data.bit(1)    = cpu.io.cartridgeSramWait;
    data.bit(3)    = cpu.io.cartridgeIoWait;
    data.bit(5, 7) = io.mode.bit(0, 2);
    break;

  case 0x00ba:  //IEEP_DATALO
    data = eeprom.read(EEPROM::DataLo);
    break;

  case 0x00bb:  //IEEP_DATAHI
    data = eeprom.read(EEPROM::DataHi);
    break;

  case 0x00bc:  //IEEP_CMDLO
    data = eeprom.read(EEPROM::CommandLo);
    break;

  case 0x00bd:  //IEEP_CMDHI
    data = eeprom.read(EEPROM::CommandHi);
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
    cpu.io.cartridgeClock = data.bit(0);
    cpu.io.cartridgeSramWait = data.bit(1);
    cpu.io.cartridgeIoWait = data.bit(3);
    io.mode     = data.bit(5,7);
    break;

  case 0x00ba:  //IEEP_DATALO
    eeprom.write(EEPROM::DataLo, data);
    break;

  case 0x00bb:  //IEEP_DATAHI
    eeprom.write(EEPROM::DataHi, data);
    break;

  case 0x00bc:  //IEEP_CMDLO
    eeprom.write(EEPROM::CommandLo, data);
    break;

  case 0x00bd:  //IEEP_CMDHI
    eeprom.write(EEPROM::CommandHi, data);
    break;

  case 0x00be:  //IEEP_CTRL
    eeprom.write(EEPROM::Control, data);
    break;

  }

  return;
}
