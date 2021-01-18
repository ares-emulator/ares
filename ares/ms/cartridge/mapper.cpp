auto Cartridge::read(n16 address) -> maybe<n8> {
  if(!node) return nothing;

  n2 page = address >> 14;
  address &= 0x3fff;

  switch(page) {

  case 0: {
    if(address <= 0x03ff) return rom.read(address);
    return rom.read(mapper.romPage0 << 14 | address);
  }

  case 1: {
    return rom.read(mapper.romPage1 << 14 | address);
  }

  case 2: {
    if(ram && mapper.ramEnablePage2) {
      return ram.read(mapper.ramPage2 << 14 | address);
    }

    return rom.read(mapper.romPage2 << 14 | address);
  }

  case 3: {
    if(ram && mapper.ramEnablePage3) {
      return ram.read(address);
    }

    return nothing;
  }

  }

  unreachable;
}

auto Cartridge::write(n16 address, n8 data) -> bool {
  if(!node) return false;

  if(address == 0xfffc) {
    mapper.shift = data.bit(0,1);
    mapper.ramPage2 = data.bit(2);
    mapper.ramEnablePage2 = data.bit(3);
    mapper.ramEnablePage3 = data.bit(4);
    mapper.romWriteEnable = data.bit(7);
  }

  if(address == 0xfffd) {
    mapper.romPage0 = data;
  }

  if(address == 0xfffe) {
    mapper.romPage1 = data;
  }

  if(address == 0xffff) {
    mapper.romPage2 = data;
  }

  n2 page = address >> 14;
  address &= 0x3fff;

  switch(page) {

  case 0: {
    return false;
  }

  case 1: {
    return false;
  }

  case 2: {
    if(ram && mapper.ramEnablePage2) {
      ram.write(mapper.ramPage2 << 14 | address, data);
      return true;
    }

    return false;
  }

  case 3: {
    if(ram && mapper.ramEnablePage3) {
      ram.write(address, data);
      return true;
    }

    return false;
  }

  }

  unreachable;
}
