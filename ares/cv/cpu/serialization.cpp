auto CPU::serialize(serializer& s) -> void {
  Z80::serialize(s);
  Z80::Bus::serialize(s);
  Thread::serialize(s);

  s.array(ram.data(), ram.size());
  s.array(expansion.data(), expansion.size());

  s.integer(state.nmiLine);
  s.integer(state.irqLine);

  s.integer(io.replaceBIOS);
  s.integer(io.replaceRAM);
}
