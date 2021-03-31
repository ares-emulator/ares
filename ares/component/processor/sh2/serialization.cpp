auto SH2::serialize(serializer& s) -> void {
  s(R);
  s(PC);
  s(PR);
  s(GBR);
  s(VBR);
  s(MACH);
  s(MACL);
  s(CCR);
  s(SR.T);
  s(SR.S);
  s(SR.I);
  s(SR.Q);
  s(SR.M);
  s(PPC);
  s(PPM);
  s(ET);
  s(ID);
  s(exceptions);

  s(cache);
  s(intc);
  s(dmac);
  s(sci);
  s(wdt);
  s(ubc);
  s(frt);
  s(bsc);
  s(sbycr);
  s(divu);

  if constexpr(Accuracy::Recompiler) {
    recompiler.reset();
  }
}

auto SH2::Cache::serialize(serializer& s) -> void {
  s(lrus);
  s(tags);
  for(auto& line : lines) s(line.longs);
  s(enable);
  s(disableCode);
  s(disableData);
  s(twoWay);
  s(waySelect);
}

auto SH2::INTC::serialize(serializer& s) -> void {
  s(icr.vecmd);
  s(icr.nmie);
  s(icr.nmil);
  s(ipra.wdtip);
  s(ipra.dmacip);
  s(ipra.divuip);
  s(iprb.frtip);
  s(iprb.sciip);
  s(vcra.srxv);
  s(vcra.serv);
  s(vcrb.stev);
  s(vcrb.stxv);
  s(vcrc.focv);
  s(vcrc.ficv);
  s(vcrd.fovv);
  s(vcrwdt.bcmv);
  s(vcrwdt.witv);
}

auto SH2::DMAC::serialize(serializer& s) -> void {
  s(sar);
  s(dar);
  s(tcr);
  for(auto& chcr : this->chcr) {
    s(chcr.de);
    s(chcr.te);
    s(chcr.ie);
    s(chcr.ta);
    s(chcr.tb);
    s(chcr.dl);
    s(chcr.ds);
    s(chcr.al);
    s(chcr.am);
    s(chcr.ar);
    s(chcr.ts);
    s(chcr.sm);
    s(chcr.dm);
  }
  s(vcrdma);
  s(drcr);
  s(dmaor.dme);
  s(dmaor.nmif);
  s(dmaor.ae);
  s(dmaor.pr);
  s(dreq);
  s(pendingIRQ);
}

auto SH2::SCI::serialize(serializer& s) -> void {
  s(smr.cks);
  s(smr.mp);
  s(smr.stop);
  s(smr.oe);
  s(smr.pe);
  s(smr.chr);
  s(smr.ca);
  s(scr.cke);
  s(scr.teie);
  s(scr.mpie);
  s(scr.re);
  s(scr.te);
  s(scr.rie);
  s(scr.tie);
  s(ssr.mpbt);
  s(ssr.mpb);
  s(ssr.tend);
  s(ssr.per);
  s(ssr.fer);
  s(ssr.orer);
  s(ssr.rdrf);
  s(ssr.tdre);
  s(brr);
  s(tdr);
  s(rdr);
  s(pendingTransmitEmptyIRQ);
  s(pendingReceiveFullIRQ);
}

auto SH2::WDT::serialize(serializer& s) -> void {
  s(wtcsr.cks);
  s(wtcsr.tme);
  s(wtcsr.wtit);
  s(wtcsr.ovf);
  s(rstcsr.rsts);
  s(rstcsr.rste);
  s(rstcsr.wovf);
  s(wtcnt);
}

auto SH2::UBC::serialize(serializer& s) -> void {
  s(bara);
  s(bamra);
  s(bbra.sza);
  s(bbra.rwa);
  s(bbra.ida);
  s(bbra.cpa);
  s(barb);
  s(bamrb);
  s(bbrb.szb);
  s(bbrb.rwb);
  s(bbrb.idb);
  s(bbrb.cpb);
  s(bdrb);
  s(bdmrb);
  s(brcr.pcbb);
  s(brcr.dbeb);
  s(brcr.seq);
  s(brcr.cmfpb);
  s(brcr.cmfcb);
  s(brcr.pcba);
  s(brcr.umd);
  s(brcr.ebbe);
  s(brcr.cmfpa);
  s(brcr.cmfca);
}

auto SH2::FRT::serialize(serializer& s) -> void {
  s(tier.ovie);
  s(tier.ociae);
  s(tier.ocibe);
  s(tier.icie);
  s(ftcsr.cclra);
  s(ftcsr.ovf);
  s(ftcsr.ocfa);
  s(ftcsr.ocfb);
  s(ftcsr.icf);
  s(frc);
  s(ocra);
  s(ocrb);
  s(tcr.cks);
  s(tcr.iedg);
  s(tocr.olvla);
  s(tocr.olvlb);
  s(tocr.ocrs);
  s(ficr);
  s(counter);
  s(pendingOutputIRQ);
}

auto SH2::BSC::serialize(serializer& s) -> void {
  s(bcr1.dram);
  s(bcr1.a0lw);
  s(bcr1.a1lw);
  s(bcr1.ahlw);
  s(bcr1.pshr);
  s(bcr1.bstrom);
  s(bcr1.a2endian);
  s(bcr1.master);
  s(bcr2.a1sz);
  s(bcr2.a2sz);
  s(bcr2.a3sz);
  s(wcr.w0);
  s(wcr.w1);
  s(wcr.w2);
  s(wcr.w3);
  s(wcr.iw0);
  s(wcr.iw1);
  s(wcr.iw2);
  s(wcr.iw3);
  s(mcr.rcd);
  s(mcr.trp);
  s(mcr.rmode);
  s(mcr.rfsh);
  s(mcr.amx);
  s(mcr.sz);
  s(mcr.trwl);
  s(mcr.rasd);
  s(mcr.be);
  s(mcr.tras);
  s(rtcsr.cks);
  s(rtcsr.cmie);
  s(rtcsr.cmf);
  s(rtcnt);
  s(rtcor);
}

auto SH2::SBYCR::serialize(serializer& s) -> void {
  s(sci);
  s(frt);
  s(divu);
  s(mult);
  s(dmac);
  s(hiz);
  s(sby);
}

auto SH2::DIVU::serialize(serializer& s) -> void {
  s(dvsr);
  s(dvcr.ovf);
  s(dvcr.ovfie);
  s(vcrdiv);
  s(dvdnth);
  s(dvdntl);
}
