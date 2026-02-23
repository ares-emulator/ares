auto FDS::serialize(serializer& s) -> void {
  s(drive);
  s(timer);
  s(audio);
}

auto FDSDrive::serialize(serializer& s) -> void {
  s(enable);
  s(power);
  s(changing);
  s(ready);
  s(scan);
  s(rewinding);
  s(scanning);
  s(reading);
  s(writeCRC);
  s(clearCRC);
  s(irq);
  s(pending);
  s(available);
  s(counter);
  s(offset);
  s(gap);
  s(data);
  s(completed);
  s(crc16);
}

auto FDSTimer::serialize(serializer& s) -> void {
  s(enable);
  s(counter);
  s(period);
  s(repeat);
  s(irq);
  s(pending);
}

auto FDSAudio::serialize(serializer& s) -> void {
  s(enable);
  s(envelopes);
  s(masterVolume);
  s(carrier.masterSpeed);
  s(carrier.speed);
  s(carrier.gain);
  s(carrier.direction);
  s(carrier.envelope);
  s(carrier.frequency);
  s(carrier.period);
  s(modulator.masterSpeed);
  s(modulator.speed);
  s(modulator.gain);
  s(modulator.direction);
  s(modulator.envelope);
  s(modulator.frequency);
  s(modulator.period);
  s(modulator.disabled);
  s(modulator.counter);
  s(modulator.overflow);
  s(modulator.output);
  s(modulator.table.data);
  s(modulator.table.index);
  s(waveform.halt);
  s(waveform.writable);
  s(waveform.overflow);
  s(waveform.data);
  s(waveform.index);
}
