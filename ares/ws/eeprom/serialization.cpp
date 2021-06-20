auto EEPROM::serialize(serializer& s) -> void {
  M93LCx6::serialize(s);
  s(io.data);
  s(io.address);
  s(io.readReady);
  s(io.writeReady);
  s(io.eraseReady);
  s(io.resetReady);
  s(io.readPending);
  s(io.writePending);
  s(io.erasePending);
  s(io.resetPending);
}
