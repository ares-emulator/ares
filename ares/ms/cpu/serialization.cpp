auto CPU::serialize(serializer& s) -> void {
  Z80::serialize(s);
  Thread::serialize(s);
  s(ram);
  s(mdr);
  s(state.nmiLine);
  s(state.irqLine);
  s(bus.ioEnable);
  s(bus.biosEnable);
  s(bus.ramEnable);
  s(bus.cardEnable);
  s(bus.cartridgeEnable);
  s(bus.expansionEnable);
}
