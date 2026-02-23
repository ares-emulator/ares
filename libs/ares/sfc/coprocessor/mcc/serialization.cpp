auto MCC::serialize(serializer& s) -> void {
  s(psram);

  s(irq.flag);
  s(irq.enable);

  s(r.mapping);
  s(r.psramEnableLo);
  s(r.psramEnableHi);
  s(r.psramMapping);
  s(r.romEnableLo);
  s(r.romEnableHi);
  s(r.exEnableLo);
  s(r.exEnableHi);
  s(r.exMapping);
  s(r.internallyWritable);
  s(r.externallyWritable);

  s(w.mapping);
  s(w.psramEnableLo);
  s(w.psramEnableHi);
  s(w.psramMapping);
  s(w.romEnableLo);
  s(w.romEnableHi);
  s(w.exEnableLo);
  s(w.exEnableHi);
  s(w.exMapping);
  s(w.internallyWritable);
  s(w.externallyWritable);
}
