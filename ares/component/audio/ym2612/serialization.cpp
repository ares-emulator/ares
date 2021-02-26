auto YM2612::serialize(serializer& s) -> void {
  s(io.address);

  s(lfo.enable);
  s(lfo.rate);
  s(lfo.clock);
  s(lfo.divider);

  s(dac.enable);
  s(dac.sample);

  s(envelope.clock);
  s(envelope.divider);

  s(timerA.enable);
  s(timerA.irq);
  s(timerA.line);
  s(timerA.period);
  s(timerA.counter);

  s(timerB.enable);
  s(timerB.irq);
  s(timerB.line);
  s(timerB.period);
  s(timerB.counter);
  s(timerB.divider);

  for(u32 n : range(6)) s(channels[n]);
}

auto YM2612::Channel::serialize(serializer& s) -> void {
  s(leftEnable);
  s(rightEnable);
  s(algorithm);
  s(feedback);
  s(vibrato);
  s(tremolo);
  s(mode);

  for(u32 n : range(4)) s(operators[n]);
}

auto YM2612::Channel::Operator::serialize(serializer& s) -> void {
  s(keyOn);
  s(lfoEnable);
  s(detune);
  s(multiple);
  s(totalLevel);
  s(outputLevel);
  s(output);
  s(prior);

  s(pitch.value);
  s(pitch.reload);
  s(pitch.latch);

  s(octave.value);
  s(octave.reload);
  s(octave.latch);

  s(phase.value);
  s(phase.delta);

  s(envelope.state);
  s(envelope.rate);
  s(envelope.divider);
  s(envelope.steps);
  s(envelope.value);
  s(envelope.keyScale);
  s(envelope.attackRate);
  s(envelope.decayRate);
  s(envelope.sustainRate);
  s(envelope.sustainLevel);
  s(envelope.releaseRate);

  s(ssg.enable);
  s(ssg.attack);
  s(ssg.alternate);
  s(ssg.hold);
  s(ssg.invert);
}
