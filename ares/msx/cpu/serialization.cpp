auto CPU::serialize(serializer& s) -> void {
  Z80::serialize(s);
  Z80::Bus::serialize(s);
  Thread::serialize(s);

  ram.serialize(s);
  for(auto& slot : this->slot) {
    s.integer(slot.memory);
    s.integer(slot.primary);
    s.array(slot.secondary);
  }
  s.integer(io.irqLine);
}
