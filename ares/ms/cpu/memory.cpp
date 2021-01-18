auto CPU::read(n16 address) -> n8 {
  n8 data = 0xff;
  if(auto result = cartridge.read(address)) {
    data = result();
  } else if(address >= 0xc000) {
    data = ram.read(address);
  }
  return mdr = data;
}

auto CPU::write(n16 address, n8 data) -> void {
  mdr = data;
  if(cartridge.write(address, data)) {
  } else if(address >= 0xc000) {
    ram.write(address, data);
  }
}

auto CPU::in(n16 address) -> n8 {
  if((address & 0xc1) == 0x01 && Model::MasterSystem() && !Region::NTSCJ()) {
    n8 data;
    data.bit(0) = controllerPort1.io.trInputEnable;
    data.bit(1) = controllerPort1.io.thInputEnable;
    data.bit(2) = controllerPort2.io.trInputEnable;
    data.bit(3) = controllerPort2.io.thInputEnable;
    data.bit(4) = controllerPort1.io.trOutputLevel;
    data.bit(5) = controllerPort1.io.thOutputLevel;
    data.bit(6) = controllerPort2.io.trOutputLevel;
    data.bit(7) = controllerPort2.io.thOutputLevel;
    return data;
  }

  if((address & 0xc0) == 0x00 && Model::MasterSystem()) {
    if(Model::MarkIII()) return mdr;
    if(Model::MasterSystemI()) return mdr;
    if(Model::MasterSystemII()) return 0xff;
  }

  if((address & 0xc0) == 0x00 && Model::GameGear()) {
    platform->input(system.controls.start);
    bool start = !system.controls.start->value();
    return 0x7f | start << 7;
  }

  if((address & 0xc1) == 0x40) {
    return vdp.vcounter();
  }

  if((address & 0xc1) == 0x41) {
    return vdp.hcounter();
  }

  if((address & 0xc1) == 0x80) {
    return vdp.data();
  }

  if((address & 0xc1) == 0x81) {
    return vdp.status();
  }

  if((address & 0xff) == 0xf2 && opll.node) {
    n8 data;
    if(psg.io.mute == 0 && opll.io.mute == 0) data.bit(0,1) = 3;
    if(psg.io.mute == 0 && opll.io.mute == 1) data.bit(0,1) = 0;
    if(psg.io.mute == 1 && opll.io.mute == 0) data.bit(0,1) = 1;
    if(psg.io.mute == 1 && opll.io.mute == 1) data.bit(0,1) = 2;
    data.bit(2) = 0;
    data.bit(3) = 0;
    data.bit(4) = 0;
    data.bit(5) = vdp.ccounter().bit( 3);
    data.bit(6) = vdp.ccounter().bit( 7);
    data.bit(7) = vdp.ccounter().bit(11);
    return data;
  }

  if((address & 0xc1) == 0xc0 && Model::MasterSystem()) {
    auto port1 = controllerPort1.read();
    auto port2 = controllerPort2.read();
    n8 data;
    data.bit(0,5) = port1.bit(0,5);
    data.bit(6,7) = port2.bit(0,1);
    if(!Region::NTSCJ()) {
      if(!controllerPort1.io.trInputEnable) data.bit(5) = controllerPort1.io.trOutputLevel;
    }
    return data;
  }

  if((address & 0xc1) == 0xc1 && Model::MasterSystem()) {
    platform->input(system.controls.reset);
    bool reset = !system.controls.reset->value();
    auto port1 = controllerPort1.read();
    auto port2 = controllerPort2.read();
    n8 data;
    data.bit(0,3) = port2.bit(2,5);
    data.bit(4)   = reset;
    data.bit(5)   = 1;
    data.bit(6)   = port1.bit(6);
    data.bit(7)   = port2.bit(6);
    if(!Region::NTSCJ()) {
      if(!controllerPort2.io.trInputEnable) data.bit(3) = controllerPort2.io.trOutputLevel;
      if(!controllerPort1.io.thInputEnable) data.bit(6) = controllerPort1.io.thOutputLevel;
      if(!controllerPort2.io.thInputEnable) data.bit(7) = controllerPort2.io.thOutputLevel;
    }
    return data;
  }

  if((address & 0xc1) == 0xc0 && Model::GameGear()) {
    system.controls.poll();
    n8 data;
    data.bit(0) = !system.controls.upLatch;
    data.bit(1) = !system.controls.downLatch;
    data.bit(2) = !system.controls.leftLatch;
    data.bit(3) = !system.controls.rightLatch;
    data.bit(4) = !system.controls.one->value();
    data.bit(5) = !system.controls.two->value();
    data.bit(6) = 1;
    data.bit(7) = 1;
    return data;
  }

  return 0xff;
}

auto CPU::out(n16 address, n8 data) -> void {
  if((address & 0xc1) == 0x01 && Model::MasterSystem()) {
    controllerPort1.io.trInputEnable = data.bit(0);
    controllerPort1.io.thInputEnable = data.bit(1);
    controllerPort2.io.trInputEnable = data.bit(2);
    controllerPort2.io.thInputEnable = data.bit(3);
    if(Region::NTSCJ()) {
      //NTSC-J region sets thOutputLevel to thInputEnable
      controllerPort1.io.thOutputLevel = data.bit(1);
      controllerPort2.io.thOutputLevel = data.bit(3);
    } else {
      //NTSC-U and PAL regions allow values to be set directly
      controllerPort1.io.trOutputLevel = data.bit(4);
      controllerPort1.io.thOutputLevel = data.bit(5);
      controllerPort2.io.trOutputLevel = data.bit(6);
      controllerPort2.io.thOutputLevel = data.bit(7);
    }
    if(controllerPort1.io.thPreviousLevel == 0 && controllerPort1.io.thOutputLevel == 1) {
      vdp.hcounterLatch();
    }
    if(controllerPort2.io.thPreviousLevel == 0 && controllerPort2.io.thOutputLevel == 1) {
      vdp.hcounterLatch();
    }
    controllerPort1.io.thPreviousLevel = controllerPort1.io.thOutputLevel;
    controllerPort2.io.thPreviousLevel = controllerPort2.io.thOutputLevel;
    return;
  }

  if((address & 0xff) == 0x06 && Model::GameGear()) {
    return psg.balance(data);
  }

  if((address & 0xc0) == 0x40) {
    return psg.write(data);
  }

  if((address & 0xc1) == 0x80) {
    return vdp.data(data);
  }

  if((address & 0xc1) == 0x81) {
    return vdp.control(data);
  }

  if((address & 0xff) == 0xf0 && opll.node) {
    return opll.address(data);
  }

  if((address & 0xff) == 0xf1 && opll.node) {
    return opll.write(data);
  }

  if((address & 0xff) == 0xf2 && opll.node) {
    if(Model::MarkIII()) data.bit(1) = 0;
    if(data.bit(0,1) == 0) psg.io.mute = 0, opll.io.mute = 1;
    if(data.bit(0,1) == 1) psg.io.mute = 1, opll.io.mute = 0;
    if(data.bit(0,1) == 2) psg.io.mute = 1, opll.io.mute = 1;
    if(data.bit(0,1) == 3) psg.io.mute = 0, opll.io.mute = 0;
    return;
  }
}
