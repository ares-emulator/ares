auto APU::serialize(serializer& s) -> void {
  Z80::serialize(s);
  Z80::Bus::serialize(s);
  Thread::serialize(s);

  ram.serialize(s);
  s.integer(nmi.line);
  s.integer(irq.line);
  s.integer(port.data);
  s.integer(io.enable);
}
