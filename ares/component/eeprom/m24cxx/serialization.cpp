auto M24Cxx::serialize(serializer& s) -> void {
  s((u32&)type);
  s(array_span<u8>{bytes, size()});
  s(clock.lo);
  s(clock.hi);
  s(clock.fall);
  s(clock.rise);
  s(clock.line);
  s(data.lo);
  s(data.hi);
  s(data.fall);
  s(data.rise);
  s(data.line);
  s((u32&)mode);
  s(counter);
  s(control);
  s(address);
  s(input);
  s(output);
  s(line);
}
