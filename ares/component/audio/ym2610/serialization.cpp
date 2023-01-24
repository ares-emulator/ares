auto YM2610::serialize(serializer& s) -> void {
  s(registerAddress);
  s(fm);
  s(ssg);
  s(pcmA);
  s(pcmB);
}

auto YM2610::PCMA::serialize(serializer& s) -> void {
  for(auto n : range(6)) {
   s(channels[n].playing);
   s(channels[n].ended);
   s(channels[n].endedMask);
   s(channels[n].left);
   s(channels[n].right);
   s(channels[n].volume);
   s(channels[n].startAddress);
   s(channels[n].endAddress);
   s(channels[n].currentAddress);
   s(channels[n].decodeAccumulator);
   s(channels[n].decodeStep);
   s(channels[n].currentNibble);
  }
}

auto YM2610::PCMB::serialize(serializer& s) -> void {
  s(playing);
  s(ended);
  s(endedMask);
  s(repeat);
  s(left);
  s(right);
  s(startAddress);
  s(endAddress);
  s(delta);
  s(volume);

  s(currentAddress);
  s(currentNibble);
  s(decodeAccumulator);
  s(decodePosition);
  s(previousAccumulator);
  s(decodeStep);
}