auto APU::serialize(serializer& s) -> void {
  Z80::serialize(s);
  Z80::Bus::serialize(s);
  Thread::serialize(s);

  ram.serialize(s);

  s.integer(io.bank);

  s.integer(state.enabled);
  s.integer(state.nmiLine);
  s.integer(state.intLine);
}
