auto YM2413::serialize(serializer& s) -> void {
//melodicTones and rhythmTones are not serialized here:
//they are reloaded during YM2413::power()
  s(customTone);
  for(auto& voice : voices) s(voice);
  s(io.clock);
  s(io.address);
  s(io.rhythmMode);
  s(io.noise);
  s(io.isVRC7);
}

auto YM2413::Voice::serialize(serializer& s) -> void {
  s(tone);
  s(fnumber);
  s(block);
  s(level);
  s(feedback);
  s(modulator);
  s(carrier);
}

auto YM2413::Operator::serialize(serializer& s) -> void {
  s(slot);
  s(keyOn);
  s(sustainOn);
  s(multiple);
  s(scaleRate);
  s(sustainable);
  s(vibrato);
  s(tremolo);
  s(scaleLevel);
  s(waveform);
  s(attack);
  s(decay);
  s(sustain);
  s(release);
  s(totalLevel);
  s(audible);
  s(state);
  s(rate);
  s(divider);
  s(sequence);
  s(envelope);
  s(level);
  s(fnumber);
  s(block);
  s(pitch);
  s(phase);
  s(output);
  s(prior);
}
