auto CPU::serialize(serializer& s) -> void {
  Z80::serialize(s);
  Z80::Bus::serialize(s);
  Thread::serialize(s);
  s(ram);
  s(mdr);
  s(state.nmiLine);
  s(state.irqLine);
}
