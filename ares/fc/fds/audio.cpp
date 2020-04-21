auto FDSAudio::Operator::clockEnvelope() -> bool {
  if(envelope && masterSpeed > 0 && !--period) {
    reloadPeriod();
    if(direction && gain < 32) {
      gain++;
    } else if(!direction && gain > 0) {
      gain--;
    }
    return true;
  }
  return false;
}

auto FDSAudio::Operator::reloadPeriod() -> void {
  period = 8 * (1 + speed) * masterSpeed;
}

//

auto FDSAudio::Modulator::enabled() -> bool {
  return !disabled && frequency > 0;
}

auto FDSAudio::Modulator::clockModulator() -> bool {
  if(enabled()) {
    overflow += frequency;
    if(overflow < frequency) {  //overflow occurred
      static const integer lookup[8] = {0, 1, 2, 4, -8, -4, -2, -1};
      integer offset = lookup[table.data[table.index++]];
      updateCounter(offset == -8 ? 0 : counter + offset);
      return true;
    }
  }
  return false;
}

auto FDSAudio::Modulator::updateOutput(uint16 pitch) -> void {
  output = counter * gain;
  integer remainder = output.bit(0,3);
  output >>= 4;
  if(remainder > 0 && !output.bit(7)) output += counter < 0 ? -1 : 2;

  if(output >= 192) {
    output -= 256;
  } else if(output < -64) {
    output += 256;
  }

  output *= pitch;
  remainder = output.bit(0,5);
  output >>= 6;
  if(remainder >= 32) output++;
}

auto FDSAudio::Modulator::updateCounter(int8 value) -> void {
  counter = value;
  if(counter >= 64) {
    counter -= 128;
  } else if(counter < -64) {
    counter += 128;
  }
}

//

auto FDSAudio::load(Node::Object parent) -> void {
  stream = parent->append<Node::Stream>("FDS");
  stream->setChannels(1);
  stream->setFrequency(uint(system.frequency() + 0.5) / cpu.rate());
}

auto FDSAudio::unload() -> void {
  stream = {};
}

auto FDSAudio::clock() -> void {
  if(!enable) return stream->sample(0.0);

  int frequency = carrier.frequency;
  if(envelopes && !waveform.halt) {
    carrier.clockEnvelope();
    if(modulator.clockEnvelope()) {
      modulator.updateOutput(frequency);
    }
  }

  if(modulator.clockModulator()) {
    modulator.updateOutput(frequency);
  }

  if(waveform.halt) {
    waveform.index = 0;
    updateOutput();
  } else {
    updateOutput();
    if(frequency + modulator.output > 0 && !waveform.writable) {
      waveform.overflow += frequency + modulator.output;
      if(waveform.overflow < frequency + modulator.output) {
        waveform.index++;
      }
    }
  }
}

auto FDSAudio::updateOutput() -> void {
  static const uint lookup[4] = {36, 24, 17, 14};
  integer level = min(carrier.gain, 32) * lookup[masterVolume];

  uint8 output = waveform.data[waveform.index] * level / 1152;
  stream->sample(output / 255.0 * 0.5);
}

auto FDSAudio::read(uint16 address, uint8 data) -> uint8 {
  if(!enable) return data;

  if(address >= 0x4040 && address <= 0x407f) {
    data.bit(0,5) = waveform.data[(uint6)address];
    return data;
  }

  switch(address) {

  case 0x4090:
    data.bit(0,5) = carrier.gain;
    return data;

  case 0x4092:
    data.bit(0,5) = modulator.gain;
    return data;

  }

  return data;
}

auto FDSAudio::write(uint16 address, uint8 data) -> void {
  if(!enable && address != 0x4025) return;

  if(address >= 0x4040 && address <= 0x407f && waveform.writable) {
    waveform.data[(uint6)address] = data.bit(0,5);
    return;
  }

  switch(address) {

  case 0x4025:
    enable = data.bit(1);
    return;

  case 0x4080:
    carrier.speed = data.bit(0,5);
    carrier.direction = data.bit(6);
    carrier.envelope = !data.bit(7);
    if(!carrier.envelope) carrier.gain = carrier.speed;
    carrier.reloadPeriod();
    return;

  case 0x4082:
    carrier.frequency.bit(0,7) = data.bit(0,7);
    return;

  case 0x4083:
    carrier.frequency.bit(8,11) = data.bit(0,3);
    envelopes = !data.bit(6);
    waveform.halt = data.bit(7);
    if(!envelopes) {
      carrier.reloadPeriod();
      modulator.reloadPeriod();
    }
    return;

  case 0x4084:
    modulator.speed = data.bit(0,5);
    modulator.direction = data.bit(6);
    modulator.envelope = !data.bit(7);
    if(!modulator.envelope) modulator.gain = modulator.speed;
    modulator.reloadPeriod();
    return;

  case 0x4085:
    modulator.updateCounter(data.bit(0,6));
    return;

  case 0x4086:
    modulator.frequency.bit(0,7) = data.bit(0,7);
    return;

  case 0x4087:
    modulator.frequency.bit(8,11) = data.bit(0,3);
    modulator.disabled = data.bit(7);
    if(modulator.disabled) modulator.overflow = 0;
    modulator.reloadPeriod();
    return;

  case 0x4088:
    if(!modulator.disabled) return;
    modulator.table.data[modulator.table.index++] = data.bit(0,2);
    modulator.table.data[modulator.table.index++] = data.bit(0,2);
    return;

  case 0x4089:
    masterVolume = data.bit(0,1);
    waveform.writable = data.bit(7);
    return;

  case 0x408a:
    carrier.masterSpeed = data;
    modulator.masterSpeed = data;
    return;

  }
}

auto FDSAudio::power() -> void {
  enable = 0;
  envelopes = 0;
  masterVolume = 0;
  carrier = {};
  modulator = {};
  waveform = {};
}
