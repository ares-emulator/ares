auto SN76489::serialize(serializer& s) -> void {
  tone0.serialize(s);
  tone1.serialize(s);
  tone2.serialize(s);
  noise.serialize(s);
  s.integer(io.register);
}

auto SN76489::Tone::serialize(serializer& s) -> void {
  s.integer(volume);
  s.integer(counter);
  s.integer(pitch);
  s.integer(output);
}

auto SN76489::Noise::serialize(serializer& s) -> void {
  s.integer(volume);
  s.integer(counter);
  s.integer(pitch);
  s.integer(enable);
  s.integer(rate);
  s.integer(lfsr);
  s.integer(flip);
  s.integer(output);
}
