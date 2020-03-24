auto M93LCx6::serialize(serializer& s) -> void {
  s.array(data);
  s.integer(size);
  s.integer(width);
  s.boolean(writable);
  s.integer(busy);
  input.serialize(s);
  output.serialize(s);
}

auto M93LCx6::InputShiftRegister::serialize(serializer& s) -> void {
  s.integer(value);
  s.integer(count);
  s.integer(addressLength);
  s.integer(dataLength);
}

auto M93LCx6::OutputShiftRegister::serialize(serializer& s) -> void {
  s.integer(value);
  s.integer(count);
}
