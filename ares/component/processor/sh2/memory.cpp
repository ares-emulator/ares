auto SH2::readByte(u32 address) -> u32 {
  //$c000:0000-$c000:0fff
  if((address & 0xffff'f000) == 0xc000'0000) {
    return cache[address & 0xfff];
  }

  //$ffff:fe00-$ffff:ffff
  if(address >= 0xffff'fe00) {
    return internalReadByte(address);
  }

  return busReadByte(address);
}

auto SH2::readWord(u32 address) -> u32 {
  //$c000:0000-$c000:0fff
  if((address & 0xffff'f000) == 0xc000'0000) {
    u16 data = 0;
    data |= cache[address & 0xffe | 0] << 8;
    data |= cache[address & 0xffe | 1] << 0;
    return data;
  }

  //$ffff:fe00-$ffff:ffff
  if(address >= 0xffff'fe00) {
    u16 data = 0;
    data |= internalReadByte(address & ~1 | 0) << 8;
    data |= internalReadByte(address & ~1 | 1) << 0;
    return data;
  }

  return busReadWord(address & ~1);
}

auto SH2::readLong(u32 address) -> u32 {
  //$c000:0000-$c000:0fff
  if((address & 0xffff'f000) == 0xc000'0000) {
    u32 data = 0;
    data |= cache[address & 0xffc | 0] << 24;
    data |= cache[address & 0xffc | 1] << 16;
    data |= cache[address & 0xffc | 2] <<  8;
    data |= cache[address & 0xffc | 3] <<  0;
    return data;
  }

  //$ffff:fe00-$ffff:ffff
  if(address >= 0xffff'fe00) {
    u32 data = 0;
    data |= internalReadByte(address & ~3 | 0) << 24;
    data |= internalReadByte(address & ~3 | 1) << 16;
    data |= internalReadByte(address & ~3 | 2) <<  8;
    data |= internalReadByte(address & ~3 | 3) <<  0;
    return data;
  }

  return busReadLong(address & ~3);
}

auto SH2::writeByte(u32 address, u32 data) -> void {
  //$c000:0000-$c000:0fff
  if((address & 0xffff'f000) == 0xc000'0000) {
    cache[address & 0xfff] = data;
    return;
  }

  //$ffff:fe00-$ffff:ffff
  if(address >= 0xffff'fe00) {
    return internalWriteByte(address, data);
  }

  return busWriteByte(address, data);
}

auto SH2::writeWord(u32 address, u32 data) -> void {
  //$c000:0000-$c000:0fff
  if((address & 0xffff'f000) == 0xc000'0000) {
    cache[address & 0xffe | 0] = data >> 8;
    cache[address & 0xffe | 1] = data >> 0;
    return;
  }

  //$ffff:fe00-$ffff:ffff
  if(address >= 0xffff'fe00) {
    internalWriteByte(address & ~1 | 0, data >> 8);
    internalWriteByte(address & ~1 | 1, data >> 0);
    return;
  }

  return busWriteWord(address & ~1, data);
}

auto SH2::writeLong(u32 address, u32 data) -> void {
  //$c000:0000-$c000:0fff
  if((address & 0xffff'f000) == 0xc000'0000) {
    cache[address & 0xffc | 0] = data >> 24;
    cache[address & 0xffc | 1] = data >> 16;
    cache[address & 0xffc | 2] = data >>  8;
    cache[address & 0xffc | 3] = data >>  0;
    return;
  }

  //$ffff:fe00-$ffff:ffff
  if(address >= 0xffff'fe00) {
    internalWriteByte(address & ~3 | 0, data >> 24);
    internalWriteByte(address & ~3 | 1, data >> 16);
    internalWriteByte(address & ~3 | 2, data >>  8);
    internalWriteByte(address & ~3 | 3, data >>  0);
    return;
  }

  return busWriteLong(address & ~3, data);
}

auto SH2::internalReadByte(u32 address, n8 data) -> n8 {
  switch(address) {

  //TIER: timer interrupt enable register
  case 0xffff'fe10:
    data.bit(0) = 0;
    data.bit(1) = frt.tier.ovie;
    data.bit(2) = frt.tier.ocie[1];
    data.bit(3) = frt.tier.ocie[0];
    data.bit(7) = frt.tier.icie;
    return data;

  //FTCSR: free-running timer control/status register
  case 0xffff'fe11:
    data.bit(0) = frt.ftcsr.cclra;
    data.bit(1) = frt.ftcsr.ovf;
    data.bit(2) = frt.ftcsr.ocf[1];
    data.bit(3) = frt.ftcsr.ocf[0];
    data.bit(7) = frt.ftcsr.icf;
    return data;

  //FRC: free-running counter
  case 0xffff'fe12: return frt.frc.byte(1);
  case 0xffff'fe13: return frt.frc.byte(0);

  //OCRA/OCRB: output compare registers A and B
  case 0xffff'fe14: return frt.ocr[frt.tocr.ocrs].byte(1);
  case 0xffff'fe15: return frt.ocr[frt.tocr.ocrs].byte(0);

  //TCR: timer control register
  case 0xffff'fe16:
    data.bit(0) = frt.tcr.cks.bit(0);
    data.bit(1) = frt.tcr.cks.bit(1);
    data.bit(7) = frt.tcr.iedg;
    return data;

  //TOCR: timer output compare control register
  case 0xffff'fe17:
    data.bit(0) = frt.tocr.olvl[1];
    data.bit(1) = frt.tocr.olvl[0];
    data.bit(4) = frt.tocr.ocrs;
    data.bit(5) = 1;
    data.bit(6) = 1;
    data.bit(7) = 1;
    return data;

  //FICR: input capture register
  case 0xffff'fe18: return frt.ficr.byte(1);
  case 0xffff'fe19: return frt.ficr.byte(0);

  //DRCR0: DMA request/response selection control register 0
  case 0xffff'fe71:
    data.bit(0,1) = dmac[0].drcr;
    return data;
  //DRCR1: DMA request/response selection control register 1
  case 0xffff'fe72:
    data.bit(0,1) = dmac[1].drcr;
    return data;

  case 0xffff'fe90: return 0;
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
    data.bit(0)   = ccr.ce;
    data.bit(1)   = ccr.id;
    data.bit(2)   = ccr.od;
    data.bit(3)   = ccr.tw;
    data.bit(4)   = ccr.cp;
    data.bit(6,7) = ccr.w;
    return data;
  case 0xffff'fe93: return 0;

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
  case 0xffff'ff08: return 0;
  case 0xffff'ff09: return 0;
  case 0xffff'ff0a: return 0;
  case 0xffff'ff0b:
    data.bit(0) = divu.dvcr.ovf;
    data.bit(1) = divu.dvcr.ovfie;
    return data;

  //VCRDIV: vector number setting register for division
  case 0xffff'ff0c: return 0;
  case 0xffff'ff0d: return 0;
  case 0xffff'ff0e: return 0;
  case 0xffff'ff0f:
    data.bit(0,6) = divu.vcrdiv;
    return data;

  //DVDNTH: dividend register H
  case 0xffff'ff10: return divu.dvdnth.byte(3);
  case 0xffff'ff11: return divu.dvdnth.byte(2);
  case 0xffff'ff12: return divu.dvdnth.byte(1);
  case 0xffff'ff13: return divu.dvdnth.byte(0);

  //DVDNTL: dividend register L
  case 0xffff'ff14: return divu.dvdntl.byte(3);
  case 0xffff'ff15: return divu.dvdntl.byte(2);
  case 0xffff'ff16: return divu.dvdntl.byte(1);
  case 0xffff'ff17: return divu.dvdntl.byte(0);

  //SAR0: DMA source address register 0
  case 0xffff'ff80: return dmac[0].sar.byte(3);
  case 0xffff'ff81: return dmac[0].sar.byte(2);
  case 0xffff'ff82: return dmac[0].sar.byte(1);
  case 0xffff'ff83: return dmac[0].sar.byte(0);

  //DAR0: DMA destination address register 0
  case 0xffff'ff84: return dmac[0].dar.byte(3);
  case 0xffff'ff85: return dmac[0].dar.byte(2);
  case 0xffff'ff86: return dmac[0].dar.byte(1);
  case 0xffff'ff87: return dmac[0].dar.byte(0);

  //TCR0: DMA transfer count register 0
  case 0xffff'ff88: return 0;
  case 0xffff'ff89: return dmac[0].tcr.byte(2);
  case 0xffff'ff8a: return dmac[0].tcr.byte(1);
  case 0xffff'ff8b: return dmac[0].tcr.byte(0);

  //CHCR0: DMA channel control register 0
  case 0xffff'ff8c: return 0;
  case 0xffff'ff8d: return 0;
  case 0xffff'ff8e:
    data.bit(0) = dmac[0].chcr.am;
    data.bit(1) = dmac[0].chcr.ar;
    data.bit(2) = dmac[0].chcr.ts.bit(0);
    data.bit(3) = dmac[0].chcr.ts.bit(1);
    data.bit(4) = dmac[0].chcr.sm.bit(0);
    data.bit(5) = dmac[0].chcr.sm.bit(1);
    data.bit(6) = dmac[0].chcr.dm.bit(0);
    data.bit(7) = dmac[0].chcr.dm.bit(1);
    return data;
  case 0xffff'ff8f:
    data.bit(0) = dmac[0].chcr.de;
    data.bit(1) = dmac[0].chcr.te;
    data.bit(2) = dmac[0].chcr.ie;
    data.bit(3) = dmac[0].chcr.ta;
    data.bit(4) = dmac[0].chcr.tb;
    data.bit(5) = dmac[0].chcr.dl;
    data.bit(6) = dmac[0].chcr.ds;
    data.bit(7) = dmac[0].chcr.al;

  //SAR1: DMA source address register 0
  case 0xffff'ff90: return dmac[1].sar.byte(3);
  case 0xffff'ff91: return dmac[1].sar.byte(2);
  case 0xffff'ff92: return dmac[1].sar.byte(1);
  case 0xffff'ff93: return dmac[1].sar.byte(0);

  //DAR1: DMA destination address register 0
  case 0xffff'ff94: return dmac[1].dar.byte(3);
  case 0xffff'ff95: return dmac[1].dar.byte(2);
  case 0xffff'ff96: return dmac[1].dar.byte(1);
  case 0xffff'ff97: return dmac[1].dar.byte(0);

  //TCR1: DMA transfer count register 0
  case 0xffff'ff98: return 0;
  case 0xffff'ff99: return dmac[1].tcr.byte(2);
  case 0xffff'ff9a: return dmac[1].tcr.byte(1);
  case 0xffff'ff9b: return dmac[1].tcr.byte(0);

  //CHCR1: DMA channel control register 1
  case 0xffff'ff9c: return 0;
  case 0xffff'ff9d: return 0;
  case 0xffff'ff9e:
    data.bit(0) = dmac[1].chcr.am;
    data.bit(1) = dmac[1].chcr.ar;
    data.bit(2) = dmac[1].chcr.ts.bit(0);
    data.bit(3) = dmac[1].chcr.ts.bit(1);
    data.bit(4) = dmac[1].chcr.sm.bit(0);
    data.bit(5) = dmac[1].chcr.sm.bit(1);
    data.bit(6) = dmac[1].chcr.dm.bit(0);
    data.bit(7) = dmac[1].chcr.dm.bit(1);
    return data;
  case 0xffff'ff9f:
    data.bit(0) = dmac[1].chcr.de;
    data.bit(1) = dmac[1].chcr.te;
    data.bit(2) = dmac[1].chcr.ie;
    data.bit(3) = dmac[1].chcr.ta;
    data.bit(4) = dmac[1].chcr.tb;
    data.bit(5) = dmac[1].chcr.dl;
    data.bit(6) = dmac[1].chcr.ds;
    data.bit(7) = dmac[1].chcr.al;
    return data;

  //VCRDMA0: DMA vector number register 0
  case 0xffff'ffa0: return 0;
  case 0xffff'ffa1: return 0;
  case 0xffff'ffa2: return 0;
  case 0xffff'ffa3: return dmac[0].vcrdma;

  //VCRDMA1: DMA vector number register 1
  case 0xffff'ffa8: return dmac[1].vcrdma;
  case 0xffff'ffa9: return 0;
  case 0xffff'ffaa: return 0;
  case 0xffff'ffab: return 0;

  //DMAOR: DMA operation register
  case 0xffff'ffb0: return 0;
  case 0xffff'ffb1: return 0;
  case 0xffff'ffb2: return 0;
  case 0xffff'ffb3:
    data.bit(0) = dmaor.dme;
    data.bit(1) = dmaor.nmif;
    data.bit(2) = dmaor.ae;
    data.bit(3) = dmaor.pr;
    return data;

  //BCR1: bus control register 1
  case 0xffff'ffe0: return 0;
  case 0xffff'ffe1: return 0;
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
  case 0xffff'ffe4: return 0;
  case 0xffff'ffe5: return 0;
  case 0xffff'ffe6: return 0;
  case 0xffff'ffe7:
    data.bit(2,3) = bsc.bcr2.a1sz;
    data.bit(4,5) = bsc.bcr2.a2sz;
    data.bit(6,7) = bsc.bcr2.a3sz;
    return data;

  //WCR: wait control register 
  case 0xffff'ffe8: return 0;
  case 0xffff'ffe9: return 0;
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
  case 0xffff'ffec: return 0;
  case 0xffff'ffed: return 0;
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
  case 0xffff'fff0: return 0;
  case 0xffff'fff1: return 0;
  case 0xffff'fff2: return 0;
  case 0xffff'fff3:
    data.bit(3,5) = bsc.rtcsr.cks;
    data.bit(6)   = bsc.rtcsr.cmie;
    data.bit(7)   = bsc.rtcsr.cmf;
    return data;

  //RTCNT: refresh timer counter
  case 0xffff'fff4: return 0;
  case 0xffff'fff5: return 0;
  case 0xffff'fff6: return 0;
  case 0xffff'fff7:
    data.bit(0,7) = bsc.rtcnt;
    return data;

  //RTCOR: refresh time constant register
  case 0xffff'fff8: return 0;
  case 0xffff'fff9: return 0;
  case 0xffff'fffa: return 0;
  case 0xffff'fffb:
    data.bit(0,7) = bsc.rtcor;
    return data;

  }

  debug(unimplemented, "[SH2] read(0x", hex(address, 8L), ")");
  return data;
}

auto SH2::internalWriteByte(u32 address, n8 data) -> void {
  switch(address) {

  //TIER: timer interrupt enable register
  case 0xffff'fe10:
    frt.tier.ovie    = data.bit(1);
    frt.tier.ocie[1] = data.bit(2);
    frt.tier.ocie[0] = data.bit(3);
    frt.tier.icie    = data.bit(7);
    return;

  //FTCSR: free-running timer control/status register
  case 0xffff'fe11:
    frt.ftcsr.cclra   = data.bit(0);
    frt.ftcsr.ovf    &= data.bit(1);
    frt.ftcsr.ocf[1] &= data.bit(2);
    frt.ftcsr.ocf[0] &= data.bit(3);
    frt.ftcsr.icf    &= data.bit(7);
    return;

  //FRC: free-running counter
  case 0xffff'fe12: frt.frc.byte(1) = data; return;
  case 0xffff'fe13: frt.frc.byte(0) = data; return;

  //OCRA/OCRB: output compare registers A and B
  case 0xffff'fe14: frt.ocr[frt.tocr.ocrs].byte(1) = data; return;
  case 0xffff'fe15: frt.ocr[frt.tocr.ocrs].byte(0) = data; return;

  //TCR: timer control register
  case 0xffff'fe16:
    frt.tcr.cks  = data.bit(0,1);
    frt.tcr.iedg = data.bit(2);
    return;

  //TOCR: timer output compare control register
  case 0xffff'fe17:
    frt.tocr.olvl[1] = data.bit(0);
    frt.tocr.olvl[0] = data.bit(1);
    frt.tocr.ocrs    = data.bit(4);
    return;

  //FICR: input capture register (read-only)
  case 0xffff'fe18: return;
  case 0xffff'fe19: return;

  //DRCR0: DMA request/response selection control register 0
  case 0xffff'fe71:
    dmac[0].drcr = data.bit(0,1);
    return;
  //DRCR1: DMA request/response selection control register 1
  case 0xffff'fe72:
    dmac[1].drcr = data.bit(0,1);
    return;

  case 0xffff'fe90: return;
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
    ccr.ce = data.bit(0);
    ccr.id = data.bit(1);
    ccr.od = data.bit(2);
    ccr.tw = data.bit(3);
    ccr.cp = data.bit(4);
    ccr.w  = data.bit(6,7);
    return;
  case 0xffff'fe93: return;

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
      s32 dividend = (s32)divu.dvdntl;
      divu.dvdntl = dividend / (s32)divu.dvsr;
      divu.dvdnth = dividend % (s32)divu.dvsr;
    } else {
      divu.dvdntl = 0x8000'0000;
      divu.dvdnth = 0x7fff'ffff;
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
  case 0xffff'ff10: divu.dvdnth.byte(3) = data; return;
  case 0xffff'ff11: divu.dvdnth.byte(2) = data; return;
  case 0xffff'ff12: divu.dvdnth.byte(1) = data; return;
  case 0xffff'ff13: divu.dvdnth.byte(0) = data; return;

  //DVDNTL: dividend register L
  case 0xffff'ff14: divu.dvdntl.byte(3) = data; return;
  case 0xffff'ff15: divu.dvdntl.byte(2) = data; return;
  case 0xffff'ff16: divu.dvdntl.byte(1) = data; return;
  case 0xffff'ff17: divu.dvdntl.byte(0) = data; {
    if(divu.dvsr) {
      s64 dividend = (s64)divu.dvdnth << 32 | (s64)divu.dvdntl << 0;
      divu.dvdntl = dividend / (s32)divu.dvsr;
      divu.dvdnth = dividend % (s32)divu.dvsr;
    } else {
      divu.dvdntl = 0x8000'0000;
      divu.dvdnth = 0x7fff'ffff;
      divu.dvcr.ovf = 1;
    }
    return;
  }

  //SAR0: DMA source address register 0
  case 0xffff'ff80: dmac[0].sar.byte(3) = data; return;
  case 0xffff'ff81: dmac[0].sar.byte(2) = data; return;
  case 0xffff'ff82: dmac[0].sar.byte(1) = data; return;
  case 0xffff'ff83: dmac[0].sar.byte(0) = data; return;

  //DAR0: DMA destination address register 0
  case 0xffff'ff84: dmac[0].dar.byte(3) = data; return;
  case 0xffff'ff85: dmac[0].dar.byte(2) = data; return;
  case 0xffff'ff86: dmac[0].dar.byte(1) = data; return;
  case 0xffff'ff87: dmac[0].dar.byte(0) = data; return;

  //TCR0: DMA transfer count register 0
  case 0xffff'ff88: return;
  case 0xffff'ff89: dmac[0].tcr.byte(2) = data; return;
  case 0xffff'ff8a: dmac[0].tcr.byte(1) = data; return;
  case 0xffff'ff8b: dmac[0].tcr.byte(0) = data; return;

  //CHCR0: DMA channel control register 0
  case 0xffff'ff8c: return;
  case 0xffff'ff8d: return;
  case 0xffff'ff8e:
    dmac[0].chcr.am  = data.bit(0);
    dmac[0].chcr.ar  = data.bit(1);
    dmac[0].chcr.ts  = data.bit(2,3);
    dmac[0].chcr.sm  = data.bit(4,5);
    dmac[0].chcr.dm  = data.bit(6,7);
    return;
  case 0xffff'ff8f:
    dmac[0].chcr.de  = data.bit(0);
    dmac[0].chcr.te &= data.bit(1);
    dmac[0].chcr.ie  = data.bit(2);
    dmac[0].chcr.ta  = data.bit(3);
    dmac[0].chcr.tb  = data.bit(4);
    dmac[0].chcr.dl  = data.bit(5);
    dmac[0].chcr.ds  = data.bit(6);
    dmac[0].chcr.al  = data.bit(7);
    return;

  //SAR1: DMA source address register 1
  case 0xffff'ff90: dmac[1].sar.byte(3) = data; return;
  case 0xffff'ff91: dmac[1].sar.byte(2) = data; return;
  case 0xffff'ff92: dmac[1].sar.byte(1) = data; return;
  case 0xffff'ff93: dmac[1].sar.byte(0) = data; return;

  //DAR1: DMA destination address register 1
  case 0xffff'ff94: dmac[1].dar.byte(3) = data; return;
  case 0xffff'ff95: dmac[1].dar.byte(2) = data; return;
  case 0xffff'ff96: dmac[1].dar.byte(1) = data; return;
  case 0xffff'ff97: dmac[1].dar.byte(0) = data; return;

  //TCR1: DMA transfer count register 0
  case 0xffff'ff98: return;
  case 0xffff'ff99: dmac[1].tcr.byte(2) = data; return;
  case 0xffff'ff9a: dmac[1].tcr.byte(1) = data; return;
  case 0xffff'ff9b: dmac[1].tcr.byte(0) = data; return;

  //CHCR1: DMA channel control register 1
  case 0xffff'ff9c: return;
  case 0xffff'ff9d: return;
  case 0xffff'ff9e:
    dmac[1].chcr.am  = data.bit(0);
    dmac[1].chcr.ar  = data.bit(1);
    dmac[1].chcr.ts  = data.bit(2,3);
    dmac[1].chcr.sm  = data.bit(4,5);
    dmac[1].chcr.dm  = data.bit(6,7);
    return;
  case 0xffff'ff9f:
    dmac[1].chcr.de  = data.bit(0);
    dmac[1].chcr.te &= data.bit(1);
    dmac[1].chcr.ie  = data.bit(2);
    dmac[1].chcr.ta  = data.bit(3);
    dmac[1].chcr.tb  = data.bit(4);
    dmac[1].chcr.dl  = data.bit(5);
    dmac[1].chcr.ds  = data.bit(6);
    dmac[1].chcr.al  = data.bit(7);
    return;

  //VCRDMA0: DMA vector number register 0
  case 0xffff'ffa0: return;
  case 0xffff'ffa1: return;
  case 0xffff'ffa2: return;
  case 0xffff'ffa3: dmac[0].vcrdma = data; return;

  //VCRDMA1: DMA vector number register 1
  case 0xffff'ffa8: return;
  case 0xffff'ffa9: return;
  case 0xffff'ffaa: return;
  case 0xffff'ffab: dmac[1].vcrdma = data; return;

  //DMAOR: DMA operation register
  case 0xffff'ffb0: return;
  case 0xffff'ffb1: return;
  case 0xffff'ffb2: return;
  case 0xffff'ffb3:
    dmaor.dme   = data.bit(0);
    dmaor.nmif &= data.bit(1);
    dmaor.ae   &= data.bit(2);
    dmaor.pr    = data.bit(3);
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

  debug(unimplemented, "[SH2] write(0x", hex(address, 8L), ", 0x", hex(data, 2L), ")");
}
