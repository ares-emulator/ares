auto SA1::serialize(serializer& s) -> void {
  WDC65816::serialize(s);
  Thread::serialize(s);

  s(iram);
  s(bwram);
  s(bwram.dma);

  s(status.counter);

  s(status.interruptPending);

  s(status.scanlines);
  s(status.vcounter);
  s(status.hcounter);

  s(dma.line);

  s(io.sa1_irq);
  s(io.sa1_rdyb);
  s(io.sa1_resb);
  s(io.sa1_nmi);
  s(io.smeg);

  s(io.cpu_irqen);
  s(io.chdma_irqen);

  s(io.cpu_irqcl);
  s(io.chdma_irqcl);

  s(io.crv);

  s(io.cnv);

  s(io.civ);

  s(io.cpu_irq);
  s(io.cpu_ivsw);
  s(io.cpu_nvsw);
  s(io.cmeg);

  s(io.sa1_irqen);
  s(io.timer_irqen);
  s(io.dma_irqen);
  s(io.sa1_nmien);

  s(io.sa1_irqcl);
  s(io.timer_irqcl);
  s(io.dma_irqcl);
  s(io.sa1_nmicl);

  s(io.snv);

  s(io.siv);

  s(io.hvselb);
  s(io.ven);
  s(io.hen);

  s(io.hcnt);

  s(io.vcnt);

  s(io.cbmode);
  s(io.cb);

  s(io.dbmode);
  s(io.db);

  s(io.ebmode);
  s(io.eb);

  s(io.fbmode);
  s(io.fb);

  s(io.sbm);

  s(io.sw46);
  s(io.cbm);

  s(io.swen);

  s(io.cwen);

  s(io.bwp);

  s(io.siwp);

  s(io.ciwp);

  s(io.dmaen);
  s(io.dprio);
  s(io.cden);
  s(io.cdsel);
  s(io.dd);
  s(io.sd);

  s(io.chdend);
  s(io.dmasize);
  s(io.dmacb);

  s(io.dsa);

  s(io.dda);

  s(io.dtc);

  s(io.bbf);

  s(io.brf);

  s(io.acm);
  s(io.md);

  s(io.ma);

  s(io.mb);

  s(io.hl);
  s(io.vb);

  s(io.va);
  s(io.vbit);

  s(io.cpu_irqfl);
  s(io.chdma_irqfl);

  s(io.sa1_irqfl);
  s(io.timer_irqfl);
  s(io.dma_irqfl);
  s(io.sa1_nmifl);

  s(io.hcr);

  s(io.vcr);

  s(io.mr);

  s(io.overflow);
}
