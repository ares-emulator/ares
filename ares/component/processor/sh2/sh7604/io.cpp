auto SH2::internalReadByte(u32 address, n8 data) -> n8 {
  switch(address) {

  //SMR: serial mode register
  case 0xffff'fe00:
    data.bit(0,1) = sci.smr.cks;
    data.bit(2)   = sci.smr.mp;
    data.bit(3)   = sci.smr.stop;
    data.bit(4)   = sci.smr.oe;
    data.bit(5)   = sci.smr.pe;
    data.bit(6)   = sci.smr.chr;
    data.bit(7)   = sci.smr.ca;
    return data;

  //BRR: bit rate register
  case 0xffff'fe01:
    data.bit(0,7) = sci.brr;
    return data;

  //SCR: serial control register
  case 0xffff'fe02:
    data.bit(0,1) = sci.scr.cke;
    data.bit(2)   = sci.scr.teie;
    data.bit(3)   = sci.scr.mpie;
    data.bit(4)   = sci.scr.re;
    data.bit(5)   = sci.scr.te;
    data.bit(6)   = sci.scr.rie;
    data.bit(7)   = sci.scr.tie;
    return data;

  //TDR: transmit data register
  case 0xffff'fe03:
    data.bit(0,7) = sci.tdr;
    return data;

  //SSR: serial status register
  case 0xffff'fe04:
    data.bit(0) = sci.ssr.mpbt;
    data.bit(1) = sci.ssr.mpb;
    data.bit(2) = sci.ssr.tend;
    data.bit(3) = sci.ssr.per;
    data.bit(4) = sci.ssr.fer;
    data.bit(5) = sci.ssr.orer;
    data.bit(6) = sci.ssr.rdrf;
    data.bit(7) = sci.ssr.tdre;
    return data;

  //RDR: receive data register
  case 0xffff'fe05:
    data.bit(0,7) = sci.rdr;
    return data;

  //TIER: timer interrupt enable register
  case 0xffff'fe10:
    data.bit(0) = 1;
    data.bit(1) = frt.tier.ovie;
    data.bit(2) = frt.tier.ocibe;
    data.bit(3) = frt.tier.ociae;
    data.bit(7) = frt.tier.icie;
    return data;

  //FTCSR: free-running timer control/status register
  case 0xffff'fe11:
    data.bit(0) = frt.ftcsr.cclra;
    data.bit(1) = frt.ftcsr.ovf;
    data.bit(2) = frt.ftcsr.ocfb;
    data.bit(3) = frt.ftcsr.ocfa;
    data.bit(7) = frt.ftcsr.icf;
    return data;

  //FRC: free-running counter
  case 0xffff'fe12: return frt.frc.byte(1);
  case 0xffff'fe13: return frt.frc.byte(0);

  //OCRA/OCRB: output compare registers A and B
  case 0xffff'fe14: return frt.tocr.ocrs == 0 ? frt.ocra.byte(1) : frt.ocrb.byte(1);
  case 0xffff'fe15: return frt.tocr.ocrs == 0 ? frt.ocra.byte(0) : frt.ocrb.byte(0);

  //TCR: timer control register
  case 0xffff'fe16:
    data.bit(0) = frt.tcr.cks.bit(0);
    data.bit(1) = frt.tcr.cks.bit(1);
    data.bit(7) = frt.tcr.iedg;
    return data;

  //TOCR: timer output compare control register
  case 0xffff'fe17:
    data.bit(0) = frt.tocr.olvlb;
    data.bit(1) = frt.tocr.olvla;
    data.bit(4) = frt.tocr.ocrs;
    data.bit(5) = 1;
    data.bit(6) = 1;
    data.bit(7) = 1;
    return data;

  //FICR: input capture register
  case 0xffff'fe18: return frt.ficr.byte(1);
  case 0xffff'fe19: return frt.ficr.byte(0);

  //IPRB: interrupt priority level setting register B
  case 0xffff'fe60:
    data.bit(0,3) = intc.iprb.frtip;
    data.bit(4,7) = intc.iprb.sciip;
    return data;
  case 0xffff'fe61: return data;

  //VCRA: vector number setting register A
  case 0xffff'fe62:
    data.bit(0,6) = intc.vcra.serv;
    return data;
  case 0xffff'fe63:
    data.bit(0,6) = intc.vcra.srxv;
    return data;

  //VCRB: vector number setting register B
  case 0xffff'fe64:
    data.bit(0,6) = intc.vcrb.stxv;
    return data;
  case 0xffff'fe65:
    data.bit(0,6) = intc.vcrb.stev;
    return data;

  //VCRC: vector number setting register C
  case 0xffff'fe66:
    data.bit(0,6) = intc.vcrc.ficv;
    return data;
  case 0xffff'fe67:
    data.bit(0,6) = intc.vcrc.focv;
    return data;

  //VCRD: vector number setting register D
  case 0xffff'fe68:
    data.bit(0,6) = intc.vcrd.fovv;
    return data;
  case 0xffff'fe69: return data;

  //DRCR0: DMA request/response selection control register 0
  case 0xffff'fe71:
    data.bit(0,1) = dmac.drcr[0];
    return data;

  //DRCR1: DMA request/response selection control register 1
  case 0xffff'fe72:
    data.bit(0,1) = dmac.drcr[1];
    return data;

  //WTCSR: watchdog timer control/status register
  case 0xffff'fe80:
    data.bit(0,2) = wdt.wtcsr.cks;
    data.bit(5)   = wdt.wtcsr.tme;
    data.bit(6)   = wdt.wtcsr.wtit;
    data.bit(7)   = wdt.wtcsr.ovf;
    return data;

  //WTCNT: watchdog timer counter
  case 0xffff'fe81:
    data.bit(0,7) = wdt.wtcnt;
    return data;

  //RSTCSR: reset control/status register
  case 0xffff'fe83:
    data.bit(5) = wdt.rstcsr.rsts;
    data.bit(6) = wdt.rstcsr.rste;
    data.bit(7) = wdt.rstcsr.wovf;
    return data;

  //SBYCR: standby control register
  case 0xffff'fe91:
    data.bit(0) = sbycr.sci;
    data.bit(1) = sbycr.frt;
    data.bit(2) = sbycr.divu;
    data.bit(3) = sbycr.mult;
    data.bit(4) = sbycr.dmac;
    data.bit(6) = sbycr.hiz;
    data.bit(7) = sbycr.sby;
    return data;

  //CCR: cache control register
  case 0xffff'fe92:
    data.bit(0)   = cache.enable;
    data.bit(1)   = cache.disableCode;
    data.bit(2)   = cache.disableData;
    data.bit(3)   = cache.twoWay == 2;
    data.bit(4)   = 0;  //cache purge always reads as 0
    data.bit(6,7) = cache.waySelect;
    return data;

  //ICR: interrupt control register
  case 0xffff'fee0:
    data.bit(0) = intc.icr.nmie;
    data.bit(7) = intc.icr.nmil;
    return data;
  case 0xffff'fee1:
    data.bit(0) = intc.icr.vecmd;
    return data;

  //IPRA: interrupt priority level setting register A
  case 0xffff'fee2:
    data.bit(0,3) = intc.ipra.dmacip;
    data.bit(4,7) = intc.ipra.divuip;
    return data;
  case 0xffff'fee3:
    data.bit(4,7) = intc.ipra.wdtip;
    return data;

  //VCRWDT: vector number setting register WDT
  case 0xffff'fee4:
    data.bit(0,6) = intc.vcrwdt.witv;
    return data;
  case 0xffff'fee5:
    data.bit(0,6) = intc.vcrwdt.bcmv;
    return data;

  //DVSR: divisor register
  case 0xffff'ff00: return divu.dvsr.byte(3);
  case 0xffff'ff01: return divu.dvsr.byte(2);
  case 0xffff'ff02: return divu.dvsr.byte(1);
  case 0xffff'ff03: return divu.dvsr.byte(0);

  //DVDNT: dividend register L for 32-bit division
  case 0xffff'ff04: return divu.dvdntl.byte(3);
  case 0xffff'ff05: return divu.dvdntl.byte(2);
  case 0xffff'ff06: return divu.dvdntl.byte(1);
  case 0xffff'ff07: return divu.dvdntl.byte(0);

  //DVCR: division control register
  case 0xffff'ff08: return data;
  case 0xffff'ff09: return data;
  case 0xffff'ff0a: return data;
  case 0xffff'ff0b:
    data.bit(0) = divu.dvcr.ovf;
    data.bit(1) = divu.dvcr.ovfie;
    return data;

  //VCRDIV: vector number setting register for division
  case 0xffff'ff0c: return data;
  case 0xffff'ff0d: return data;
  case 0xffff'ff0e: return data;
  case 0xffff'ff0f:
    data.bit(0,6) = divu.vcrdiv;
    return data;

  //DVDNTH: dividend register H
  case 0xffff'ff10: case 0xffff'ff18: return divu.dvdnth.byte(3);
  case 0xffff'ff11: case 0xffff'ff19: return divu.dvdnth.byte(2);
  case 0xffff'ff12: case 0xffff'ff1a: return divu.dvdnth.byte(1);
  case 0xffff'ff13: case 0xffff'ff1b: return divu.dvdnth.byte(0);

  //DVDNTL: dividend register L
  case 0xffff'ff14: case 0xffff'ff1c: return divu.dvdntl.byte(3);
  case 0xffff'ff15: case 0xffff'ff1d: return divu.dvdntl.byte(2);
  case 0xffff'ff16: case 0xffff'ff1e: return divu.dvdntl.byte(1);
  case 0xffff'ff17: case 0xffff'ff1f: return divu.dvdntl.byte(0);

  //BARA: break address register A
  case 0xffff'ff40: return ubc.bara.byte(3);
  case 0xffff'ff41: return ubc.bara.byte(2);
  case 0xffff'ff42: return ubc.bara.byte(1);
  case 0xffff'ff43: return ubc.bara.byte(0);

  //BAMRA: break address mask register A
  case 0xffff'ff44: return ubc.bamra.byte(3);
  case 0xffff'ff45: return ubc.bamra.byte(2);
  case 0xffff'ff46: return ubc.bamra.byte(1);
  case 0xffff'ff47: return ubc.bamra.byte(0);

  //BBRA: break bus register A
  case 0xffff'ff48: return data;
  case 0xffff'ff49:
    data.bit(0,1) = ubc.bbra.sza;
    data.bit(2,3) = ubc.bbra.rwa;
    data.bit(4,5) = ubc.bbra.ida;
    data.bit(6,7) = ubc.bbra.cpa;
    return data;

  //BARB: break address register B
  case 0xffff'ff60: return ubc.barb.byte(3);
  case 0xffff'ff61: return ubc.barb.byte(2);
  case 0xffff'ff62: return ubc.barb.byte(1);
  case 0xffff'ff63: return ubc.barb.byte(0);

  //BAMRB: break address mask register B
  case 0xffff'ff64: return ubc.bamrb.byte(3);
  case 0xffff'ff65: return ubc.bamrb.byte(2);
  case 0xffff'ff66: return ubc.bamrb.byte(1);
  case 0xffff'ff67: return ubc.bamrb.byte(0);

  //BBRB: break bus register B
  case 0xffff'ff68: return data;
  case 0xffff'ff69:
    data.bit(0,1) = ubc.bbrb.szb;
    data.bit(2,3) = ubc.bbrb.rwb;
    data.bit(4,5) = ubc.bbrb.idb;
    data.bit(6,7) = ubc.bbrb.cpb;
    return data;

  //BDRB: break data register B
  case 0xffff'ff70: return ubc.bdrb.byte(3);
  case 0xffff'ff71: return ubc.bdrb.byte(2);
  case 0xffff'ff72: return ubc.bdrb.byte(1);
  case 0xffff'ff73: return ubc.bdrb.byte(0);

  //BDMRB: break data mask register B
  case 0xffff'ff74: return ubc.bdmrb.byte(3);
  case 0xffff'ff75: return ubc.bdmrb.byte(2);
  case 0xffff'ff76: return ubc.bdmrb.byte(1);
  case 0xffff'ff77: return ubc.bdmrb.byte(0);

  //BRCR: break control register
  case 0xffff'ff78:
    data.bit(2) = ubc.brcr.pcba;
    data.bit(4) = ubc.brcr.umd;
    data.bit(5) = ubc.brcr.ebbe;
    data.bit(6) = ubc.brcr.cmfpa;
    data.bit(7) = ubc.brcr.cmfca;
    return data;
  case 0xffff'ff79:
    data.bit(2) = ubc.brcr.pcbb;
    data.bit(3) = ubc.brcr.dbeb;
    data.bit(4) = ubc.brcr.seq;
    data.bit(6) = ubc.brcr.cmfpb;
    data.bit(7) = ubc.brcr.cmfcb;
    return data;

  //SAR0: DMA source address register 0
  case 0xffff'ff80: return dmac.sar[0].byte(3);
  case 0xffff'ff81: return dmac.sar[0].byte(2);
  case 0xffff'ff82: return dmac.sar[0].byte(1);
  case 0xffff'ff83: return dmac.sar[0].byte(0);

  //DAR0: DMA destination address register 0
  case 0xffff'ff84: return dmac.dar[0].byte(3);
  case 0xffff'ff85: return dmac.dar[0].byte(2);
  case 0xffff'ff86: return dmac.dar[0].byte(1);
  case 0xffff'ff87: return dmac.dar[0].byte(0);

  //TCR0: DMA transfer count register 0
  case 0xffff'ff88: return data;
  case 0xffff'ff89: return dmac.tcr[0].byte(2);
  case 0xffff'ff8a: return dmac.tcr[0].byte(1);
  case 0xffff'ff8b: return dmac.tcr[0].byte(0);

  //CHCR0: DMA channel control register 0
  case 0xffff'ff8c: return data;
  case 0xffff'ff8d: return data;
  case 0xffff'ff8e:
    data.bit(0) = dmac.chcr[0].am;
    data.bit(1) = dmac.chcr[0].ar;
    data.bit(2) = dmac.chcr[0].ts.bit(0);
    data.bit(3) = dmac.chcr[0].ts.bit(1);
    data.bit(4) = dmac.chcr[0].sm.bit(0);
    data.bit(5) = dmac.chcr[0].sm.bit(1);
    data.bit(6) = dmac.chcr[0].dm.bit(0);
    data.bit(7) = dmac.chcr[0].dm.bit(1);
    return data;
  case 0xffff'ff8f:
    data.bit(0) = dmac.chcr[0].de;
    data.bit(1) = dmac.chcr[0].te;
    data.bit(2) = dmac.chcr[0].ie;
    data.bit(3) = dmac.chcr[0].ta;
    data.bit(4) = dmac.chcr[0].tb;
    data.bit(5) = dmac.chcr[0].dl;
    data.bit(6) = dmac.chcr[0].ds;
    data.bit(7) = dmac.chcr[0].al;
    return data;

  //SAR1: DMA source address register 1
  case 0xffff'ff90: return dmac.sar[1].byte(3);
  case 0xffff'ff91: return dmac.sar[1].byte(2);
  case 0xffff'ff92: return dmac.sar[1].byte(1);
  case 0xffff'ff93: return dmac.sar[1].byte(0);

  //DAR1: DMA destination address register 1
  case 0xffff'ff94: return dmac.dar[1].byte(3);
  case 0xffff'ff95: return dmac.dar[1].byte(2);
  case 0xffff'ff96: return dmac.dar[1].byte(1);
  case 0xffff'ff97: return dmac.dar[1].byte(0);

  //TCR1: DMA transfer count register 1
  case 0xffff'ff98: return data;
  case 0xffff'ff99: return dmac.tcr[1].byte(2);
  case 0xffff'ff9a: return dmac.tcr[1].byte(1);
  case 0xffff'ff9b: return dmac.tcr[1].byte(0);

  //CHCR1: DMA channel control register 1
  case 0xffff'ff9c: return data;
  case 0xffff'ff9d: return data;
  case 0xffff'ff9e:
    data.bit(0) = dmac.chcr[1].am;
    data.bit(1) = dmac.chcr[1].ar;
    data.bit(2) = dmac.chcr[1].ts.bit(0);
    data.bit(3) = dmac.chcr[1].ts.bit(1);
    data.bit(4) = dmac.chcr[1].sm.bit(0);
    data.bit(5) = dmac.chcr[1].sm.bit(1);
    data.bit(6) = dmac.chcr[1].dm.bit(0);
    data.bit(7) = dmac.chcr[1].dm.bit(1);
    return data;
  case 0xffff'ff9f:
    data.bit(0) = dmac.chcr[1].de;
    data.bit(1) = dmac.chcr[1].te;
    data.bit(2) = dmac.chcr[1].ie;
    data.bit(3) = dmac.chcr[1].ta;
    data.bit(4) = dmac.chcr[1].tb;
    data.bit(5) = dmac.chcr[1].dl;
    data.bit(6) = dmac.chcr[1].ds;
    data.bit(7) = dmac.chcr[1].al;
    return data;

  //VCRDMA0: DMA vector number register 0
  case 0xffff'ffa0: return data;
  case 0xffff'ffa1: return data;
  case 0xffff'ffa2: return data;
  case 0xffff'ffa3: return dmac.vcrdma[0];

  //VCRDMA1: DMA vector number register 1
  case 0xffff'ffa8: return data;
  case 0xffff'ffa9: return data;
  case 0xffff'ffaa: return data;
  case 0xffff'ffab: return dmac.vcrdma[1];

  //DMAOR: DMA operation register
  case 0xffff'ffb0: return data;
  case 0xffff'ffb1: return data;
  case 0xffff'ffb2: return data;
  case 0xffff'ffb3:
    data.bit(0) = dmac.dmaor.dme;
    data.bit(1) = dmac.dmaor.nmif;
    data.bit(2) = dmac.dmaor.ae;
    data.bit(3) = dmac.dmaor.pr;
    return data;

  //BCR1: bus control register 1
  case 0xffff'ffe0: return data;
  case 0xffff'ffe1: return data;
  case 0xffff'ffe2:
    data.bit(0,1) = bsc.bcr1.ahlw;
    data.bit(2)   = bsc.bcr1.pshr;
    data.bit(3)   = bsc.bcr1.bstrom;
    data.bit(4)   = bsc.bcr1.a2endian;
    data.bit(7)   = bsc.bcr1.master;
    return data;
  case 0xffff'ffe3:
    data.bit(0,2) = bsc.bcr1.dram;
    data.bit(4,5) = bsc.bcr1.a0lw;
    data.bit(6,7) = bsc.bcr1.a1lw;
    return data;

  //BCR2: bus control register 2
  case 0xffff'ffe4: return data;
  case 0xffff'ffe5: return data;
  case 0xffff'ffe6: return data;
  case 0xffff'ffe7:
    data.bit(2,3) = bsc.bcr2.a1sz;
    data.bit(4,5) = bsc.bcr2.a2sz;
    data.bit(6,7) = bsc.bcr2.a3sz;
    return data;

  //WCR: wait control register 
  case 0xffff'ffe8: return data;
  case 0xffff'ffe9: return data;
  case 0xffff'ffea:
    data.bit(0,1) = bsc.wcr.iw0;
    data.bit(2,3) = bsc.wcr.iw1;
    data.bit(4,5) = bsc.wcr.iw2;
    data.bit(6,7) = bsc.wcr.iw3;
    return data;
  case 0xffff'ffeb:
    data.bit(0,1) = bsc.wcr.w0;
    data.bit(2,3) = bsc.wcr.w1;
    data.bit(4,5) = bsc.wcr.w2;
    data.bit(6,7) = bsc.wcr.w3;
    return data;

  //MCR: individual memory control register
  case 0xffff'ffec: return data;
  case 0xffff'ffed: return data;
  case 0xffff'ffee:
    data.bit(1) = bsc.mcr.rasd;
    data.bit(2) = bsc.mcr.be;
    data.bit(3) = bsc.mcr.tras.bit(0);
    data.bit(4) = bsc.mcr.tras.bit(1);
    data.bit(5) = bsc.mcr.trwl;
    data.bit(6) = bsc.mcr.rcd;
    data.bit(7) = bsc.mcr.trp;
    return data;
  case 0xffff'ffef:
    data.bit(2) = bsc.mcr.rmode;
    data.bit(3) = bsc.mcr.rfsh;
    data.bit(4) = bsc.mcr.amx.bit(0);
    data.bit(5) = bsc.mcr.amx.bit(1);
    data.bit(6) = bsc.mcr.sz;
    data.bit(7) = bsc.mcr.amx.bit(2);
    return data;

  //RTCSR: refresh timer control/status register
  case 0xffff'fff0: return data;
  case 0xffff'fff1: return data;
  case 0xffff'fff2: return data;
  case 0xffff'fff3:
    data.bit(3,5) = bsc.rtcsr.cks;
    data.bit(6)   = bsc.rtcsr.cmie;
    data.bit(7)   = bsc.rtcsr.cmf;
    return data;

  //RTCNT: refresh timer counter
  case 0xffff'fff4: return data;
  case 0xffff'fff5: return data;
  case 0xffff'fff6: return data;
  case 0xffff'fff7:
    data.bit(0,7) = bsc.rtcnt;
    return data;

  //RTCOR: refresh time constant register
  case 0xffff'fff8: return data;
  case 0xffff'fff9: return data;
  case 0xffff'fffa: return data;
  case 0xffff'fffb:
    data.bit(0,7) = bsc.rtcor;
    return data;

  }

  debug(unusual, "[SH2] read(0x", hex(address, 8L), ")");
  return data;
}

auto SH2::internalWriteByte(u32 address, n8 data) -> void {
  switch(address) {

  //SMR: serial mode register
  case 0xffff'fe00:
    sci.smr.cks  = data.bit(0,1);
    sci.smr.mp   = data.bit(2);
    sci.smr.stop = data.bit(3);
    sci.smr.oe   = data.bit(4);
    sci.smr.pe   = data.bit(5);
    sci.smr.chr  = data.bit(6);
    sci.smr.ca   = data.bit(7);
    return;

  //BRR: bit rate register
  case 0xffff'fe01:
    sci.brr = data.bit(0,7);
    return;

  //SCR: serial control register
  case 0xffff'fe02: {
    bool te = sci.scr.te;
    sci.scr.cke  = data.bit(0,1);
    sci.scr.teie = data.bit(2);
    sci.scr.mpie = data.bit(3);
    sci.scr.re   = data.bit(4);
    sci.scr.te   = data.bit(5);
    sci.scr.rie  = data.bit(6);
    sci.scr.tie  = data.bit(7);
    if(!te && sci.scr.te) sci.run();
    return;
  }

  //TDR: transmit data register
  case 0xffff'fe03:
    sci.tdr = data.bit(0,7);
    return;

  //SSR: serial status register
  case 0xffff'fe04:
    sci.ssr.mpbt  = data.bit(0);
  //sci.ssr.mpb   = data.bit(1) = readonly;
  //sci.ssr.tend  = data.bit(2) = readonly;
    sci.ssr.per  &= data.bit(3);
    sci.ssr.fer  &= data.bit(4);
    sci.ssr.orer &= data.bit(5);
    sci.ssr.rdrf &= data.bit(6);
    sci.ssr.tdre &= data.bit(7);
    if(sci.scr.te) sci.run();
    return;

  //RDR: receive data register
  case 0xffff'fe05:
    sci.rdr = data.bit(0,7);
    return;

  //TIER: timer interrupt enable register
  case 0xffff'fe10:
    frt.tier.ovie  = data.bit(1);
    frt.tier.ocibe = data.bit(2);
    frt.tier.ociae = data.bit(3);
    frt.tier.icie  = data.bit(7);
    return;

  //FTCSR: free-running timer control/status register
  case 0xffff'fe11:
    frt.ftcsr.cclra = data.bit(0);
    frt.ftcsr.ovf  &= data.bit(1);
    frt.ftcsr.ocfb &= data.bit(2);
    frt.ftcsr.ocfa &= data.bit(3);
    frt.ftcsr.icf  &= data.bit(7);
    return;

  //FRC: free-running counter
  case 0xffff'fe12: frt.frc.byte(1) = data; return;
  case 0xffff'fe13: frt.frc.byte(0) = data; return;

  //OCRA/OCRB: output compare registers A and B
  case 0xffff'fe14:
    if(frt.tocr.ocrs == 0) frt.ocra.byte(1) = data;
    if(frt.tocr.ocrs == 1) frt.ocrb.byte(1) = data;
    return;
  case 0xffff'fe15:
    if(frt.tocr.ocrs == 0) frt.ocra.byte(0) = data;
    if(frt.tocr.ocrs == 1) frt.ocrb.byte(0) = data;
    return;

  //TCR: timer control register
  case 0xffff'fe16:
    frt.tcr.cks  = data.bit(0,1);
    frt.tcr.iedg = data.bit(2);
    return;

  //TOCR: timer output compare control register
  case 0xffff'fe17:
    frt.tocr.olvlb = data.bit(0);
    frt.tocr.olvla = data.bit(1);
    frt.tocr.ocrs  = data.bit(4);
    return;

  //FICR: input capture register (read-only)
  case 0xffff'fe18: return;
  case 0xffff'fe19: return;

  //IPRB: interrupt priority level setting register B
  case 0xffff'fe60:
    intc.iprb.frtip = data.bit(0,3);
    intc.iprb.sciip = data.bit(4,7);
    return;
  case 0xffff'fe61: return;

  //VCRA: vector number setting register A
  case 0xffff'fe62:
    intc.vcra.serv = data.bit(0,6);
    return;
  case 0xffff'fe63:
    intc.vcra.srxv = data.bit(0,6);
    return;

  //VCRB: vector number setting register B
  case 0xffff'fe64:
    intc.vcrb.stxv = data.bit(0,6);
    return;
  case 0xffff'fe65:
    intc.vcrb.stev = data.bit(0,6);
    return;

  //VCRC: vector number setting register C
  case 0xffff'fe66:
    intc.vcrc.ficv = data.bit(0,6);
    return;
  case 0xffff'fe67:
    intc.vcrc.focv = data.bit(0,6);
    return;

  //VCRD: vector number setting register D
  case 0xffff'fe68:
    intc.vcrd.fovv = data.bit(0,6);
    return;
  case 0xffff'fe69: return;

  //DRCR0: DMA request/response selection control register 0
  case 0xffff'fe71:
    dmac.drcr[0] = data.bit(0,1);
    return;

  //DRCR1: DMA request/response selection control register 1
  case 0xffff'fe72:
    dmac.drcr[1] = data.bit(0,1);
    return;

  //WTCSR: watchdog timer control/status register
  case 0xffff'fe80:
    wdt.wtcsr.cks  = data.bit(0,2);
    wdt.wtcsr.tme  = data.bit(5);
    wdt.wtcsr.wtit = data.bit(6);
    wdt.wtcsr.ovf &= data.bit(7);
    return;

  //WTCNT: watchdog timer counter
  case 0xffff'fe81:
    wdt.wtcnt = data.bit(0,7);
    return;

  //RSTCSR: reset control/status register
  case 0xffff'fe83:
    wdt.rstcsr.rsts = data.bit(5);
    wdt.rstcsr.rste = data.bit(6);
    wdt.rstcsr.wovf = data.bit(7);
    return;

  //SBYCR: standby control register
  case 0xffff'fe91:
    sbycr.sci  = data.bit(0);
    sbycr.frt  = data.bit(1);
    sbycr.divu = data.bit(2);
    sbycr.mult = data.bit(3);
    sbycr.dmac = data.bit(4);
    sbycr.hiz  = data.bit(6);
    sbycr.sby  = data.bit(7);
    return;

  //CCR: cache control register
  case 0xffff'fe92:
    cache.enable      = data.bit(0);
    cache.disableCode = data.bit(1);
    cache.disableData = data.bit(2);
    cache.twoWay      = data.bit(3) ? 2 : 0;
    cache.waySelect   = data.bit(6,7);
    if(data.bit(3)) cache.purge<2>();  //purge ways 0-1
    if(data.bit(4)) cache.purge<4>();  //purge ways 0-3
    return;

  //ICR: interrupt control register
  case 0xffff'fee0:
    intc.icr.nmie = data.bit(0);
  //intc.icr.nmil = data.bit(7) = readonly;
    return;
  case 0xffff'fee1:
    intc.icr.vecmd = data.bit(0);
    return;

  //IPRA: interrupt priority level setting register A
  case 0xffff'fee2:
    intc.ipra.dmacip = data.bit(0,3);
    intc.ipra.divuip = data.bit(4,7);
    return;
  case 0xffff'fee3:
    intc.ipra.wdtip = data.bit(4,7);
    return;

  //VCRWDT: vector number setting register WDT
  case 0xffff'fee4:
    intc.vcrwdt.witv = data.bit(0,6);
    return;
  case 0xffff'fee5:
    intc.vcrwdt.bcmv = data.bit(0,6);
    return;

  //DVSR: divisor register
  case 0xffff'ff00: divu.dvsr.byte(3) = data; return;
  case 0xffff'ff01: divu.dvsr.byte(2) = data; return;
  case 0xffff'ff02: divu.dvsr.byte(1) = data; return;
  case 0xffff'ff03: divu.dvsr.byte(0) = data; return;

  //DVDNT: dividend register L for 32-bit division
  case 0xffff'ff04: divu.dvdntl.byte(3) = data; return;
  case 0xffff'ff05: divu.dvdntl.byte(2) = data; return;
  case 0xffff'ff06: divu.dvdntl.byte(1) = data; return;
  case 0xffff'ff07: divu.dvdntl.byte(0) = data; {
    if(divu.dvsr) {
      s32 dividend  = (s32)divu.dvdntl;
      s32 quotient  = dividend / (s32)divu.dvsr;
      s32 remainder = dividend % (s32)divu.dvsr;
      divu.dvdntl = quotient;
      divu.dvdnth = remainder;
    } else {
      //division by zero
      divu.dvdntl = +0x7fff'ffff;
      divu.dvcr.ovf = 1;
    }
    return;
  }

  //DVCR: division control register
  case 0xffff'ff08: return;
  case 0xffff'ff09: return;
  case 0xffff'ff0a: return;
  case 0xffff'ff0b:
    divu.dvcr.ovf   = data.bit(0);
    divu.dvcr.ovfie = data.bit(1);
    return;

  //VCRDIV: vector number setting register for division
  case 0xffff'ff0c: return;
  case 0xffff'ff0d: return;
  case 0xffff'ff0e: return;
  case 0xffff'ff0f:
    divu.vcrdiv = data.bit(0,6);
    return;

  //DVDNTH: dividend register H
  case 0xffff'ff10: case 0xffff'ff18: divu.dvdnth.byte(3) = data; return;
  case 0xffff'ff11: case 0xffff'ff19: divu.dvdnth.byte(2) = data; return;
  case 0xffff'ff12: case 0xffff'ff1a: divu.dvdnth.byte(1) = data; return;
  case 0xffff'ff13: case 0xffff'ff1b: divu.dvdnth.byte(0) = data; return;

  //DVDNTL: dividend register L
  case 0xffff'ff14: case 0xffff'ff1c: divu.dvdntl.byte(3) = data; return;
  case 0xffff'ff15: case 0xffff'ff1d: divu.dvdntl.byte(2) = data; return;
  case 0xffff'ff16: case 0xffff'ff1e: divu.dvdntl.byte(1) = data; return;
  case 0xffff'ff17: case 0xffff'ff1f: divu.dvdntl.byte(0) = data; {
    if(divu.dvsr) {
      s64 dividend  = (s64)divu.dvdnth << 32 | (s64)divu.dvdntl << 0;
      s64 quotient  = dividend / (s32)divu.dvsr;
      s32 remainder = dividend % (s32)divu.dvsr;
      divu.dvdntl = quotient;
      divu.dvdnth = remainder;
      if(quotient > +0x7fff'7fffLL) {
        //overflow
        divu.dvdntl = +0x7fff'ffff;
        divu.dvcr.ovf = 1;
      }
      if(quotient < -0x8000'0000LL) {
        //underflow
        divu.dvdntl = -0x8000'0000;
        divu.dvcr.ovf = 1;
      }
    } else {
      //division by zero
      divu.dvdntl = +0x7fff'ffff;
      divu.dvcr.ovf = 1;
    }
    return;
  }

  //BARA: break address register A
  case 0xffff'ff40: ubc.bara.byte(3) = data; return;
  case 0xffff'ff41: ubc.bara.byte(2) = data; return;
  case 0xffff'ff42: ubc.bara.byte(1) = data; return;
  case 0xffff'ff43: ubc.bara.byte(0) = data; return;

  //BAMRA: break address mask register A
  case 0xffff'ff44: ubc.bamra.byte(3) = data; return;
  case 0xffff'ff45: ubc.bamra.byte(2) = data; return;
  case 0xffff'ff46: ubc.bamra.byte(1) = data; return;
  case 0xffff'ff47: ubc.bamra.byte(0) = data; return;

  //BBRA: break bus register A
  case 0xffff'ff48: return;
  case 0xffff'ff49:
    ubc.bbra.sza = data.bit(0,1);
    ubc.bbra.rwa = data.bit(2,3);
    ubc.bbra.ida = data.bit(4,5);
    ubc.bbra.cpa = data.bit(6,7);
    return;

  //BARB: break address register B
  case 0xffff'ff60: ubc.barb.byte(3) = data; return;
  case 0xffff'ff61: ubc.barb.byte(2) = data; return;
  case 0xffff'ff62: ubc.barb.byte(1) = data; return;
  case 0xffff'ff63: ubc.barb.byte(0) = data; return;

  //BAMRB: break address mask register B
  case 0xffff'ff64: ubc.bamrb.byte(3) = data; return;
  case 0xffff'ff65: ubc.bamrb.byte(2) = data; return;
  case 0xffff'ff66: ubc.bamrb.byte(1) = data; return;
  case 0xffff'ff67: ubc.bamrb.byte(0) = data; return;

  //BBRB: break bus register B
  case 0xffff'ff68: return;
  case 0xffff'ff69:
    ubc.bbrb.szb = data.bit(0,1);
    ubc.bbrb.rwb = data.bit(2,3);
    ubc.bbrb.idb = data.bit(4,5);
    ubc.bbrb.cpb = data.bit(6,7);
    return;

  //BDRB: break data register B
  case 0xffff'ff70: ubc.bdrb.byte(3) = data; return;
  case 0xffff'ff71: ubc.bdrb.byte(2) = data; return;
  case 0xffff'ff72: ubc.bdrb.byte(1) = data; return;
  case 0xffff'ff73: ubc.bdrb.byte(0) = data; return;

  //BDMRB: break data mask register B
  case 0xffff'ff74: ubc.bdmrb.byte(3) = data; return;
  case 0xffff'ff75: ubc.bdmrb.byte(2) = data; return;
  case 0xffff'ff76: ubc.bdmrb.byte(1) = data; return;
  case 0xffff'ff77: ubc.bdmrb.byte(0) = data; return;

  //BRCR: break control register
  case 0xffff'ff78:
    ubc.brcr.pcba  = data.bit(2);
    ubc.brcr.umd   = data.bit(4);
    ubc.brcr.ebbe  = data.bit(5);
    ubc.brcr.cmfpa = data.bit(6);
    ubc.brcr.cmfca = data.bit(7);
    return;
  case 0xffff'ff79:
    ubc.brcr.pcbb  = data.bit(2);
    ubc.brcr.dbeb  = data.bit(3);
    ubc.brcr.seq   = data.bit(4);
    ubc.brcr.cmfpb = data.bit(6);
    ubc.brcr.cmfcb = data.bit(7);
    return;

  //SAR0: DMA source address register 0
  case 0xffff'ff80: dmac.sar[0].byte(3) = data; return;
  case 0xffff'ff81: dmac.sar[0].byte(2) = data; return;
  case 0xffff'ff82: dmac.sar[0].byte(1) = data; return;
  case 0xffff'ff83: dmac.sar[0].byte(0) = data; return;

  //DAR0: DMA destination address register 0
  case 0xffff'ff84: dmac.dar[0].byte(3) = data; return;
  case 0xffff'ff85: dmac.dar[0].byte(2) = data; return;
  case 0xffff'ff86: dmac.dar[0].byte(1) = data; return;
  case 0xffff'ff87: dmac.dar[0].byte(0) = data; return;

  //TCR0: DMA transfer count register 0
  case 0xffff'ff88: return;
  case 0xffff'ff89: dmac.tcr[0].byte(2) = data; return;
  case 0xffff'ff8a: dmac.tcr[0].byte(1) = data; return;
  case 0xffff'ff8b: dmac.tcr[0].byte(0) = data; return;

  //CHCR0: DMA channel control register 0
  case 0xffff'ff8c: return;
  case 0xffff'ff8d: return;
  case 0xffff'ff8e:
    dmac.chcr[0].am  = data.bit(0);
    dmac.chcr[0].ar  = data.bit(1);
    dmac.chcr[0].ts  = data.bit(2,3);
    dmac.chcr[0].sm  = data.bit(4,5);
    dmac.chcr[0].dm  = data.bit(6,7);
    return;
  case 0xffff'ff8f:
    dmac.chcr[0].de  = data.bit(0);
    dmac.chcr[0].te &= data.bit(1);
    dmac.chcr[0].ie  = data.bit(2);
    dmac.chcr[0].ta  = data.bit(3);
    dmac.chcr[0].tb  = data.bit(4);
    dmac.chcr[0].dl  = data.bit(5);
    dmac.chcr[0].ds  = data.bit(6);
    dmac.chcr[0].al  = data.bit(7);
    return;

  //SAR1: DMA source address register 1
  case 0xffff'ff90: dmac.sar[1].byte(3) = data; return;
  case 0xffff'ff91: dmac.sar[1].byte(2) = data; return;
  case 0xffff'ff92: dmac.sar[1].byte(1) = data; return;
  case 0xffff'ff93: dmac.sar[1].byte(0) = data; return;

  //DAR1: DMA destination address register 1
  case 0xffff'ff94: dmac.dar[1].byte(3) = data; return;
  case 0xffff'ff95: dmac.dar[1].byte(2) = data; return;
  case 0xffff'ff96: dmac.dar[1].byte(1) = data; return;
  case 0xffff'ff97: dmac.dar[1].byte(0) = data; return;

  //TCR1: DMA transfer count register 1
  case 0xffff'ff98: return;
  case 0xffff'ff99: dmac.tcr[1].byte(2) = data; return;
  case 0xffff'ff9a: dmac.tcr[1].byte(1) = data; return;
  case 0xffff'ff9b: dmac.tcr[1].byte(0) = data; return;

  //CHCR1: DMA channel control register 1
  case 0xffff'ff9c: return;
  case 0xffff'ff9d: return;
  case 0xffff'ff9e:
    dmac.chcr[1].am  = data.bit(0);
    dmac.chcr[1].ar  = data.bit(1);
    dmac.chcr[1].ts  = data.bit(2,3);
    dmac.chcr[1].sm  = data.bit(4,5);
    dmac.chcr[1].dm  = data.bit(6,7);
    return;
  case 0xffff'ff9f:
    dmac.chcr[1].de  = data.bit(0);
    dmac.chcr[1].te &= data.bit(1);
    dmac.chcr[1].ie  = data.bit(2);
    dmac.chcr[1].ta  = data.bit(3);
    dmac.chcr[1].tb  = data.bit(4);
    dmac.chcr[1].dl  = data.bit(5);
    dmac.chcr[1].ds  = data.bit(6);
    dmac.chcr[1].al  = data.bit(7);
    return;

  //VCRDMA0: DMA vector number register 0
  case 0xffff'ffa0: return;
  case 0xffff'ffa1: return;
  case 0xffff'ffa2: return;
  case 0xffff'ffa3:
    dmac.vcrdma[0] = data.bit(0,7);
    return;

  //VCRDMA1: DMA vector number register 1
  case 0xffff'ffa8: return;
  case 0xffff'ffa9: return;
  case 0xffff'ffaa: return;
  case 0xffff'ffab:
    dmac.vcrdma[1] = data.bit(0,7);
    return;

  //DMAOR: DMA operation register
  case 0xffff'ffb0: return;
  case 0xffff'ffb1: return;
  case 0xffff'ffb2: return;
  case 0xffff'ffb3:
    dmac.dmaor.dme   = data.bit(0);
    dmac.dmaor.nmif &= data.bit(1);
    dmac.dmaor.ae   &= data.bit(2);
    dmac.dmaor.pr    = data.bit(3);
    return;

  //BCR1: bus control register 1
  case 0xffff'ffe0: return;
  case 0xffff'ffe1: return;
  case 0xffff'ffe2:
    bsc.bcr1.ahlw     = data.bit(0,1);
    bsc.bcr1.pshr     = data.bit(2);
    bsc.bcr1.bstrom   = data.bit(3);
    bsc.bcr1.a2endian = data.bit(4);
    bsc.bcr1.master   = data.bit(7);
    return;
  case 0xffff'ffe3:
    bsc.bcr1.dram     = data.bit(0,2);
    bsc.bcr1.a0lw     = data.bit(4,5);
    bsc.bcr1.a1lw     = data.bit(6,7);
    return;

  //BCR2: bus control register 2
  case 0xffff'ffe4: return;
  case 0xffff'ffe5: return;
  case 0xffff'ffe6: return;
  case 0xffff'ffe7:
    bsc.bcr2.a1sz = data.bit(2,3);
    bsc.bcr2.a2sz = data.bit(4,5);
    bsc.bcr2.a3sz = data.bit(6,7);
    return;

  //WCR: wait control register 
  case 0xffff'ffe8: return;
  case 0xffff'ffe9: return;
  case 0xffff'ffea:
    bsc.wcr.iw0 = data.bit(0,1);
    bsc.wcr.iw1 = data.bit(2,3);
    bsc.wcr.iw2 = data.bit(4,5);
    bsc.wcr.iw3 = data.bit(6,7);
    return;
  case 0xffff'ffeb:
    bsc.wcr.w0 = data.bit(0,1);
    bsc.wcr.w1 = data.bit(2,3);
    bsc.wcr.w2 = data.bit(4,5);
    bsc.wcr.w3 = data.bit(6,7);
    return;

  //MCR: individual memory control register
  case 0xffff'ffec: return;
  case 0xffff'ffed: return;
  case 0xffff'ffee:
    bsc.mcr.rasd = data.bit(1);
    bsc.mcr.be   = data.bit(2);
    bsc.mcr.tras = data.bit(3,4);
    bsc.mcr.trwl = data.bit(5);
    bsc.mcr.rcd  = data.bit(6);
    bsc.mcr.trp  = data.bit(7);
    return;
  case 0xffff'ffef:
    bsc.mcr.rmode      = data.bit(2);
    bsc.mcr.rfsh       = data.bit(3);
    bsc.mcr.amx.bit(0) = data.bit(4);
    bsc.mcr.amx.bit(1) = data.bit(5);
    bsc.mcr.sz         = data.bit(6);
    bsc.mcr.amx.bit(2) = data.bit(7);
    return;

  //RTCSR: refresh timer control/status register
  case 0xffff'fff0: return;
  case 0xffff'fff1: return;
  case 0xffff'fff2: return;
  case 0xffff'fff3:
    bsc.rtcsr.cks  = data.bit(3,5);
    bsc.rtcsr.cmie = data.bit(6);
    bsc.rtcsr.cmf  = data.bit(7);
    return;

  //RTCNT: refresh timer counter
  case 0xffff'fff4: return;
  case 0xffff'fff5: return;
  case 0xffff'fff6: return;
  case 0xffff'fff7:
    bsc.rtcnt = data.bit(0,7);
    return;

  //RTCOR: refresh tim constant register
  case 0xffff'fff8: return;
  case 0xffff'fff9: return;
  case 0xffff'fffa: return;
  case 0xffff'fffb:
    bsc.rtcor = data.bit(0,7);
    return;

  }

  debug(unusual, "[SH2] write(0x", hex(address, 8L), ", 0x", hex(data, 2L), ")");
}
