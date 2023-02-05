auto Cartridge::readIO(n16 address) -> n8 {
  n8 data;

  switch(address) {

  case 0x00c0:  //BANK_ROM2
  case 0x00cf:
    data.bit(0,7) = io.romBank2.bit(0,7);
    break;

  case 0x00c1:  //BANK_SRAM
  case 0x00d0:
    data.bit(0,7) = io.sramBank.bit(0,7);
    break;

  case 0x00d1:  //BANK_SRAM_HI
    data.bit(0,7) = io.sramBank.bit(8,15);
    break;

  case 0x00c2:  //BANK_ROM0
  case 0x00d2:
    data.bit(0,7) = io.romBank0.bit(0,7);
    break;

  case 0x00d3:  //BANK_ROM0_HI
    data.bit(0,7) = io.romBank0.bit(8,15);
    break;

  case 0x00c3:  //BANK_ROM1
  case 0x00d4:
    data.bit(0,7) = io.romBank1.bit(0,7);
    break;

  case 0x00d5:  //BANK_ROM1_HI
    data.bit(0,7) = io.romBank1.bit(8,15);
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

  case 0x00ce:  //MEMORY_CTRL
    data.bit(0) = io.flashEnable;
    break;
    
  }

  return data;
}

auto Cartridge::writeIO(n16 address, n8 data) -> void {
  switch(address) {

  case 0x00c0:  //BANK_ROM2
  case 0x00cf:
    io.romBank2.bit(0,7) = data.bit(0,7);
    break;

  case 0x00c1:  //BANK_SRAM
  case 0x00d0:
    io.sramBank.bit(0,7) = data.bit(0,7);
    break;

  case 0x00d1:  //BANK_SRAM_HI
    io.sramBank.bit(8,15) = data.bit(0,7);
    break;

  case 0x00c2:  //BANK_ROM0
  case 0x00d2:
    io.romBank0.bit(0,7) = data.bit(0,7);
    break;

  case 0x00d3:  //BANK_ROM0_HI
    io.romBank0.bit(8,15) = data.bit(0,7);
    break;

  case 0x00c3:  //BANK_ROM1
  case 0x00d4:
    io.romBank1.bit(0,7) = data.bit(0,7);
    break;

  case 0x00d5:  //BANK_ROM1_HI
    io.romBank1.bit(8,15) = data.bit(0,7);
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
    if (rtc.ram) rtc.execute(data);
    break;

  case 0x00cb:  //RTC_DATA
    if (rtc.ram) rtc.write(data);
    break;

  case 0x00cc:  //GPO_EN
    io.gpoEnable = data;
    break;

  case 0x00cd:  //GPO_DATA
    io.gpoData = data;
    break;

  case 0x00ce:  //MEMORY_CTRL
    io.flashEnable = data.bit(0);
    break;

  }

  return;
}
