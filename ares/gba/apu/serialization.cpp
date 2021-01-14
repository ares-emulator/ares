auto APU::serialize(serializer& s) -> void {
  Thread::serialize(s);

  s(clock);

  s(bias.level);
  s(bias.amplitude);

  s(square1.sweep.shift);
  s(square1.sweep.direction);
  s(square1.sweep.frequency);
  s(square1.sweep.enable);
  s(square1.sweep.negate);
  s(square1.sweep.period);

  s(square1.envelope.frequency);
  s(square1.envelope.direction);
  s(square1.envelope.volume);
  s(square1.envelope.period);

  s(square1.enable);
  s(square1.length);
  s(square1.duty);
  s(square1.frequency);
  s(square1.counter);
  s(square1.initialize);
  s(square1.shadowfrequency);
  s(square1.signal);
  s(square1.output);
  s(square1.period);
  s(square1.phase);
  s(square1.volume);

  s(square2.envelope.frequency);
  s(square2.envelope.direction);
  s(square2.envelope.volume);
  s(square2.envelope.period);

  s(square2.enable);
  s(square2.length);
  s(square2.duty);
  s(square2.frequency);
  s(square2.counter);
  s(square2.initialize);
  s(square2.shadowfrequency);
  s(square2.signal);
  s(square2.output);
  s(square2.period);
  s(square2.phase);
  s(square2.volume);

  s(wave.mode);
  s(wave.bank);
  s(wave.dacenable);
  s(wave.length);
  s(wave.volume);
  s(wave.frequency);
  s(wave.counter);
  s(wave.initialize);
  s(wave.pattern);
  s(wave.enable);
  s(wave.output);
  s(wave.patternaddr);
  s(wave.patternbank);
  s(wave.patternsample);
  s(wave.period);

  s(noise.envelope.frequency);
  s(noise.envelope.direction);
  s(noise.envelope.volume);
  s(noise.envelope.period);

  s(noise.length);
  s(noise.divisor);
  s(noise.narrowlfsr);
  s(noise.frequency);
  s(noise.counter);
  s(noise.initialize);
  s(noise.enable);
  s(noise.lfsr);
  s(noise.output);
  s(noise.period);
  s(noise.volume);

  s(sequencer.volume);
  s(sequencer.lvolume);
  s(sequencer.rvolume);
  s(sequencer.lenable);
  s(sequencer.renable);
  s(sequencer.masterenable);
  s(sequencer.base);
  s(sequencer.step);
  s(sequencer.lsample);
  s(sequencer.rsample);
  s(sequencer.loutput);
  s(sequencer.routput);

  for(auto& f : fifo) {
    s(f.samples);
    s(f.active);
    s(f.output);
    s(f.rdoffset);
    s(f.wroffset);
    s(f.size);
    s(f.volume);
    s(f.lenable);
    s(f.renable);
    s(f.timer);
  }
}
