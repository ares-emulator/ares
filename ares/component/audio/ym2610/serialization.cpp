auto YM2610::serialize(serializer& s) -> void {
  s(registerAddress);
  s(fm);
  s(ssg);
}
