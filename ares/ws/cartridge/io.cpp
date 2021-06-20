auto Cartridge::readIO(n16 address) -> n8 {
  n8 data;

  switch(address) {

  case 0x00c0:  //BANK_ROM2
    data = io.romBank2;
    break;

  case 0x00c1:  //BANK_SRAM
    data = io.sramBank;
    break;

  case 0x00c2:  //BANK_ROM0
    data = io.romBank0;
    break;

  case 0x00c3:  //BANK_ROM1
    data = io.romBank1;
    break;

  case 0x00c4:  //EEP_DATALO
    data = eeprom.read(EEPROM::DataLo);
    break;

  case 0x00c5:  //EEP_DATAHI
    data = eeprom.read(EEPROM::DataHi);
    break;

  case 0x00c6:  //EEP_ADDRLO
    data = eeprom.read(EEPROM::AddressLo);
    break;

  case 0x00c7:  //EEP_ADDRHI
    data = eeprom.read(EEPROM::AddressHi);
    break;

  case 0x00c8:  //EEP_STATUS
    data = eeprom.read(EEPROM::Status);
    break;

  case 0x00ca:  //RTC_STATUS
    data = rtc.status();
    break;

  case 0x00cb:  //RTC_DATA
    data = rtc.read();
    break;

  case 0x00cc:  //GPO_EN
    data = io.gpoEnable;
    break;

  case 0x00cd:  //GPO_DATA
    data = io.gpoData;
    break;

  }

  return data;
}

auto Cartridge::writeIO(n16 address, n8 data) -> void {
  switch(address) {

  case 0x00c0:  //BANK_ROM2
    io.romBank2 = data;
    break;

  case 0x00c1:  //BANK_SRAM
    io.sramBank = data;
    break;

  case 0x00c2:  //BANK_ROM0
    io.romBank0 = data;
    break;

  case 0x00c3:  //BANK_ROM1
    io.romBank1 = data;
    break;

  case 0x00c4:  //EEP_DATALO
    eeprom.write(EEPROM::DataLo, data);
    break;

  case 0x00c5:  //EEP_DATAHI
    eeprom.write(EEPROM::DataHi, data);
    break;

  case 0x00c6:  //EEP_ADDRLO
    eeprom.write(EEPROM::AddressLo, data);
    break;

  case 0x00c7:  //EEP_ADDRHI
    eeprom.write(EEPROM::AddressHi, data);
    break;

  case 0x00c8:  //EEP_CMD
    eeprom.write(EEPROM::Command, data);
    break;

  case 0x00ca:  //RTC_CMD
    rtc.execute(data);
    break;

  case 0x00cb:  //RTC_DATA
    rtc.write(data);
    break;

  case 0x00cc:  //GPO_EN
    io.gpoEnable = data;
    break;

  case 0x00cd:  //GPO_DATA
    io.gpoData = data;
    break;

  }

  return;
}
