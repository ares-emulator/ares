auto EEPROM::serialize(serializer& s) -> void {
  M93LCx6::serialize(s);
  s(r.data);
  s(r.address);
  s(r.readReady);
  s(r.writeReady);
  s(r.eraseReady);
  s(r.resetReady);
  s(r.readPending);
  s(r.writePending);
  s(r.erasePending);
  s(r.resetPending);
}
