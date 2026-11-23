auto MBM29F016A::serialize(serializer& s) -> void {
  s(state);
  for(auto& byte : data) s(byte);
}
