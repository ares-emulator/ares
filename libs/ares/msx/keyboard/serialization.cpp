auto Keyboard::serialize(serializer& s) -> void {
  s(io.select);
}
