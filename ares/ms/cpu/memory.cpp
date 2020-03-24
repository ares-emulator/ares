auto CPU::read(uint16 address) -> uint8 {
  uint8 data = 0xff;
  if(auto result = cartridge.read(address)) {
    data = result();
  } else if(address >= 0xc000) {
    data = ram.read(address);
  }
  return data;
}

auto CPU::write(uint16 address, uint8 data) -> void {
  if(cartridge.write(address, data)) {
  } else if(address >= 0xc000) {
    ram.write(address, data);
  }
}

auto CPU::in(uint16 address) -> uint8 {
  switch(address.bit(6,7)) {

  case 0: {
    if(Model::GameGear()) {
      platform->input(system.controls.start);
      bool start = !system.controls.start->value();
      return 0x7f | start << 7;
    }

    return 0xff;  //SMS1 = MDR, SMS2 = 0xff
  }

  case 1: {
    return !address.bit(0) ? vdp.vcounter() : vdp.hcounter();
  }

  case 2: {
    return !address.bit(0) ? vdp.data() : vdp.status();
  }

  case 3: {
    if(address == 0xf2) return state.enableFM;

    if(Model::MasterSystem()) {
      platform->input(system.controls.reset);
      bool reset = !system.controls.reset->value();
      auto port1 = controllerPort1.read();
      auto port2 = controllerPort2.read();
      if(address.bit(0) == 0) {
        return port1.bit(0,5) << 0 | port2.bit(0,1) << 6;
      } else {
        return port2.bit(2,5) << 0 | reset << 4 | 1 << 5 | port1.bit(6) << 6 | port2.bit(6) << 7;
      }
    }

    if(Model::GameGear()) {
      if(address.bit(0) == 0) {
        system.controls.poll();
        bool up    = !system.controls.upLatch;
        bool down  = !system.controls.downLatch;
        bool left  = !system.controls.leftLatch;
        bool right = !system.controls.rightLatch;
        bool one   = !system.controls.one->value();
        bool two   = !system.controls.two->value();
        return up << 0 | down << 1 | left << 2 | right << 3 | one << 4 | two << 5 | 1 << 6 | 1 << 7;
      } else {
        return 0xff;
      }
    }

    return 0xff;
  }

  }

  return 0xff;
}

auto CPU::out(uint16 address, uint8 data) -> void {
  if((uint8)address == 0x06) {
    if(Model::GameGear()) return psg.balance(data);
  }

  switch(address.bit(6,7)) {

  case 1: {
    return psg.write(data);
  }

  case 2: {
    return !address.bit(0) ? vdp.data(data) : vdp.control(data);
  }

  case 3: {
    if(address == 0xf0 && state.enableFM) opll.address(data);
    if(address == 0xf1 && state.enableFM) opll.write(data);
    if(address == 0xf2) state.enableFM = data.bit(0);
  }

  }
}
