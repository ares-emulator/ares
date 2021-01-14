auto AY38910::serialize(serializer& s) -> void {
  s(toneA.counter);
  s(toneA.period);
  s(toneA.output);

  s(toneB.counter);
  s(toneB.period);
  s(toneB.output);

  s(toneC.counter);
  s(toneC.period);
  s(toneC.output);

  s(noise.counter);
  s(noise.period);
  s(noise.flip);
  s(noise.lfsr);
  s(noise.output);

  s(envelope.counter);
  s(envelope.period);
  s(envelope.holding);
  s(envelope.attacking);
  s(envelope.hold);
  s(envelope.alternate);
  s(envelope.attack);
  s(envelope.repeat);
  s(envelope.output);

  s(channelA.tone);
  s(channelA.noise);
  s(channelA.envelope);
  s(channelA.volume);

  s(channelB.tone);
  s(channelB.noise);
  s(channelB.envelope);
  s(channelB.volume);

  s(channelC.tone);
  s(channelC.noise);
  s(channelC.envelope);
  s(channelC.volume);

  s(portA.direction);
  s(portA.data);

  s(portB.direction);
  s(portB.data);

  s(io.register);
}
