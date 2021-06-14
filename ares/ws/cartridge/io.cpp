auto Cartridge::portRead(n16 address) -> n8 {
  n8 data;

  switch(address) {

  case 0x00c0:  //BANK_ROM2
    data = r.romBank2;
    break;

  case 0x00c1:  //BANK_SRAM
    data = r.sramBank;
    break;

  case 0x00c2:  //BANK_ROM0
    data = r.romBank0;
    break;

  case 0x00c3:  //BANK_ROM1
    data = r.romBank1;
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
    data = rtcStatus();
    break;

  case 0x00cb:  //RTC_DATA
    data = rtcRead();
    break;

  case 0x00cc:  //GPO_EN
    data = r.gpoEnable;
    break;

  case 0x00cd:  //GPO_DATA
    data = r.gpoData;
    break;

  }

  return data;
}

auto Cartridge::portWrite(n16 address, n8 data) -> void {
  switch(address) {

  case 0x00c0:  //BANK_ROM2
    r.romBank2 = data;
    break;

  case 0x00c1:  //BANK_SRAM
    r.sramBank = data;
    break;

  case 0x00c2:  //BANK_ROM0
    r.romBank0 = data;
    break;

  case 0x00c3:  //BANK_ROM1
    r.romBank1 = data;
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
    rtcCommand(data);
    break;

  case 0x00cb:  //RTC_DATA
    rtcWrite(data);
    break;

  case 0x00cc:  //GPO_EN
    r.gpoEnable = data;
    break;

  case 0x00cd:  //GPO_DATA
    r.gpoData = data;
    break;

  }

  return;
}
