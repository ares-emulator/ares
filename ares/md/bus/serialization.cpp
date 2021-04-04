auto Bus::serialize(serializer& s) -> void {
  s(state.acquired);
}
