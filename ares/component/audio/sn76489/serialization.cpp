auto SN76489::serialize(serializer& s) -> void {
  s(tone0);
  s(tone1);
  s(tone2);
  s(noise);
  s(io.register);
}

auto SN76489::Tone::serialize(serializer& s) -> void {
  s(volume);
  s(counter);
  s(pitch);
  s(output);
}

auto SN76489::Noise::serialize(serializer& s) -> void {
  s(volume);
  s(counter);
  s(pitch);
  s(enable);
  s(rate);
  s(lfsr);
  s(flip);
  s(output);
}
