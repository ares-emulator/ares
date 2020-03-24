auto T6W28::serialize(serializer& s) -> void {
  tone0.serialize(s);
  tone1.serialize(s);
  tone2.serialize(s);
  noise.serialize(s);
  s.integer(io.register);
}

auto T6W28::Tone::serialize(serializer& s) -> void {
  s.integer(counter);
  s.integer(pitch);
  s.integer(output);
  s.integer(volume.left);
  s.integer(volume.right);
}

auto T6W28::Noise::serialize(serializer& s) -> void {
  s.integer(counter);
  s.integer(pitch);
  s.integer(enable);
  s.integer(rate);
  s.integer(lfsr);
  s.integer(flip);
  s.integer(output);
  s.integer(volume.left);
  s.integer(volume.right);
}
