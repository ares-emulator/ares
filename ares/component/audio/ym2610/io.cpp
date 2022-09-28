auto YM2610::read(n2 address) -> n8 {
  switch(address) {
  case 0: return fm.readStatus();
  case 1: return ssg.read();

  }

  debug(unimplemented, "[YM2610] read ", hex(address));
  return 0;
}

auto YM2610::write(n2 address, n8 data) -> void {
  switch(address) {
  case 0: {
    registerAddress = 0x000 | data;
    if (registerAddress <= 0xf) ssg.select(registerAddress);
    else fm.writeAddress(registerAddress);
    return;
  }
  case 1: return writeLower(data);
  case 2: registerAddress = 0x100 | data; fm.writeAddress(registerAddress); return;
  case 3: return writeUpper(data);
  }
}

auto YM2610::writeLower(n8 data) -> void {
  switch(registerAddress) {
  case 0x000 ... 0x00f:
    ssg.write(data);
    return;
  case 0x010:
              pcmB.repeat  = data.bit(4);
              if(data.bit(0)) pcmB.power();
              if(data.bit(7)) pcmB.beginPlay();
              else            pcmB.playing = 0;
              return;
  case 0x011: pcmB.left    = data.bit(7);
              pcmB.right   = data.bit(6);           return;
  case 0x012: pcmB.startAddress.bit( 8, 15) = data; return;
  case 0x013: pcmB.startAddress.bit(16, 23) = data; return;
  case 0x014: pcmB.endAddress.bit  ( 8, 15) = data; return;
  case 0x015: pcmB.endAddress.bit  (16, 23) = data; return;
  case 0x016 ... 0x018: debug(unimplemented, "[YM2610::ADPCMB] Write ", hex(registerAddress), " = ", data);
  case 0x019: pcmB.delta.bit       ( 0,  7) = data; return;
  case 0x01a: pcmB.delta.bit       ( 8, 15) = data; return;
  case 0x01b: pcmB.volume                   = data; return;
  case 0x01c:
    //EOS
    debug(unimplemented, "[YM2610::EOS] Write ", hex(registerAddress), " = ", data);
    return;
  case 0x01d ... 0x1ff:
    fm.writeData(data);
    return;
  }

  debug(unimplemented, "[YM2610] Write ", hex(registerAddress), " = ", data);
}

auto YM2610::writeUpper(n8 data) -> void {
  switch(registerAddress) {
  case 0x100:
    for(auto n : range(6)) {
      if(!data.bit(n)) continue;
      if(data.bit(7)) {
        pcmA.channels[n].keyOff();
        continue;
      }

      pcmA.channels[n].keyOn();
    }
    return;
  case 0x101:           pcmA.volume = data.bit(0, 5);                                           return;
  case 0x108 ... 0x10d: pcmA.channels[registerAddress - 0x108].volume = data.bit(0,4);
                        pcmA.channels[registerAddress - 0x108].left   = data.bit(7);
                        pcmA.channels[registerAddress - 0x108].right  = data.bit(6);            return;
  case 0x110 ... 0x115: pcmA.channels[registerAddress - 0x110].startAddress.bit( 8, 15) = data; return;
  case 0x118 ... 0x11d: pcmA.channels[registerAddress - 0x118].startAddress.bit(16, 23) = data; return;
  case 0x120 ... 0x125: pcmA.channels[registerAddress - 0x120].endAddress.bit  ( 8, 15) = data; return;
  case 0x128 ... 0x12d: pcmA.channels[registerAddress - 0x128].endAddress.bit  (16, 23) = data; return;
  case 0x130 ... 0x1ff: fm.writeAddress(registerAddress); fm.writeData(data);                   return;
  }

  debug(unimplemented, "[YM2610] Write ", hex(registerAddress), " = ", data);
}
