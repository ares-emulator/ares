auto YM2612::readStatus() -> uint8 {
  //d7 = busy (not emulated, requires cycle timing accuracy)
  return timerA.line << 0 | timerB.line << 1;
}

auto YM2612::writeAddress(uint9 data) -> void {
  io.address = data;
}

auto YM2612::writeData(uint8 data) -> void {
  switch(io.address) {

  //LFO
  case 0x022: {
    lfo.rate = data.bit(0,2);
    lfo.enable = data.bit(3);
    break;
  }

  //timer A period (high)
  case 0x024: {
    timerA.period.bit(2,9) = data.bit(0,7);
    break;
  }

  //timer A period (low)
  case 0x025: {
    timerA.period.bit(0,1) = data.bit(0,1);
    break;
  }

  //timer B period
  case 0x026: {
    timerB.period.bit(0,7) = data.bit(0,7);
    break;
  }

  //timer control
  case 0x027: {
    //d6,d7 = mode (unimplemented; treated as mode 0 always)

    //reload period on 0->1 transition
    if(!timerA.enable && data.bit(0)) timerA.counter = timerA.period;
    if(!timerB.enable && data.bit(1)) timerB.counter = timerB.period;

    timerA.enable = data.bit(0);
    timerB.enable = data.bit(1);
    timerA.irq = data.bit(2);
    timerB.irq = data.bit(3);

    if(data.bit(4)) timerA.line = 0;
    if(data.bit(5)) timerB.line = 0;

    channels[2].mode = data.bit(6,7);
    for(auto& op : channels[2].operators) op.updatePitch();

    break;
  }

  //key on/off
  case 0x28: {
    //0,1,2,4,5,6 => 0,1,2,3,4,5
    uint index = data.bit(0,2);
    if(index == 3 || index == 7) break;
    if(index >= 4) index--;

    channels[index][0].trigger(data.bit(4));
    channels[index][1].trigger(data.bit(5));
    channels[index][2].trigger(data.bit(6));
    channels[index][3].trigger(data.bit(7));

    break;
  }

  //DAC sample
  case 0x2a: {
    dac.sample = data;
    break;
  }

  //DAC enable
  case 0x2b: {
    dac.enable = data.bit(7);
    break;
  }

  }

  if(io.address.bit(0,1) == 3) return;
  uint3 voice = io.address.bit(8) * 3 + io.address.bit(0,1);
  uint2 index = io.address.bit(2,3) >> 1 | io.address.bit(2,3) << 1;  //0,1,2,3 => 0,2,1,3

  auto& channel = channels[voice];
  auto& op = channel.operators[index];

  switch(io.address & 0x0f0) {

  //detune, multiple
  case 0x030: {
    op.multiple = data.bit(0,3);
    op.detune = data.bit(4,6);
    channel[index].updatePhase();
    break;
  }

  //total level
  case 0x040: {
    op.totalLevel = data.bit(0,6);
    channel[index].updateLevel();
    break;
  }

  //key scaling, attack rate
  case 0x050: {
    op.envelope.attackRate = data.bit(0,4);
    op.envelope.keyScale = data.bit(6,7);
    channel[index].updateEnvelope();
    channel[index].updatePhase();
    break;
  }

  //LFO enable, decay rate
  case 0x060: {
    op.envelope.decayRate = data.bit(0,4);
    op.lfoEnable = data.bit(7);
    channel[index].updateEnvelope();
    channel[index].updateLevel();
    break;
  }

  //sustain rate
  case 0x070: {
    op.envelope.sustainRate = data.bit(0,4);
    channel[index].updateEnvelope();
    break;
  }

  //sustain level, release rate
  case 0x080: {
    op.envelope.releaseRate = data.bit(0,3) << 1 | 1;
    op.envelope.sustainLevel = data.bit(4,7);
    channel[index].updateEnvelope();
    break;
  }

  //SSG-EG
  case 0x090: {
    op.ssg.hold = data.bit(0);
    op.ssg.alternate = data.bit(1);
    op.ssg.attack = data.bit(2);
    op.ssg.enable = data.bit(3);
    break;
  }

  }

  switch(io.address & 0x0fc) {

  //pitch (low)
  case 0x0a0: {
    channel[3].pitch.reload = channel[3].pitch.latch | data;
    channel[3].octave.reload = channel[3].octave.latch;
    for(uint index : range(4)) channel[index].updatePitch();
    break;
  }

  //pitch (high)
  case 0x0a4: {
    channel[3].pitch.latch = data << 8;
    channel[3].octave.latch = data >> 3;
    break;
  }

  //per-operator pitch (low)
  case 0x0a8: {
    if(io.address == 0x0a9) index = 0;
    if(io.address == 0x0aa) index = 1;
    if(io.address == 0x0a8) index = 2;
    channels[2][index].pitch.reload = channels[2][index].pitch.latch | data;
    channels[2][index].octave.reload = channels[2][index].octave.latch;
    channels[2][index].updatePitch();
    break;
  }

  //per-operator pitch (high)
  case 0x0ac: {
    if(io.address == 0x0ad) index = 0;
    if(io.address == 0x0ae) index = 1;
    if(io.address == 0x0ac) index = 2;
    channels[2][index].pitch.latch = data << 8;
    channels[2][index].octave.latch = data >> 3;
    break;
  }

  //algorithm, feedback
  case 0x0b0: {
    channel.algorithm = data.bit(0,2);
    channel.feedback = data.bit(3,5);
    break;
  }

  //panning, tremolo, vibrato
  case 0x0b4: {
    channel.vibrato = data.bit(0,2);
    channel.tremolo = data.bit(4,5);
    channel.rightEnable = data.bit(6);
    channel.leftEnable = data.bit(7);
    for(uint index : range(4)) {
      channel[index].updateLevel();
      channel[index].updatePhase();
    }
    break;
  }

  }
}
