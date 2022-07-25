auto YM2610::serialize(serializer& s) -> void {
  s(registerAddress);
  s(fm);
  s(ssg);
  s(pcmA);
}

auto YM2610::PCMA::serialize(serializer& s) -> void {

}