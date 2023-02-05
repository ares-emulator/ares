
auto CIC::serialize(serializer& s) -> void {
  s(seed);
  s(version);
  s(type);
}
