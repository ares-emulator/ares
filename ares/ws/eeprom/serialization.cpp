auto EEPROM::serialize(serializer& s) -> void {
  M93LCx6::serialize(s);
  s(io.read);
  s(io.write);
  s(io.command);
  s(io.ready);
  s(io.readComplete);
  s(io.control);
  s(io.protect);
}
