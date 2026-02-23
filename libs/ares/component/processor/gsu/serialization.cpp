auto GSU::serialize(serializer& s) -> void {
  s(regs.pipeline);
  s(regs.ramaddr);

  for(u32 n : range(16)) {
    s(regs.r[n].data);
    s(regs.r[n].modified);
  }

  s(regs.sfr.data);
  s(regs.pbr);
  s(regs.rombr);
  s(regs.rambr);
  s(regs.cbr);
  s(regs.scbr);

  s(regs.scmr.ht);
  s(regs.scmr.ron);
  s(regs.scmr.ran);
  s(regs.scmr.md);

  s(regs.colr);

  s(regs.por.obj);
  s(regs.por.freezehigh);
  s(regs.por.highnibble);
  s(regs.por.dither);
  s(regs.por.transparent);

  s(regs.bramr);
  s(regs.vcr);

  s(regs.cfgr.irq);
  s(regs.cfgr.ms0);

  s(regs.clsr);

  s(regs.romcl);
  s(regs.romdr);

  s(regs.ramcl);
  s(regs.ramar);
  s(regs.ramdr);

  s(regs.sreg);
  s(regs.dreg);

  s(cache.buffer);
  s(cache.valid);

  for(u32 n : range(2)) {
    s(pixelcache[n].offset);
    s(pixelcache[n].bitpend);
    s(pixelcache[n].data);
  }
}
