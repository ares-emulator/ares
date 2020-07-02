auto SPU::Voice::sample(i16 modulation) -> std::pair<i32, i32> {
  if(adsr.phase == ADSR::Phase::Off) {
    adsr.lastVolume = 0;
    return {0, 0};
  }

  i32 l = 0;
  i32 r = 0;

  if(!adpcm.hasSamples) {
    readBlock();
    decodeBlock();
    adpcm.hasSamples = 1;

    if(block.loopStart && !adpcm.ignoreLoopAddress) {
      adpcm.repeatAddress = adpcm.currentAddress;
    }
  }

  i32 volume = 0;
  if(non) {
    volume = self.noise.level;
  } else {
    volume = gaussianInterpolate();
  }
  volume = amplify(volume, adsr.volume);
  adsr.lastVolume = volume;
  tickEnvelope();

  u16 step = adpcm.sampleRate;
  if(pmon) step = amplify(step, modulation + 0x8000);
  counter += uclamp<14>(step);
  if(counter >> 12 >= 28) {
    counter -= 28 << 12;
    adpcm.hasSamples = 0;
    adpcm.currentAddress += 16;
    if(block.loopEnd) {
      endx = 1;
      if(!block.loopRepeat) {
        forceOff();
      } else {
        adpcm.currentAddress = adpcm.repeatAddress & ~15;
      }
    }
  }

  l = amplify(volume, this->volume[0].level);
  r = amplify(volume, this->volume[1].level);
  //todo: sweep
  return {l, r};
}

auto SPU::Voice::tickEnvelope() -> void {
  if(adsr.phase == ADSR::Phase::Off) return;
  adsr.volume = envelope.tick(adsr.volume);
  if(envelope.decreasing == 0 && adsr.volume >= adsr.target) advancePhase();
  if(envelope.decreasing == 1 && adsr.volume <= adsr.target) advancePhase();
}

auto SPU::Voice::advancePhase() -> void {
  switch(adsr.phase) {
  case ADSR::Phase::Attack:  adsr.phase = ADSR::Phase::Decay;   return updateEnvelope();
  case ADSR::Phase::Decay:   adsr.phase = ADSR::Phase::Sustain; return updateEnvelope();
  case ADSR::Phase::Sustain: adsr.phase = ADSR::Phase::Sustain; return;  //holds until key off
  case ADSR::Phase::Release: adsr.phase = ADSR::Phase::Off;     return updateEnvelope();
  }
}

auto SPU::Voice::updateEnvelope() -> void {
  switch(adsr.phase) {
  case ADSR::Phase::Off:
    adsr.target = 0;
    envelope.reset(0, 0, 0);
    break;
  case ADSR::Phase::Attack:
    adsr.target = 0x7fff;
    envelope.reset(attack.rate, 0, attack.exponential);
    break;
  case ADSR::Phase::Decay:
    adsr.target = uclamp<15>(sustain.level + 1 << 11);
    envelope.reset(decay.rate << 2, 1, 1);
    break;
  case ADSR::Phase::Sustain:
    adsr.target = 0;
    envelope.reset(sustain.rate, sustain.decrease, sustain.exponential);
    break;
  case ADSR::Phase::Release:
    adsr.target = 0;
    envelope.reset(release.rate << 2, 1, release.exponential);
    break;
  }
}

auto SPU::Voice::forceOff() -> void {
  if(adsr.phase == ADSR::Phase::Off) return;
  adsr.phase = ADSR::Phase::Off;
  adsr.volume = 0;
}

auto SPU::Voice::keyOff() -> void {
  koff = 0;
  if(adsr.phase == ADSR::Phase::Off) return;
  if(adsr.phase == ADSR::Phase::Release) return;
  adsr.phase = ADSR::Phase::Release;
  updateEnvelope();
}

auto SPU::Voice::keyOn() -> void {
  kon = 0;
  adpcm.currentAddress = adpcm.startAddress & ~15;
  adpcm.hasSamples = 0;
  adpcm.ignoreLoopAddress = 0;
  adpcm.lastSamples[0] = 0;
  adpcm.lastSamples[1] = 0;
  adsr.phase = ADSR::Phase::Attack;
  adsr.volume = 0;
  updateEnvelope();
}
