auto TIA::read(n8 address, n8 data) -> n8 {
  data.bit(6, 7) = 0;
  if(address <= 0x07) return collision.read(address, data);

  switch(address) {
  case 0x08: updateAnalogInput(0); data.bit(7) = analog.read(0); return data; // INPT0
  case 0x09: updateAnalogInput(1); data.bit(7) = analog.read(1); return data; // INPT1
  case 0x0a: updateAnalogInput(2); data.bit(7) = analog.read(2); return data; // INPT2
  case 0x0b: updateAnalogInput(3); data.bit(7) = analog.read(3); return data; // INPT3
  case 0x0c: data.bit(7) = readTrigger(0);                       return data; // INPT4
  case 0x0d: data.bit(7) = readTrigger(1);                       return data; // INPT5
  }

  return data;
}

auto TIA::write(n8 address, n8 data) -> void {
  switch(address) {
  case 0x00: setVsync(data.bit(1));                            return; // VSYNC
  case 0x01: writeVblank(data);                                return; // VBLANK
  case 0x02: if(timing.hcounter) cpu.rdyLine(0);               return; // WSYNC
  case 0x03: rsync();                                          return; // RSYNC
  case 0x04: objects.nusiz(0, data, timing.horizontalBlank()); return; // NUSIZ0
  case 0x05: objects.nusiz(1, data, timing.horizontalBlank()); return; // NUSIZ1
  case 0x06: priority.playerColor[0] = data.bit(1, 7);         return; // COLUP0
  case 0x07: priority.playerColor[1] = data.bit(1, 7);         return; // COLUP1
  case 0x08: priority.playfieldColor = data.bit(1, 7);         return; // COLUPF
  case 0x09: priority.backgroundColor = data.bit(1, 7);        return; // COLUBK
  case 0x0a: ctrlpf(data);                                     return; // CTRLPF
  case 0x0b: queueWrite(address, data, 1);                     return; // REFP0
  case 0x0c: queueWrite(address, data, 1);                     return; // REFP1
  case 0x0d: queueWrite(address, data, 2);                     return; // PF0
  case 0x0e: queueWrite(address, data, 2);                     return; // PF1
  case 0x0f: queueWrite(address, data, 2);                     return; // PF2
  case 0x10: resp(0);                                          return; // RESP0
  case 0x11: resp(1);                                          return; // RESP1
  case 0x12: resm(0);                                          return; // RESM0
  case 0x13: resm(1);                                          return; // RESM1
  case 0x14: resbl();                                          return; // RESBL
  case 0x15: audio.channel[0].control = data;                  return; // AUDC0
  case 0x16: audio.channel[1].control = data;                  return; // AUDC1
  case 0x17: audio.channel[0].frequency = data;                return; // AUDF0
  case 0x18: audio.channel[1].frequency = data;                return; // AUDF1
  case 0x19: audio.channel[0].volume = data;                   return; // AUDV0
  case 0x1a: audio.channel[1].volume = data;                   return; // AUDV1
  case 0x1b: queueWrite(address, data, 2);                     return; // GRP0
  case 0x1c: queueWrite(address, data, 2);                     return; // GRP1
  case 0x1d: queueWrite(address, data, 1);                     return; // ENAM0
  case 0x1e: queueWrite(address, data, 1);                     return; // ENAM1
  case 0x1f: queueWrite(address, data, 1);                     return; // ENAB1
  case 0x20: queueWrite(address, data, 2);                     return; // HMP0
  case 0x21: queueWrite(address, data, 2);                     return; // HMP1
  case 0x22: queueWrite(address, data, 2);                     return; // HMM0
  case 0x23: queueWrite(address, data, 2);                     return; // HMM1
  case 0x24: queueWrite(address, data, 2);                     return; // HMBL
  case 0x25: objects.player(0).delay = data.bit(0);            return; // VDELP0
  case 0x26: objects.player(1).delay = data.bit(0);            return; // VDELP1
  case 0x27: objects.ball.delay      = data.bit(0);            return; // VDELBL
  case 0x28: resmp(0, data);                                   return; // RESMP0
  case 0x29: resmp(1, data);                                   return; // RESMP1
  case 0x2a: queueWrite(address, data, 6);                     return; // HMOVE
  case 0x2b: queueWrite(address, data, 2);                     return; // HMCLR
  case 0x2c: collision.clear();                                return; // CXCLR
  }
}

auto TIA::writeVblank(n8 data) -> void {
  for(auto index : range(4)) {
    updateAnalogInput(index);
  }
  analog.vblank(data.bit(7));
  controllerPort1.vblank(data.bit(7));
  controllerPort2.vblank(data.bit(7));
  triggers.vblank(data.bit(6));
  queueWrite(0x01, data, 1);
}

auto TIA::setVsync(n1 state) -> void {
  if(vsync == state) return;
  vsync = state;
  video.vsync(state);
}

auto TIA::rsync() -> void {
  //Stella finishes the line after RSYNC's three-clock tail.
  auto position = timing.position();
  auto x = max(0, position - 68);
  video.fill(x, priority.backgroundColor);
  timing.rsync();
}

auto TIA::ctrlpf(n8 data) -> void {
  playfield.mirror    = data.bit(0);
  priority.scoreMode = data.bit(1);
  priority.playfieldPriority = data.bit(2);
  objects.ball.size   = data.bit(4, 5);
}

auto TIA::hmove() -> void {
  objects.hmove();
  if(timing.hcounter < 68) timing.extendedHblank = 1;
}

auto TIA::grp(n1 index, n8 data) -> void {
  objects.player(index).graphics[0] = data;
  objects.player(!index).graphics[1] = objects.player(!index).graphics[0];
  if(index) objects.ball.enable[1] = objects.ball.enable[0];
}

auto TIA::resp(n1 index) -> void {
  objects.player(index).reset(objectResetCounter());
}

auto TIA::resm(n1 index) -> void {
  objects.missile(index).reset(objectResetCounter(), timing.horizontalBlank());
}

auto TIA::resmp(n1 index, n8 data) -> void {
  objects.missile(index).lockedToPlayer = data.bit(1);
}

auto TIA::resbl() -> void {
  objects.ball.reset(objectResetCounter());
}
