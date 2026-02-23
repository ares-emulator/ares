auto TIA::WriteQueue::add(u8 address, n8 data, i8 delay) -> void {
  for(auto n : range(maxItems)) {
    if(!items[n].active) {
      items[n].address = address;
      items[n].data = data;
      items[n].delay = delay;
      items[n].active = 1;
      return;
    }
  }
}

auto TIA::WriteQueue::step() -> void {
  auto& items = tia.writeQueue.items;

  for(auto n : range(maxItems)) {
    if(items[n].active && --items[n].delay == 0) {
      items[n].commit();
      items[n].active = 0;
    }
  }
}

auto TIA::WriteQueue::QueueItem::commit() -> void {
  switch(address) {
  case 0x01: tia.io.vblank = data.bit(1);                                                return; // VBLANK
  case 0x0b: tia.player[0].reflect = data.bit(3);                                        return; // REFP0
  case 0x0c: tia.player[1].reflect = data.bit(3);                                        return; // REFP1
  case 0x0d: tia.playfield.graphics.bit(0, 3) = data.bit(4, 7);                          return; // PF0
  case 0x0e: for(auto n : range(8)) tia.playfield.graphics.bit(4 + n) = data.bit(7 - n); return; // PF1
  case 0x0f: tia.playfield.graphics.bit(12, 19) = data;                                  return; // PF2
  case 0x1b: tia.grp(0, data);                                                           return; // GRP0
  case 0x1c: tia.grp(1, data);                                                           return; // GRP1
  case 0x1d: tia.missile[0].enable = data.bit(1);                                        return; // ENAM0
  case 0x1e: tia.missile[1].enable = data.bit(1);                                        return; // ENAM1
  case 0x1f: tia.ball.enable[0] = data.bit(1);                                           return; // ENABL
  case 0x20: tia.player[0].offset = data.bit(4, 7);                                      return; // HMP0
  case 0x21: tia.player[1].offset = data.bit(4, 7);                                      return; // HMP1
  case 0x22: tia.missile[0].offset = data.bit(4, 7);                                     return; // HMM0
  case 0x23: tia.missile[1].offset = data.bit(4, 7);                                     return; // HMM1
  case 0x24: tia.ball.offset = data.bit(4, 7);                                           return; // HMBL
  case 0x2a: tia.hmove();                                                                return; // HMOVE
  case 0x2b: tia.hmclr();                                                                return; // HMCLR
  }
  debug(unimplemented, "[TIA] writeQueue: ",hex(address), " = ", hex(data));
}
