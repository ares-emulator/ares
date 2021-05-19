auto YM2610::serialize(serializer& s) -> void {
  s(register);
  s(fm);
  s(ssg);
}
