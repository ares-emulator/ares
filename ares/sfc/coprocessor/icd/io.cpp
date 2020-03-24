auto ICD::readIO(uint24 address, uint8 data) -> uint8 {
  address &= 0x40ffff;

  //LY counter
  if(address == 0x6000) {
    return vcounter & ~7 | writeBank;
  }

  //command ready port
  if(address == 0x6002) {
    data = packetSize > 0;
    if(data) {
      for(auto n : range(16)) r7000[n] = packet[0][n];
      packetSize--;
      for(auto n : range(packetSize)) packet[n] = packet[n + 1];
    }
    return data;
  }

  //ICD2 revision
  if(address == 0x600f) {
    return 0x21;
  }

  //command port
  if((address & 0x40fff0) == 0x7000) {
    return r7000[address & 15];
  }

  //VRAM port
  if(address == 0x7800) {
    data = output[readBank * 512 + readAddress++];
    return data;
  }

  return 0x00;
}

auto ICD::writeIO(uint24 address, uint8 data) -> void {
  address &= 0xffff;

  //VRAM port
  if(address == 0x6001) {
    readBank = data.bit(0,1);
    readAddress = 0;
    return;
  }

  //control port
  //d7: 0 = halt, 1 = reset
  //d5,d4: 0 = 1-player, 1 = 2-player, 2 = 4-player, 3 = ???
  //d1,d0: 0 = frequency divider (clock rate adjust)
  if(address == 0x6003) {
    if(r6003.bit(7) == 0 && data.bit(7) == 1) {
      power(true);  //soft reset
    }

    mltReq = data.bit(4,5);
    if(mltReq == 0) joypID &= ~0;  //1-player mode
    if(mltReq == 1) joypID &= ~1;  //2-player mode
    if(mltReq == 2) joypID &= ~3;  //4-player mode (unverified; but the most likely behavior)
    if(mltReq == 3) joypID &= ~3;  //4-player mode

    auto frequency = system.cpuFrequency();
    switch(data.bit(0,1)) {
    case 0: frequency /= 4; break;  //fast (glitchy, even on real hardware)
    case 1: frequency /= 5; break;  //normal
    case 2: frequency /= 7; break;  //slow
    case 3: frequency /= 9; break;  //very slow
    }
    Thread::setFrequency(frequency);
    GameBoy::apu.stream->setFrequency(frequency / 2.0);
    r6003 = data;
    return;
  }

  if(address == 0x6004) { r6004 = data; return; }  //joypad 1
  if(address == 0x6005) { r6005 = data; return; }  //joypad 2
  if(address == 0x6006) { r6006 = data; return; }  //joypad 3
  if(address == 0x6007) { r6007 = data; return; }  //joypad 4
}
