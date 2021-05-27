auto M24Cxxx::serialize(serializer& s) -> void {
  s((u32&)type);
  s((u32&)mode);
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
  s(enable);
  s(counter);
  s(device);
  s(address);
  s(input);
  s(output);
  s(response);
  s(writable);
  s(locked);
  s(array_span<u8>{memory, size()});
  s(idpage);
}
