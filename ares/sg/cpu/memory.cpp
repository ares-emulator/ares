auto CPU::read(n16 address) -> n8 {
  n8 data = 0xff;
  if(auto result = cartridge.read(address)) {
    data = result();
  } else if(address >= 0xc000) {
    data = ram.read(address);
  }
  return data;
}

auto CPU::write(n16 address, n8 data) -> void {
  if(cartridge.write(address, data)) {
  } else if(address >= 0xc000) {
    ram.write(address, data);
  }
}

auto CPU::in(n16 address) -> n8 {
  switch(address.bit(6,7)) {

  case 0: {
    return 0xff;
  }

  case 1: {
    return 0xff;
  }

  case 2: {
    return !address.bit(0) ? vdp.data() : vdp.status();
  }

  case 3: {
    return ppi.read(address.bit(0,1));
  }

  }

  return 0xff;
}

auto CPU::out(n16 address, n8 data) -> void {
  switch(address.bit(6,7)) {

  case 1: {
    return psg.write(data);
  }

  case 2: {
    return !address.bit(0) ? vdp.data(data) : vdp.control(data);
  }

  case 3: {
    return ppi.write(address.bit(0,1), data);
  }

  }
}
