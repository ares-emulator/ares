auto YM2413::serialize(serializer& s) -> void {
//melodicTones and rhythmTones are not serialized here:
//they are reloaded during YM2413::power()
  s.array(customTone);
  for(auto& voice : voices) voice.serialize(s);
  s.integer(io.clock);
  s.integer(io.address);
  s.integer(io.rhythmMode);
  s.integer(io.noise);
  s.integer(io.isVRC7);
}

auto YM2413::Voice::serialize(serializer& s) -> void {
  s.integer(tone);
  s.integer(fnumber);
  s.integer(block);
  s.integer(level);
  s.integer(feedback);
  modulator.serialize(s);
  carrier.serialize(s);
}

auto YM2413::Operator::serialize(serializer& s) -> void {
  s.integer(slot);
  s.integer(keyOn);
  s.integer(sustainOn);
  s.integer(multiple);
  s.integer(scaleRate);
  s.integer(sustainable);
  s.integer(vibrato);
  s.integer(tremolo);
  s.integer(scaleLevel);
  s.integer(waveform);
  s.integer(attack);
  s.integer(decay);
  s.integer(sustain);
  s.integer(release);
  s.integer(totalLevel);
  s.integer(audible);

  s.integer(state);
  s.integer(rate);
  s.integer(divider);
  s.integer(sequence);
  s.integer(envelope);
  s.integer(level);

  s.integer(fnumber);
  s.integer(block);
  s.array(pitch);
  s.integer(phase);

  s.integer(output);
  s.integer(prior);
}
