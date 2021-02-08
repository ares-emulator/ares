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
    auto port1 = controllerPort1.read();
    auto port2 = controllerPort2.read();
    if(address.bit(0) == 0) {
      return port1.bit(0,5) << 0 | port2.bit(0,1) << 6;
    } else {
      return port2.bit(2,5) << 0 | 1 << 4 | 1 << 5 | port1.bit(6) << 6 | port2.bit(6) << 7;
    }
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

  }
}
