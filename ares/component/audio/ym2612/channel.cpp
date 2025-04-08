auto YM2612::Channel::Operator::updateKeyState() -> void {
  if(keyOn == keyLine) return;  //no change
  keyOn = keyLine;

  if(keyOn) {
    //restart phase and envelope generators
    phase.value = 0;
    ssg.invert = false;
    envelope.state = Attack;
    updateEnvelope();

    if(envelope.rate >= 62) {
      //skip attack phase
      envelope.value = 0;
    }
  } else {
    envelope.state = Release;
    updateEnvelope();

    if(ssg.enable && ssg.attack != ssg.invert) {
      //SSG-EG key-off
      envelope.value = 0x200 - envelope.value;
    }
  }

  updateLevel();
}

auto YM2612::Channel::Operator::runEnvelope() -> void {
  if(ym2612.envelope.clock & (1 << envelope.divider) - 1) return;

  if(envelope.state == Attack && envelope.value == 0) {
    envelope.state = Decay;
    updateEnvelope();
  }

  u32 sustain = envelope.sustainLevel < 15 ? envelope.sustainLevel << 5 : 0x1f << 5;
  if(envelope.state == Decay && envelope.value >= sustain) {
    envelope.state = Sustain;
    updateEnvelope();
  }

  u32 value = ym2612.envelope.clock >> envelope.divider;
  u32 step = envelope.steps >> ((~value & 7) << 2) & 0xf;

  if(envelope.state == Attack) {
    // will stop updating if attack rate is increased to upper threshold during attack phase (confirmed behavior)
    if(envelope.rate < 62) envelope.value += ~u16(envelope.value) * step >> 4;
  }
  if(envelope.state != Attack) {
    if(ssg.enable) step = envelope.value < 0x200 ? step << 2 : 0;  //SSG results in a 4x faster envelope
    envelope.value = min(envelope.value + step, 0x3ff);
  }

  updateLevel();
}

auto YM2612::Channel::Operator::runPhase() -> void {
  updateKeyState();
  phase.value += phase.delta;  //advance wave position
  if(!(ssg.enable && envelope.value >= 0x200)) return;  //SSG loop check

  if(!ssg.hold && !ssg.alternate) phase.value = 0;
  if(!(ssg.hold && ssg.invert)) ssg.invert ^= ssg.alternate;

  if(envelope.state == Attack) {
    //do nothing; SSG is meant to skip the attack phase
  } else if(envelope.state != Release && !ssg.hold) {
    //if still looping, reset the envelope
    envelope.state = Attack;
    updateEnvelope();

    if(envelope.rate >= 62) {
      //skip attack phase
      envelope.value = 0;
    }
  } else if(envelope.state == Release || (ssg.hold && ssg.attack == ssg.invert)) {
    //clear envelope once finished
    envelope.value = 0x3ff;
  }

  updateLevel();
}

auto YM2612::Channel::Operator::updateEnvelope() -> void {
  u32 rate = 0;

  if(envelope.state == Attack)  rate += (envelope.attackRate  << 1);
  if(envelope.state == Decay)   rate += (envelope.decayRate   << 1);
  if(envelope.state == Sustain) rate += (envelope.sustainRate << 1);
  if(envelope.state == Release) rate += (envelope.releaseRate << 1);

  rate += (keyScale >> 3 - envelope.rateScaling) * (rate > 0);
  rate  = min(rate, 63);

  auto& entry = envelopeRates[rate >> 2];
  envelope.rate    = rate;
  envelope.divider = entry.divider;
  envelope.steps   = entry.steps[rate & 3];
}

auto YM2612::Channel::Operator::updatePitch() -> void {
  //only channel[2] allows per-operator frequencies
  //implemented by forcing mode to zero (single frequency) for other channels
  //in single frequency mode, operator[3] frequency is used for all operators
  pitch.value = channel.mode ? pitch.reload : channel[3].pitch.reload;
  octave.value = channel.mode ? octave.reload : channel[3].octave.reload;

  u32 key = min(max((u32)pitch.value, 0x300), 0x4ff);
  keyScale = (octave.value << 2) + ((key - 0x300) >> 7);

  updatePhase();
  updateEnvelope();  //due to key scaling
}

auto YM2612::Channel::Operator::updatePhase() -> void {
  u32 tuning = detune & 3 ? detunes[(detune & 3) - 1][keyScale & 7] >> (3 - (keyScale >> 3)) : 0;
  u32 lfo = ym2612.lfo.clock >> 2 & 0x1f;
  s32 pm = (pitch.value * vibratos[channel.vibrato][lfo & 15] >> 9) * (lfo > 15 ? -1 : 1);

  phase.delta = pitch.value + pm << 6 >> 7 - octave.value;
  phase.delta = (!detune.bit(2) ? phase.delta + tuning : phase.delta - tuning) & 0x1ffff;
  phase.delta = (multiple ? phase.delta * multiple : phase.delta >> 1) & 0xfffff;
}

auto YM2612::Channel::Operator::updateLevel() -> void {
  u32 lfo = ym2612.lfo.clock & 0x40 ? ym2612.lfo.clock & 0x3f : ~ym2612.lfo.clock & 0x3f;
  u32 depth = tremolos[tremoloEnable * channel.tremolo];

  bool invert = ssg.attack != ssg.invert && envelope.state != Release;
  n10 value = ssg.enable && invert ? 0x200 - envelope.value : 0 + envelope.value;

  outputLevel = (totalLevel << 3) + value + (lfo << 1 >> depth) << 3;
}

auto YM2612::Channel::power() -> void {
  leftEnable = 1;
  rightEnable = 1;

  algorithm = 0;
  feedback = 0;
  vibrato = 0;
  tremolo = 0;

  mode = 0;

  for(auto& op : operators) {
    op.keyOn = 0;
    op.keyLine = 0;
    op.tremoloEnable = 0;
    op.keyScale = 0;
    op.detune = 0;
    op.multiple = 0;
    op.totalLevel = 0;

    op.outputLevel = 0x1fff;
    op.output = 0;
    op.prior = 0;
    op.priorBuffer = 0;

    op.pitch.value = 0;
    op.pitch.reload = 0;
    op.pitch.latch = 0;

    op.octave.value = 0;
    op.octave.reload = 0;
    op.octave.latch = 0;

    op.phase.value = 0;
    op.phase.delta = 0;

    op.envelope.state = Release;
    op.envelope.rate = 0;
    op.envelope.divider = 11;
    op.envelope.steps = 0;
    op.envelope.value = 0x3ff;

    op.envelope.rateScaling = 0;
    op.envelope.attackRate = 0;
    op.envelope.decayRate = 0;
    op.envelope.sustainRate = 0;
    op.envelope.sustainLevel = 0;
    op.envelope.releaseRate = 1;

    op.ssg.enable = 0;
    op.ssg.attack = 0;
    op.ssg.alternate = 0;
    op.ssg.hold = 0;
    op.ssg.invert = 0;

    op.updatePitch();
    op.updateLevel();
  }
}
