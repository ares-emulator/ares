auto CPU::read(n16 address) -> n8 {
  if(auto result = platform->cheat(address)) return *result;

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
  n8 output = 0xff;

  switch(address.bit(6,7)) {

  case 2:
    return output = !address.bit(0) ? vdp.data() : vdp.status();
  case 3:
    if(Model::SG1000()) {
      if(address.bit(0) == 0) {
        output.bit(0, 5) = controllerPort1.read();
        output.bit(6, 7) = controllerPort2.read().bit(0, 1);
      } else {
        output.bit(0, 3) = controllerPort2.read().bit(2, 5);
      }

      return output;
    }

    return ppi.read(address.bit(0,1));
  }

  return output;
}

auto CPU::out(n16 address, n8 data) -> void {
  switch(address.bit(6,7)) {

  case 1: return psg.write(data);
  case 2: return !address.bit(0) ? vdp.data(data) : vdp.control(data);
  case 3: if(!Model::SG1000()) return ppi.write(address.bit(0,1), data); break;

  }
}
