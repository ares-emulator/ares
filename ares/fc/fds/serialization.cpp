auto FDS::serialize(serializer& s) -> void {
  drive.serialize(s);
  timer.serialize(s);
  audio.serialize(s);
}

auto FDSDrive::serialize(serializer& s) -> void {
  s.integer(enable);
  s.integer(power);
  s.integer(changing);
  s.integer(ready);
  s.integer(scan);
  s.integer(rewinding);
  s.integer(scanning);
  s.integer(reading);
  s.integer(writeCRC);
  s.integer(clearCRC);
  s.integer(irq);
  s.integer(pending);
  s.integer(available);
  s.integer(counter);
  s.integer(offset);
  s.integer(gap);
  s.integer(data);
  s.integer(completed);
  s.integer(crc16);
}

auto FDSTimer::serialize(serializer& s) -> void {
  s.integer(enable);
  s.integer(counter);
  s.integer(period);
  s.integer(repeat);
  s.integer(irq);
  s.integer(pending);
}

auto FDSAudio::serialize(serializer& s) -> void {
  s.integer(enable);
  s.integer(envelopes);
  s.integer(masterVolume);

  s.integer(carrier.masterSpeed);
  s.integer(carrier.speed);
  s.integer(carrier.gain);
  s.integer(carrier.direction);
  s.integer(carrier.envelope);
  s.integer(carrier.frequency);
  s.integer(carrier.period);

  s.integer(modulator.masterSpeed);
  s.integer(modulator.speed);
  s.integer(modulator.gain);
  s.integer(modulator.direction);
  s.integer(modulator.envelope);
  s.integer(modulator.frequency);
  s.integer(modulator.period);

  s.integer(modulator.disabled);
  s.integer(modulator.counter);
  s.integer(modulator.overflow);
  s.integer(modulator.output);
  s.array(modulator.table.data);
  s.integer(modulator.table.index);

  s.integer(waveform.halt);
  s.integer(waveform.writable);
  s.integer(waveform.overflow);
  s.array(waveform.data);
  s.integer(waveform.index);
}
