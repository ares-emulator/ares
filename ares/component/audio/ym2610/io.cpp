auto YM2610::read(n2 address) -> n8 {
  switch(address) {
  case 0: return fm.readStatus();
  case 1: return ssg.read();
  // TODO: 2/3
  }
  return 0;
}

auto YM2610::write(n2 address, n8 data) -> void {
  switch(address) {
  case 0:
      registerAddress = 0x000 | data;
      if(registerAddress <= 0xf) ssg.select(registerAddress);
      else fm.writeAddress(registerAddress);
      return;
  case 1: return writeLower(data);
  case 2: registerAddress = 0x100 | data; fm.writeAddress(registerAddress); return;
  case 3: return writeUpper(data);
  }
}

auto YM2610::writeLower(n8 data) -> void {
  switch(registerAddress) {
  case 0x000 ... 0x00f:
    ssg.write(data);
    return;
  case 0x010 ... 0x01b:
    //ADPCM-B
    return;
  case 0x01c:
    //EOS
    return;
  case 0x01d ... 0x1ff:
    fm.writeData(data);
    return;
  }
  unreachable;
}

auto YM2610::writeUpper(n8 data) -> void {
  switch(registerAddress) {
  case 0x000 ... 0x12f:
    //ADPCM-A
    return;
  case 0x130 ... 0x1ff:
    fm.writeAddress(registerAddress);
    fm.writeData(data);
    return;
  }
}
