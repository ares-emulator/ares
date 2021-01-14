auto T6W28::serialize(serializer& s) -> void {
  s(tone0);
  s(tone1);
  s(tone2);
  s(noise);
  s(io.register);
}

auto T6W28::Tone::serialize(serializer& s) -> void {
  s(counter);
  s(pitch);
  s(output);
  s(volume.left);
  s(volume.right);
}

auto T6W28::Noise::serialize(serializer& s) -> void {
  s(counter);
  s(pitch);
  s(enable);
  s(rate);
  s(lfsr);
  s(flip);
  s(output);
  s(volume.left);
  s(volume.right);
}
