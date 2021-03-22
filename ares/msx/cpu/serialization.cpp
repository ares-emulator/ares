auto CPU::serialize(serializer& s) -> void {
  Z80::serialize(s);
  Thread::serialize(s);

  s(ram);
  for(auto& slot : this->slot) {
    s(slot.memory);
    s(slot.primary);
    s(slot.secondary);
  }
  s(io.irqLine);
}
