auto TIA::read(n8 address) -> n8 {
  n8 value;

  switch(address) {
  case 0x00: value.bit(7) = collision.M0P1; value.bit(6) = collision.M0P0; return value; // CXM0P
  case 0x01: value.bit(7) = collision.M1P0; value.bit(6) = collision.M1P1; return value; // CXM1P
  case 0x02: value.bit(7) = collision.P0PF; value.bit(6) = collision.P0BL; return value; // CX0FB
  case 0x03: value.bit(7) = collision.P1PF; value.bit(6) = collision.P1BL; return value; // CX1FB
  case 0x04: value.bit(7) = collision.M0PF; value.bit(6) = collision.M0BL; return value; // CXM0FB
  case 0x05: value.bit(7) = collision.M1PF; value.bit(6) = collision.M1BL; return value; // CXM1FB
  case 0x06: value.bit(7) = collision.BLPF;                                return value; // CXBLPF
  case 0x07: value.bit(7) = collision.P0P1; value.bit(6) = collision.M0M1; return value; // CXPPMM
  case 0x0c: value.bit(7) = controllerPort1.read().bit(4);                 return value; // INPT4
  case 0x0d: value.bit(7) = controllerPort2.read().bit(4);                 return value; // INPT5
  }

  debug(unimplemented, "[TIA] read ", hex(address));
  return 0xff;
}

auto TIA::write(n8 address, n8 data) -> void {
  switch(address) {
  case 0x00: vsync(data.bit(1));                                               return; // VSYNC
  case 0x01: writeQueue.add(address, data, 1);                                 return; // VBLANK
  case 0x02: cpu.rdyLine(0);                                                   return; // WSYNC
  case 0x03: io.hcounter = 0;                                                  return; // RSYNC
  case 0x04: player[0].size = data.bit(0,2); missile[0].size = data.bit(4, 5); return; // NUSIZ1
  case 0x05: player[1].size = data.bit(0,2); missile[1].size = data.bit(4, 5); return; // NUSIZ1
  case 0x06: io.p0Color = data.bit(1, 7);                                      return; // COLUP0
  case 0x07: io.p1Color = data.bit(1, 7);                                      return; // COLUP1
  case 0x08: io.fgColor = data.bit(1, 7);                                      return; // COLUPF
  case 0x09: io.bgColor = data.bit(1, 7);                                      return; // COLUBK
  case 0x0a: ctrlpf(data);                                                     return; // CTRLPF
  case 0x0b: writeQueue.add(address, data, 1);                                 return; // REFP0
  case 0x0c: writeQueue.add(address, data, 1);                                 return; // REFP1
  case 0x0d: writeQueue.add(address, data, 2);                                 return; // PF0
  case 0x0e: writeQueue.add(address, data, 2);                                 return; // PF1
  case 0x0f: writeQueue.add(address, data, 2);                                 return; // PF2
  case 0x10: resp(0);                                                          return; // RESP0
  case 0x11: resp(1);                                                          return; // RESP1
  case 0x12: resm(0);                                                          return; // RESM0
  case 0x13: resm(1);                                                          return; // RESM1
  case 0x14: resbl();                                                          return; // RESBL
  case 0x15: audio[0].control = data;                                          return; // AUDC0
  case 0x16: audio[1].control = data;                                          return; // AUDC1
  case 0x17: audio[0].frequency = data;                                        return; // AUDF0
  case 0x18: audio[1].frequency = data;                                        return; // AUDF1
  case 0x19: audio[0].volume = data;                                           return; // AUDV0
  case 0x1a: audio[1].volume = data;                                           return; // AUDV1
  case 0x1b: writeQueue.add(address, data, 1);                                 return; // GRP0
  case 0x1c: writeQueue.add(address, data, 1);                                 return; // GRP1
  case 0x1d: writeQueue.add(address, data, 1);                                 return; // ENAM0
  case 0x1e: writeQueue.add(address, data, 1);                                 return; // ENAM1
  case 0x1f: writeQueue.add(address, data, 1);                                 return; // ENAB1
  case 0x20: writeQueue.add(address, data, 2);                                 return; // HMP0
  case 0x21: writeQueue.add(address, data, 2);                                 return; // HMP1
  case 0x22: writeQueue.add(address, data, 2);                                 return; // HMM0
  case 0x23: writeQueue.add(address, data, 2);                                 return; // HMM1
  case 0x24: writeQueue.add(address, data, 2);                                 return; // HMBL
  case 0x25: player[0].delay = data.bit(0);                                    return; // VDELP0
  case 0x26: player[1].delay = data.bit(0);                                    return; // VDELP1
  case 0x27: ball.delay      = data.bit(0);                                    return; // VDELBL
  case 0x28: resmp(0, data);                                                   return; // RESMP0
  case 0x29: resmp(1, data);                                                   return; // RESMP1
  case 0x2a: writeQueue.add(address, data, 6);                                 return; // HMOVE
  case 0x2b: writeQueue.add(address, data, 2);                                 return; // HMCLR
  case 0x2c: cxclr();                                                          return; // CXCLR
  }

  debug(unimplemented, "[TIA] write ",hex(address), " = ", hex(data));
}


auto TIA::vsync(n1 state) -> void {
  if(!io.vsync && state) {
    frame();
    tia.io.vcounter = 0;
  }

  io.vsync = state;
}

auto TIA::ctrlpf(n8 data) -> void {
  playfield.mirror    = data.bit(0);
  playfield.scoreMode = data.bit(1);
  playfield.priority  = data.bit(2);
  ball.size           = data.bit(4, 5);
}

auto TIA::cxclr() -> void {
  collision.M0P0 = 0;
  collision.M0P1 = 0;
  collision.M1P0 = 0;
  collision.M1P1 = 0;
  collision.P0PF = 0;
  collision.P0BL = 0;
  collision.P1PF = 0;
  collision.P1BL = 0;
  collision.M0PF = 0;
  collision.M0BL = 0;
  collision.M1PF = 0;
  collision.M1BL = 0;
  collision.BLPF = 0;
  collision.P0P1 = 0;
  collision.M0M1 = 0;
}

auto TIA::hmove() -> void {
  u4 mask = 1 << 3;
  for(auto& p : player) p.step(p.offset ^ mask);
  for(auto& m : missile) m.step(m.offset ^ mask);
  ball.step(ball.offset ^ mask);

  io.hmoveTriggered = 1;
}

auto TIA::hmclr() -> void {
  for(auto& p : player)  p.offset = 0;
  for(auto& m : missile) m.offset = 0;
  ball.offset = 0;
}

auto TIA::grp(n1 index, n8 data) -> void {
  player[index].graphics[0] = data;
  player[!index].graphics[1] = player[!index].graphics[0];
  if(index) ball.enable[1] = ball.enable[0];
}

auto TIA::resp(n1 index) -> void {
  player[index].reset();
}

auto TIA::resm(n1 index) -> void {
  missile[index].reset();
}

auto TIA::resmp(n1 index, n8 data) -> void {
  missile[index].lockedToPlayer = data.bit(1);
}

auto TIA::resbl() -> void {
  ball.reset();
}
