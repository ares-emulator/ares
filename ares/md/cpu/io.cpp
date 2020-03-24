/* $a10000-bfffff */

auto CPU::readIO(uint1 upper, uint1 lower, uint24 address, uint16 data) -> uint16 {
  if(address >= 0xa10000 && address <= 0xa100ff) {
    if(!lower) return data;  //even byte writes ignored
    address.bit(5,7) = 0;   //a10020-a100ff mirrors a10000-a1001f

    switch(address) {
    case 0xa10000:
      data.bit(0) =  io.version;       //0 = Model 1; 1 = Model 2+
      data.bit(5) = !MegaCD();         //0 = expansion unit connected; 1 = no expansion unit connected
      data.bit(6) =  Region::PAL();    //0 = NTSC; 1 = PAL
      data.bit(7) = !Region::NTSCJ();  //0 = domestic (Japan); 1 = export
      break;

    case 0xa10002:
      data.byte(0) = controllerPort1.readData();
      break;

    case 0xa10004:
      data.byte(0) = controllerPort2.readData();
      break;

    case 0xa10006:
      data.byte(0) = extensionPort.readData();
      break;

    case 0xa10008:
      data.byte(0) = controllerPort1.readControl();
      break;

    case 0xa1000a:
      data.byte(0) = controllerPort2.readControl();
      break;

    case 0xa1000c:
      data.byte(0) = extensionPort.readControl();
      break;
    }

    return data;
  }

  if(address >= 0xa11100 && address <= 0xa111ff) {
    data.byte(1) = !apu.granted();
    return data;
  }

  return data;
}

auto CPU::writeIO(uint1 upper, uint1 lower, uint24 address, uint16 data) -> void {
  if(address >= 0xa10000 && address <= 0xa100ff) {
    if(!lower) return;      //even byte writes ignored
    address.bit(5,7) = 0;  //a10020-a100ff mirrors a10000-a1001f

    switch(address) {
    case 0xa10002:
      controllerPort1.writeData(data);
      break;

    case 0xa10004:
      controllerPort2.writeData(data);
      break;

    case 0xa10006:
      extensionPort.writeData(data);
      break;

    case 0xa10008:
      controllerPort1.writeControl(data);
      break;

    case 0xa1000a:
      controllerPort2.writeControl(data);
      break;

    case 0xa1000c:
      extensionPort.writeControl(data);
      break;
    }

    return;
  }

  if(address >= 0xa11100 && address <= 0xa111ff) {
    if(!upper) return;  //unconfirmed
    apu.request(data.bit(8));
    return;
  }

  if(address >= 0xa11200 && address <= 0xa112ff) {
    if(!upper) return;  //unconfirmed
    apu.enable(data.bit(8));
    return;
  }

  if(address == 0xa14000) {
    if(!tmss) return;
    if(!upper || !lower) return;  //unconfirmed
    io.vdpEnable[0] = data == 0x5345;
    return;
  }

  if(address == 0xa14002) {
    if(!tmss) return;
    if(!upper || !lower) return;  //unconfirmed
    io.vdpEnable[1] = data == 0x4741;
    return;
  }

  if(address == 0xa14100) {
    if(!tmss) return;
    if(!lower) return;
    io.romEnable = data.bit(0);
    return;
  }
}
