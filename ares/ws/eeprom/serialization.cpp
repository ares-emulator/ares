auto EEPROM::serialize(serializer& s) -> void {
  M93LCx6::serialize(s);

  s.integer(r.data);
  s.integer(r.address);
  s.integer(r.readReady);
  s.integer(r.writeReady);
  s.integer(r.eraseReady);
  s.integer(r.resetReady);
  s.integer(r.readPending);
  s.integer(r.writePending);
  s.integer(r.erasePending);
  s.integer(r.resetPending);
}
