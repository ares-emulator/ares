auto APU::serialize(serializer& s) -> void {
  Z80::serialize(s);
  Thread::serialize(s);
  s(ram);
  s(communication.input);
  s(communication.output);
  s(nmi.pending);
  s(nmi.enable);
  s(irq.pending);
  s(rom.bankA);
  s(rom.bankB);
  s(rom.bankC);
  s(rom.bankD);
}
