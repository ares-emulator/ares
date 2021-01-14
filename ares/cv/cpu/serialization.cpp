auto CPU::serialize(serializer& s) -> void {
  Z80::serialize(s);
  Z80::Bus::serialize(s);
  Thread::serialize(s);
  s(ram);
  s(expansion);
  s(state.nmiLine);
  s(state.irqLine);
  s(io.replaceBIOS);
  s(io.replaceRAM);
}
