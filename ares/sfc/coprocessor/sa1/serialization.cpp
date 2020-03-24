auto SA1::serialize(serializer& s) -> void {
  WDC65816::serialize(s);
  Thread::serialize(s);

  s.array(iram.data(), iram.size());
  s.array(bwram.data(), bwram.size());
  s.integer(bwram.dma);

  s.integer(status.counter);

  s.integer(status.interruptPending);

  s.integer(status.scanlines);
  s.integer(status.vcounter);
  s.integer(status.hcounter);

  s.integer(dma.line);

  s.integer(io.sa1_irq);
  s.integer(io.sa1_rdyb);
  s.integer(io.sa1_resb);
  s.integer(io.sa1_nmi);
  s.integer(io.smeg);

  s.integer(io.cpu_irqen);
  s.integer(io.chdma_irqen);

  s.integer(io.cpu_irqcl);
  s.integer(io.chdma_irqcl);

  s.integer(io.crv);

  s.integer(io.cnv);

  s.integer(io.civ);

  s.integer(io.cpu_irq);
  s.integer(io.cpu_ivsw);
  s.integer(io.cpu_nvsw);
  s.integer(io.cmeg);

  s.integer(io.sa1_irqen);
  s.integer(io.timer_irqen);
  s.integer(io.dma_irqen);
  s.integer(io.sa1_nmien);

  s.integer(io.sa1_irqcl);
  s.integer(io.timer_irqcl);
  s.integer(io.dma_irqcl);
  s.integer(io.sa1_nmicl);

  s.integer(io.snv);

  s.integer(io.siv);

  s.integer(io.hvselb);
  s.integer(io.ven);
  s.integer(io.hen);

  s.integer(io.hcnt);

  s.integer(io.vcnt);

  s.integer(io.cbmode);
  s.integer(io.cb);

  s.integer(io.dbmode);
  s.integer(io.db);

  s.integer(io.ebmode);
  s.integer(io.eb);

  s.integer(io.fbmode);
  s.integer(io.fb);

  s.integer(io.sbm);

  s.integer(io.sw46);
  s.integer(io.cbm);

  s.integer(io.swen);

  s.integer(io.cwen);

  s.integer(io.bwp);

  s.integer(io.siwp);

  s.integer(io.ciwp);

  s.integer(io.dmaen);
  s.integer(io.dprio);
  s.integer(io.cden);
  s.integer(io.cdsel);
  s.integer(io.dd);
  s.integer(io.sd);

  s.integer(io.chdend);
  s.integer(io.dmasize);
  s.integer(io.dmacb);

  s.integer(io.dsa);

  s.integer(io.dda);

  s.integer(io.dtc);

  s.integer(io.bbf);

  s.array(io.brf);

  s.integer(io.acm);
  s.integer(io.md);

  s.integer(io.ma);

  s.integer(io.mb);

  s.integer(io.hl);
  s.integer(io.vb);

  s.integer(io.va);
  s.integer(io.vbit);

  s.integer(io.cpu_irqfl);
  s.integer(io.chdma_irqfl);

  s.integer(io.sa1_irqfl);
  s.integer(io.timer_irqfl);
  s.integer(io.dma_irqfl);
  s.integer(io.sa1_nmifl);

  s.integer(io.hcr);

  s.integer(io.vcr);

  s.integer(io.mr);

  s.integer(io.overflow);
}
