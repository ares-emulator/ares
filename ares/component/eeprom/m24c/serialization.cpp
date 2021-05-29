auto M24C::serialize(serializer& s) -> void {
  if(type == Type::None) return;
  s((u32&)type);
  s((u32&)mode);
  s(clock.latch);
  s(clock.value);
  s(data.latch);
  s(data.value);
  s(enable);
  s(counter);
  s(device);
  s(bank);
  s(address);
  s(input);
  s(output);
  s(response);
  s(writable);
  s(array_span<u8>{memory, size()});
  if(type >= Type::M24C32) s(idpage);
  s(locked);
}
