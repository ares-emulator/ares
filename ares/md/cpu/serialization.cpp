auto CPU::serialize(serializer& s) -> void {
  M68K::serialize(s);
  Thread::serialize(s);

  ram.serialize(s);

  s.boolean(io.version);
  s.boolean(io.romEnable);
  s.boolean(io.vdpEnable[0]);
  s.boolean(io.vdpEnable[1]);

  s.integer(refresh.ram);
  s.integer(refresh.external);

  s.integer(state.interruptLine);
  s.integer(state.interruptPending);
}
