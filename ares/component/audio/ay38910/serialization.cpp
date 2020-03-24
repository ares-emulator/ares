auto AY38910::serialize(serializer& s) -> void {
  s.integer(toneA.counter);
  s.integer(toneA.period);
  s.integer(toneA.output);

  s.integer(toneB.counter);
  s.integer(toneB.period);
  s.integer(toneB.output);

  s.integer(toneC.counter);
  s.integer(toneC.period);
  s.integer(toneC.output);

  s.integer(noise.counter);
  s.integer(noise.period);
  s.integer(noise.flip);
  s.integer(noise.lfsr);
  s.integer(noise.output);

  s.integer(envelope.counter);
  s.integer(envelope.period);
  s.integer(envelope.holding);
  s.integer(envelope.attacking);
  s.integer(envelope.hold);
  s.integer(envelope.alternate);
  s.integer(envelope.attack);
  s.integer(envelope.repeat);
  s.integer(envelope.output);

  s.integer(channelA.tone);
  s.integer(channelA.noise);
  s.integer(channelA.envelope);
  s.integer(channelA.volume);

  s.integer(channelB.tone);
  s.integer(channelB.noise);
  s.integer(channelB.envelope);
  s.integer(channelB.volume);

  s.integer(channelC.tone);
  s.integer(channelC.noise);
  s.integer(channelC.envelope);
  s.integer(channelC.volume);

  s.integer(portA.direction);
  s.integer(portA.data);

  s.integer(portB.direction);
  s.integer(portB.data);

  s.integer(io.register);
}
