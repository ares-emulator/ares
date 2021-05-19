auto YM2610::read(n2 address) -> n8 {
  switch(address) {
  case 0: return fm.readStatus();
  }
  return 0;
}

auto YM2610::write(n2 address, n8 data) -> void {
  switch(address) {
  case 0: register = 0x000 | data; return;
  case 1: return writeLower(data);
  case 2: register = 0x100 | data; return;
  case 3: return writeUpper(data);
  }
}

auto YM2610::writeLower(n8 data) -> void {
  switch(register) {
  case 0x000 ... 0x00f:
    ssg.select(register);
    ssg.write(data);
    return;
  case 0x010 ... 0x01b:
    //ADPCM-B
    return;
  case 0x01c:
    //EOS
    return;
  case 0x01d ... 0x1ff:
    fm.writeAddress(register);
    fm.writeData(data);
    return;
  }
  unreachable;
}

auto YM2610::writeUpper(n8 data) -> void {
  switch(register) {
  case 0x000 ... 0x12f:
    //ADPCM-A
    return;
  case 0x130 ... 0x1ff:
    fm.writeAddress(register);
    fm.writeData(data);
    return;
  }
}
