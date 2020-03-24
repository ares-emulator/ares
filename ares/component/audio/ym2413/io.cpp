auto YM2413::address(uint8 data) -> void {
  io.address = data;
}

auto YM2413::write(uint8 data) -> void {
  uint4 index = io.address.bit(0,3) % 9;
  auto& voice = voices[index];

  if(io.address >= 0x00 && io.address <= 0x07) {
    customTone[io.address] = data;
    for(uint n : range(9)) {
      if(voices[n].tone == 0) reload(n);
    }
    return;
  }

  if(io.address == 0x0e) {
    if(io.isVRC7) return;
    if(io.rhythmMode != data.bit(5)) {
      io.rhythmMode = data.bit(5);
      for(uint n : range(9)) reload(n);
    }
    if(io.rhythmMode) {
      hihat.trigger(data.bit(0));
      cymbal.trigger(data.bit(1));
      tomtom.trigger(data.bit(2));
      snare.trigger(data.bit(3));
      bass.carrier.trigger(data.bit(4));
    }
    return;
  }

  if(io.address == 0x0f) {
    //test register is currently unemulated
    return;
  }

  if(io.address >= 0x10 && io.address <= 0x1f) {  //0x19+ mirrors 0x10+
    if(io.isVRC7 && index >= 6) return;
    voice.fnumber.bit(0,7) = data;
    voice.update();
    return;
  }

  if(io.address >= 0x20 && io.address <= 0x2f) {  //0x29+ mirrors 0x20+
    if(io.isVRC7 && index >= 6) return;
    voice.fnumber.bit(8) = data.bit(0);
    voice.block          = data.bit(1,3);
    uint1 keyOn          = data.bit(4);
    uint1 sustainOn      = data.bit(5);
    voice.update();
    voice.carrier.trigger(keyOn, sustainOn);
    return;
  }

  if(io.address >= 0x30 && io.address <= 0x3f) {  //0x39+ mirrors 0x30+
    if(io.isVRC7 && index >= 6) return;
    voice.level = data.bit(0,3);
    voice.tone  = data.bit(4,7);
    if(io.rhythmMode && index >= 6) voice.update();
    else if(voice.tone) voice.update(melodicTones[voice.tone - 1]);
    else voice.update(customTone);
    return;
  }
}
