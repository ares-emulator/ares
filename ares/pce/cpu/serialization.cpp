auto CPU::serialize(serializer& s) -> void {
  HuC6280::serialize(s);
  Thread::serialize(s);
  s(ram);
  s(irq2.disable);
  s(irq2.pending);
  s(irq1.disable);
  s(irq1.pending);
  s(tiq.disable);
  s(tiq.pending);
  s(timer.line);
  s(timer.enable);
  s(timer.reload);
  s(timer.value);
  s(timer.counter);
  s(io.buffer);
}
