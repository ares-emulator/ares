auto CPU::mdr() const -> n8 {
  return (bus.mdr | bus.pullup) & ~bus.pulldown;
}

auto CPU::read(n16 address) -> n8 {
  n8 data = mdr();
  if(address >= 0xc000 && bus.ramEnable) data = ram.read(address);
  if(bus.biosEnable) data = bios.read(address, data);
  if(bus.cartridgeEnable) data = cartridge.read(address, data);
  return bus.mdr = data;
}

auto CPU::write(n16 address, n8 data) -> void {
  bus.mdr = data;
  if(address >= 0xc000 && bus.ramEnable) ram.write(address, data);
  if(bus.biosEnable) bios.write(address, data);
  if(bus.cartridgeEnable) cartridge.write(address, data);
}

//note: the Japanese Mark III / Master System supposedly decodes a0-a7 fully for I/O.
//since I don't have explicit confirmation of this, I haven't implemented this yet.

auto CPU::in(n16 address) -> n8 {
  n8 data = mdr();
  if(0);

 else if((address & 0xff) == 0x00 && Display::LCD()) {
    platform->input(system.controls.start);
    data.bit(0) = 0;
    data.bit(1) = 0;
    data.bit(2) = 0;
    data.bit(3) = 0;
    data.bit(4) = 0;
    data.bit(5) = Region::PAL();
    data.bit(6) = !Region::NTSCJ();
    data.bit(7) = !system.controls.start->value();
  }

  else if((address & 0xff) == 0x01 && Display::LCD()) {
    data.bit(0) = sio.dataDirection.bit(0) ? 1 : sio.parallelData.bit(0);
    data.bit(1) = sio.dataDirection.bit(1) ? 1 : sio.parallelData.bit(1);
    data.bit(2) = sio.dataDirection.bit(2) ? 1 : sio.parallelData.bit(2);
    data.bit(3) = sio.dataDirection.bit(3) ? 1 : sio.parallelData.bit(3);
    data.bit(4) = sio.dataDirection.bit(4) ? 1 : sio.parallelData.bit(4);
    data.bit(5) = sio.dataDirection.bit(5) ? 1 : sio.parallelData.bit(5);
    data.bit(6) = sio.dataDirection.bit(6) ? 1 : sio.parallelData.bit(6);
    data.bit(7) =                                sio.parallelData.bit(7);
  }

  else if((address & 0xff) == 0x02 && Display::LCD()) {
    data.bit(0,6) = sio.dataDirection;
    data.bit(7)   = sio.nmiEnable;
  }

  else if((address & 0xff) == 0x03 && Display::LCD()) {
    data = sio.transmitData;
  }

  else if((address & 0xff) == 0x04 && Display::LCD()) {
    data = sio.receiveData;
  }

  else if((address & 0xff) == 0x05 && Display::LCD()) {
    data.bit(0)   = sio.transmitFull;
    data.bit(1)   = sio.receiveFull;
    data.bit(2)   = sio.framingError;
    data.bit(3,5) = sio.unknown;
    data.bit(6,7) = sio.baudRate;
  }

  else if((address & 0xff) == 0x06 && Display::LCD()) {
    data = mdr();
  }

  else if((address & 0xc1) == 0x00) {
    data = mdr();
  }

  else if((address & 0xc1) == 0x01) {
    if(bus.ioEnable && Display::CRT() && !Region::NTSCJ()) {
      data.bit(0) = controllerPort1.trDirection;
      data.bit(1) = controllerPort1.thDirection;
      data.bit(2) = controllerPort2.trDirection;
      data.bit(3) = controllerPort2.thDirection;
      data.bit(4) = controllerPort1.trLevel;
      data.bit(5) = controllerPort1.thLevel;
      data.bit(6) = controllerPort2.trLevel;
      data.bit(7) = controllerPort2.thLevel;
    } else {
      data = mdr();
    }
  }

  else if((address & 0xc1) == 0x40) {
    data = vdp.vcounterQuery();
  }

  else if((address & 0xc1) == 0x41) {
    data = vdp.hcounterQuery();
  }

  else if((address & 0xc1) == 0x80) {
    data = vdp.data();
  }

  else if((address & 0xc1) == 0x81) {
    data = vdp.status();
  }

  else if((address & 0xff) == 0xf2 && opll.node) {
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
  }

  else if((address & 0xc1) == 0xc0 && Display::CRT()) {
    auto port1 = controllerPort1.read();
    auto port2 = controllerPort2.read();
    data.bit(0,5) = port1.bit(0,5);
    data.bit(6,7) = port2.bit(0,1);
    if(controllerPort1.trOutput()) data.bit(5) = controllerPort1.trLevel;
  }

  else if((address & 0xc1) == 0xc1 && Display::CRT()) {
    auto port1 = controllerPort1.read();
    auto port2 = controllerPort2.read();
    data.bit(0,3) = port2.bit(2,5);
    data.bit(4)   = Region::NTSCJ() ? 1 : !system.controls.reset->value();
    data.bit(5)   = 1;
    data.bit(6)   = port1.bit(6);
    data.bit(7)   = port2.bit(6);
    if(controllerPort2.trOutput()) data.bit(3) = controllerPort2.trLevel;
    if(controllerPort1.thOutput()) data.bit(6) = controllerPort1.thLevel;
    if(controllerPort2.thOutput()) data.bit(7) = controllerPort2.thLevel;
  }

  else if(((address & 0xff) == 0xc0 || (address & 0xff) == 0xdc) && Display::LCD()) {
    system.controls.poll();
    data.bit(0) = !system.controls.upLatch;
    data.bit(1) = !system.controls.downLatch;
    data.bit(2) = !system.controls.leftLatch;
    data.bit(3) = !system.controls.rightLatch;
    data.bit(4) = !system.controls.one->value();
    data.bit(5) = !system.controls.two->value();
    data.bit(6) = sio.dataDirection.bit(0) ? 1 : sio.parallelData.bit(0);
    data.bit(7) = sio.dataDirection.bit(1) ? 1 : sio.parallelData.bit(1);
  }

  else if(((address & 0xff) == 0xc1 || (address & 0xff) == 0xdd) && Display::LCD()) {
    data.bit(0) = sio.dataDirection.bit(2) ? 1 : sio.parallelData.bit(2);
    data.bit(1) = sio.dataDirection.bit(3) ? 1 : sio.parallelData.bit(3);
    data.bit(2) = sio.dataDirection.bit(4) ? 1 : sio.parallelData.bit(4);
    data.bit(3) = sio.dataDirection.bit(5) ? 1 : sio.parallelData.bit(5);
    data.bit(4) = 1;
    data.bit(5) = 1;
    data.bit(6) = 1;
    data.bit(7) = sio.dataDirection.bit(6) ? 1 : sio.parallelData.bit(6);
  }

  debugger.in(address, data);
  return data;
}

auto CPU::out(n16 address, n8 data) -> void {
  if(0);

  else if((address & 0xff) == 0x00 && Display::LCD()) {
    //input port 2 (read-only)
  }

  else if((address & 0xff) == 0x01 && Display::LCD()) {
    sio.parallelData = data;
  }

  else if((address & 0xff) == 0x02 && Display::LCD()) {
    sio.dataDirection = data.bit(0,6);
    sio.nmiEnable     = data.bit(7);
  }

  else if((address & 0xff) == 0x03 && Display::LCD()) {
    sio.transmitData = data;
  }

  else if((address & 0xff) == 0x04 && Display::LCD()) {
    //receive data buffer (read-only)
  }

  else if((address & 0xff) == 0x05 && Display::LCD()) {
    sio.transmitFull = 0;
    sio.receiveFull  = 0;
    sio.framingError = 0;
    sio.unknown      = data.bit(3,5);
    sio.baudRate     = data.bit(6,7);
  }

  else if((address & 0xff) == 0x06 && Display::LCD()) {
    psg.balance(data);
  }

  else if((address & 0xc1) == 0x00) {
    bus.ioEnable        = !data.bit(2);
    bus.biosEnable      = !data.bit(3);
    bus.ramEnable       = !data.bit(4);
    bus.cardEnable      = !data.bit(5);
    bus.cartridgeEnable = !data.bit(6);
    bus.expansionEnable = !data.bit(7);
  }

  else if((address & 0xc1) == 0x01 && Display::CRT()) {
    auto thLevel1 = controllerPort1.thLevel;
    auto thLevel2 = controllerPort2.thLevel;

    controllerPort1.trDirection = data.bit(0);
    controllerPort1.thDirection = data.bit(1);
    controllerPort2.trDirection = data.bit(2);
    controllerPort2.thDirection = data.bit(3);
    if(Region::NTSCJ()) {
      //NTSC-J region sets thLevel to thDirection
      controllerPort1.thLevel = controllerPort1.thDirection;
      controllerPort2.thLevel = controllerPort2.thDirection;
    } else {
      //NTSC-U and PAL regions allow values to be set directly
      controllerPort1.trLevel = data.bit(4);
      controllerPort1.thLevel = data.bit(5);
      controllerPort2.trLevel = data.bit(6);
      controllerPort2.thLevel = data.bit(7);
    }

    if(!thLevel1 && controllerPort1.thLevel) vdp.hcounterLatch();
    if(!thLevel2 && controllerPort2.thLevel) vdp.hcounterLatch();
  }

  else if((address & 0xc0) == 0x40) {
    psg.write(data);
  }

  else if((address & 0xc1) == 0x80) {
    vdp.data(data);
  }

  else if((address & 0xc1) == 0x81) {
    vdp.control(data);
  }

  else if((address & 0xff) == 0xf0 && opll.node) {
    opll.address(data);
  }

  else if((address & 0xff) == 0xf1 && opll.node) {
    opll.write(data);
  }

  else if((address & 0xff) == 0xf2 && opll.node) {
    if(Model::MarkIII()) data.bit(1) = 0;
    if(data.bit(0,1) == 0) psg.io.mute = 0, opll.io.mute = 1;
    if(data.bit(0,1) == 1) psg.io.mute = 1, opll.io.mute = 0;
    if(data.bit(0,1) == 2) psg.io.mute = 1, opll.io.mute = 1;
    if(data.bit(0,1) == 3) psg.io.mute = 0, opll.io.mute = 0;
  }

  debugger.out(address, data);
  return;
}
