auto CPU::serialize(serializer& s) -> void {
  HuC6280::serialize(s);
  Thread::serialize(s);

  ram.serialize(s);

  s.integer(irq2.disable);
  s.integer(irq2.pending);

  s.integer(irq1.disable);
  s.integer(irq1.pending);

  s.integer(tiq.disable);
  s.integer(tiq.pending);

  s.integer(timer.line);
  s.integer(timer.enable);
  s.integer(timer.reload);
  s.integer(timer.value);
  s.integer(timer.counter);

  s.integer(io.buffer);
}
