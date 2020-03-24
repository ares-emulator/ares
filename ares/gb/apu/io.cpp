auto APU::readIO(uint cycle, uint16 address, uint8 data) -> uint8 {
  if(address < 0xff10 || address > 0xff3f) return data;

  //NR10
  if(address == 0xff10 && cycle == 2) {
    data.bit(0,2) = square1.sweepShift;
    data.bit(3)   = square1.sweepDirection;
    data.bit(4,6) = square1.sweepFrequency;
    return data;
  }

  //NR11
  if(address == 0xff11 && cycle == 2) {
    data.bit(6,7) = square1.duty;
    return data;
  }

  //NR12
  if(address == 0xff12 && cycle == 2) {
    data.bit(0,2) = square1.envelopeFrequency;
    data.bit(3)   = square1.envelopeDirection;
    data.bit(4,7) = square1.envelopeVolume;
    return data;
  }

  //NR13
  if(address == 0xff13 && cycle == 2) {
    return data;
  }

  //NR14
  if(address == 0xff14 && cycle == 2) {
    data.bit(6) = square1.counter;
    return data;
  }

  //NR20
  if(address == 0xff15 && cycle == 2) {
    return data;
  }

  //NR21
  if(address == 0xff16 && cycle == 2) {
    data.bit(6,7) = square2.duty;
    return data;
  }

  //NR22
  if(address == 0xff17 && cycle == 2) {
    data.bit(0,2) = square2.envelopeFrequency;
    data.bit(3)   = square2.envelopeDirection;
    data.bit(4,7) = square2.envelopeVolume;
    return data;
  }

  //NR23
  if(address == 0xff18 && cycle == 2) {
    return data;
  }

  //NR24
  if(address == 0xff19 && cycle == 2) {
    data.bit(6) = square2.counter;
    return data;
  }

  //NR30
  if(address == 0xff1a && cycle == 2) {
    data.bit(7) = wave.dacEnable;
    return data;
  }

  //NR31
  if(address == 0xff1b && cycle == 2) {
    return data;
  }

  //NR32
  if(address == 0xff1c && cycle == 2) {
    data.bit(5,6) = wave.volume;
    return data;
  }

  //NR33
  if(address == 0xff1d && cycle == 2) {
    return data;
  }

  //NR34
  if(address == 0xff1e && cycle == 2) {
    data.bit(6) = wave.counter;
    return data;
  }

  //NR40
  if(address == 0xff1f && cycle == 2) {
    return data;
  }

  //NR41
  if(address == 0xff20 && cycle == 2) {
    return data;
  }

  //NR42
  if(address == 0xff21 && cycle == 2) {
    data.bit(0,2) = noise.envelopeFrequency;
    data.bit(3)   = noise.envelopeDirection;
    data.bit(4,7) = noise.envelopeVolume;
    return data;
  }

  //NR43
  if(address == 0xff22 && cycle == 2) {
    data.bit(0,2) = noise.divisor;
    data.bit(3)   = noise.narrow;
    data.bit(4,7) = noise.frequency;
    return data;
  }

  //NR44
  if(address == 0xff23 && cycle == 2) {
    data.bit(6) = noise.counter;
    return data;
  }

  //NR50
  if(address == 0xff24 && cycle == 2) {
    data.bit(0,2) = sequencer.rightVolume;
    data.bit(3)   = sequencer.rightEnable;
    data.bit(4,6) = sequencer.leftVolume;
    data.bit(7)   = sequencer.leftEnable;
    return data;
  }

  //NR51
  if(address == 0xff25 && cycle == 2) {
    data.bit(0) = sequencer.square1.rightEnable;
    data.bit(1) = sequencer.square2.rightEnable;
    data.bit(2) = sequencer.wave.rightEnable;
    data.bit(3) = sequencer.noise.rightEnable;
    data.bit(4) = sequencer.square1.leftEnable;
    data.bit(5) = sequencer.square2.leftEnable;
    data.bit(6) = sequencer.wave.leftEnable;
    data.bit(7) = sequencer.noise.leftEnable;
    return data;
  }

  //NR52
  if(address == 0xff26) {
    data.bit(0) = square1.enable;
    data.bit(1) = square2.enable;
    data.bit(2) = wave.enable;
    data.bit(3) = noise.enable;
    data.bit(7) = sequencer.enable;
    return data;
  }

  if(address >= 0xff30 && address <= 0xff3f && cycle == 2) {
    return wave.readRAM(address, data);
  }

  return data;
}

auto APU::writeIO(uint cycle, uint16 address, uint8 data) -> void {
  if(address < 0xff10 || address > 0xff3f) return;

  if(!sequencer.enable) {
    bool valid = address == 0xff26;  //NR52
    if(!Model::GameBoyColor()) {
      //NRx1 length is writable only on DMG,SGB; not on CGB
      if(address == 0xff11) valid = true, data &= 0x3f;  //NR11; duty is not writable (remains 0)
      if(address == 0xff16) valid = true, data &= 0x3f;  //NR21; duty is not writable (remains 0)
      if(address == 0xff1b) valid = true;  //NR31
      if(address == 0xff20) valid = true;  //NR41
    }
    if(!valid) return;
  }

  //NR10
  if(address == 0xff10 && cycle == 2) {
    if(square1.sweepEnable && square1.sweepNegate && !data.bit(3)) square1.enable = 0;
    square1.sweepShift     = data.bit(0,2);
    square1.sweepDirection = data.bit(3);
    square1.sweepFrequency = data.bit(4,6);
    return;
  }

  //NR11
  if(address == 0xff11 && cycle == 2) {
    square1.length = 64 - data.bit(0,5);
    square1.duty   = data.bit(6,7);
    return;
  }

  //NR12
  if(address == 0xff12 && cycle == 2) {
    square1.envelopeFrequency = data.bit(0,2);
    square1.envelopeDirection = data.bit(3);
    square1.envelopeVolume    = data.bit(4,7);
    if(!square1.dacEnable()) square1.enable = 0;
    return;
  }

  //NR13
  if(address == 0xff13 && cycle == 2) {
    square1.frequency.bit(0,7) = data;
    return;
  }

  //NR14
  if(address == 0xff14 && cycle == 4) {
    if(phase.bit(0) && !square1.counter && data.bit(6)) {
      if(square1.length && --square1.length == 0) square1.enable = 0;
    }
    square1.frequency.bit(8,10) = data.bit(0,2);
    square1.counter = data.bit(6);
    if(data.bit(7)) square1.trigger();
    return;
  }

  //NR20
  if(address == 0xff15 && cycle == 2) {
    return;
  }

  //NR21
  if(address == 0xff16 && cycle == 2) {
    square2.length = 64 - data.bit(0,5);
    square2.duty   = data.bit(6,7);
    return;
  }

  //NR22
  if(address == 0xff17 && cycle == 2) {
    square2.envelopeFrequency = data.bit(0,2);
    square2.envelopeDirection = data.bit(3);
    square2.envelopeVolume    = data.bit(4,7);
    if(!square2.dacEnable()) square2.enable = 0;
    return;
  }

  //NR23
  if(address == 0xff18 && cycle == 2) {
    square2.frequency.bit(0,7) = data;
    return;
  }

  //NR24
  if(address == 0xff19 && cycle == 4) {
    if(phase.bit(0) && !square2.counter && data.bit(6)) {
      if(square2.length && --square2.length == 0) square2.enable = 0;
    }
    square2.frequency.bit(8,10) = data.bit(0,2);
    square2.counter = data.bit(6);
    if(data.bit(7)) square2.trigger();
    return;
  }

  //NR30
  if(address == 0xff1a && cycle == 2) {
    wave.dacEnable = data.bit(7);
    if(!wave.dacEnable) wave.enable = 0;
    return;
  }

  //NR31
  if(address == 0xff1b && cycle == 2) {
    wave.length = 256 - data;
    return;
  }

  //NR32
  if(address == 0xff1c && cycle == 2) {
    wave.volume = data.bit(5,6);
    return;
  }

  //NR33
  if(address == 0xff1d && cycle == 2) {
    wave.frequency.bit(0,7) = data;
    return;
  }

  //NR34
  if(address == 0xff1e && cycle == 4) {
    if(phase.bit(0) && !wave.counter && data.bit(6)) {
      if(wave.length && --wave.length == 0) wave.enable = 0;
    }
    wave.frequency.bit(8,10) = data.bit(0,2);
    wave.counter = data.bit(6);
    if(data.bit(7)) wave.trigger();
    return;
  }

  //NR40
  if(address == 0xff1f && cycle == 2) {
    return;
  }

  //NR41
  if(address == 0xff20 && cycle == 2) {
    noise.length = 64 - data.bit(0,5);
    return;
  }

  //NR42
  if(address == 0xff21 && cycle == 2) {
    noise.envelopeFrequency = data.bit(0,2);
    noise.envelopeDirection = data.bit(3);
    noise.envelopeVolume    = data.bit(4,7);
    if(!noise.dacEnable()) noise.enable = 0;
    return;
  }

  //NR43
  if(address == 0xff22 && cycle == 2) {
    noise.divisor   = data.bit(0,2);
    noise.narrow    = data.bit(3);
    noise.frequency = data.bit(4,7);
    noise.period    = noise.getPeriod();
    return;
  }

  //NR44
  if(address == 0xff23 && cycle == 4) {
    if(phase.bit(0) && !noise.counter && data.bit(6)) {
      if(noise.length && --noise.length == 0) noise.enable = 0;
    }
    noise.counter = data.bit(6);
    if(data.bit(7)) noise.trigger();
    return;
  }

  //NR50
  if(address == 0xff24 && cycle == 2) {
    sequencer.rightVolume = data.bit(0,2);
    sequencer.rightEnable = data.bit(3);
    sequencer.leftVolume  = data.bit(4,6);
    sequencer.leftEnable  = data.bit(7);
    return;
  }

  //NR51
  if(address == 0xff25 && cycle == 2) {
    sequencer.square1.rightEnable = data.bit(0);
    sequencer.square2.rightEnable = data.bit(1);
    sequencer.wave.rightEnable    = data.bit(2);
    sequencer.noise.rightEnable   = data.bit(3);
    sequencer.square1.leftEnable  = data.bit(4);
    sequencer.square2.leftEnable  = data.bit(5);
    sequencer.wave.leftEnable     = data.bit(6);
    sequencer.noise.leftEnable    = data.bit(7);
    return;
  }

  //NR52
  if(address == 0xff26 && cycle == 4) {
    if(sequencer.enable != data.bit(7)) {
      sequencer.enable = data.bit(7);
      if(!sequencer.enable) {
        bool resetLengthCounters = Model::GameBoyColor();
        square1.power(resetLengthCounters);
        square2.power(resetLengthCounters);
        wave.power(resetLengthCounters);
        noise.power(resetLengthCounters);
        sequencer.power();
      } else {
        phase = 0;
      }
    }
    return;
  }

  if(address >= 0xff30 && address <= 0xff3f && cycle == 2) {
    return wave.writeRAM(address, data);
  }
}
