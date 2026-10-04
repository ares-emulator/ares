auto TIA::queueWrite(u8 address, n8 data, i8 delay) -> void {
  for(auto& write : writes) {
    if(!write.active) {
      write.address = address;
      write.data = data;
      write.delay = delay;
      write.active = 1;
      return;
    }
  }
}

auto TIA::clockWrites() -> void {
  for(auto& write : writes) {
    if(write.active && --write.delay == 0) {
      commitWrite(write.address, write.data);
      write.active = 0;
    }
  }
}

auto TIA::commitWrite(u8 address, n8 data) -> void {
  switch(address) {
  case 0x01: vblank = data.bit(1);                                                   return; // VBLANK
  case 0x0b: objects.player(0).reflect = data.bit(3);                                return; // REFP0
  case 0x0c: objects.player(1).reflect = data.bit(3);                                return; // REFP1
  case 0x0d: playfield.graphics.bit(0, 3) = data.bit(4, 7);                          return; // PF0
  case 0x0e: for(auto n : range(8)) playfield.graphics.bit(4 + n) = data.bit(7 - n); return; // PF1
  case 0x0f: playfield.graphics.bit(12, 19) = data;                                  return; // PF2
  case 0x1b: grp(0, data);                                                           return; // GRP0
  case 0x1c: grp(1, data);                                                           return; // GRP1
  case 0x1d: objects.missile(0).enable = data.bit(1);                                return; // ENAM0
  case 0x1e: objects.missile(1).enable = data.bit(1);                                return; // ENAM1
  case 0x1f: objects.ball.enable[0] = data.bit(1);                                   return; // ENABL
  case 0x20: objects.player(0).offset = data.bit(4, 7);                              return; // HMP0
  case 0x21: objects.player(1).offset = data.bit(4, 7);                              return; // HMP1
  case 0x22: objects.missile(0).offset = data.bit(4, 7);                             return; // HMM0
  case 0x23: objects.missile(1).offset = data.bit(4, 7);                             return; // HMM1
  case 0x24: objects.ball.offset = data.bit(4, 7);                                   return; // HMBL
  case 0x2a: hmove();                                                                return; // HMOVE
  case 0x2b: objects.hmclr();                                                        return; // HMCLR
  }
  debug(unimplemented, "[TIA] delayed write: ",hex(address), " = ", hex(data));
}
