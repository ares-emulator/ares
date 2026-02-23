auto M93LCx6::serialize(serializer& s) -> void {
  s(data);
  s(size);
  s(width);
  s(endian);
  s(writable);
  s(busy);
  s(input);
  s(output);
}

auto M93LCx6::InputShiftRegister::serialize(serializer& s) -> void {
  s(value);
  s(count);
  s(addressLength);
  s(dataLength);
}

auto M93LCx6::OutputShiftRegister::serialize(serializer& s) -> void {
  s(value);
  s(count);
}
