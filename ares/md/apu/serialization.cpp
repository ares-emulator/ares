auto APU::serialize(serializer& s) -> void {
  Z80::serialize(s);
  Z80::Bus::serialize(s);
  Thread::serialize(s);
  s(ram);
  s(io.bank);
  s(state.enabled);
  s(state.nmiLine);
  s(state.intLine);
}
