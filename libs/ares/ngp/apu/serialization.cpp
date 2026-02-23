auto APU::serialize(serializer& s) -> void {
  Z80::serialize(s);
  Thread::serialize(s);
  s(ram);
  s(nmi.line);
  s(irq.line);
  s(port.data);
  s(io.enable);
}
