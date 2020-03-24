auto Keyboard::serialize(serializer& s) -> void {
  s.integer(io.select);
}
