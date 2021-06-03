auto CPU::serialize(serializer& s) -> void {
  M68000::serialize(s);
  Thread::serialize(s);
  s(ram);
  s(io.version);
  s(io.romEnable);
  s(io.vdpEnable);
  s(refresh.ram);
  s(refresh.external);
  s(state.interruptPending);
}
