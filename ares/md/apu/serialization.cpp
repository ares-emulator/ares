auto APU::serialize(serializer& s) -> void {
  Z80::serialize(s);
  Thread::serialize(s);
  s(ram);
  s(state.nmiLine);
  s(state.intLine);
  s(state.resLine);
  s(state.busreqLine);
  s(state.busreqLatch);
  s(state.bank);
}
