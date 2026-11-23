auto LH28F400::serialize(serializer& s) -> void {
  s(state);
  for(auto& byte : data) s(byte);
}
