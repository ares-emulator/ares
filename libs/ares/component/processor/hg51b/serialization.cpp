auto HG51B::serialize(serializer& s) -> void {
  s(programRAM[0]);
  s(programRAM[1]);
  s(dataRAM);

  s(r.pb);
  s(r.pc);

  s(r.n);
  s(r.z);
  s(r.c);
  s(r.v);
  s(r.i);

  s(r.a);
  s(r.p);
  s(r.mul);
  s(r.mdr);
  s(r.rom);
  s(r.ram);
  s(r.mar);
  s(r.dpr);
  s(r.gpr);

  s(io.lock);
  s(io.halt);
  s(io.irq);
  s(io.rom);
  s(io.vector);

  s(io.wait.rom);
  s(io.wait.ram);

  s(io.suspend.enable);
  s(io.suspend.duration);

  s(io.cache.enable);
  s(io.cache.page);
  s(io.cache.lock);
  s(io.cache.address);
  s(io.cache.base);
  s(io.cache.pb);
  s(io.cache.pc);

  s(io.dma.enable);
  s(io.dma.source);
  s(io.dma.target);
  s(io.dma.length);

  s(io.bus.enable);
  s(io.bus.reading);
  s(io.bus.writing);
  s(io.bus.pending);
  s(io.bus.address);

  s(stack);
}
