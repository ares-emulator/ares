auto SA1::readIOCPU(uint24 address, uint8 data) -> uint8 {
  cpu.synchronize(sa1);

  switch(0x2200 | address.bit(0,8)) {

  //(SFR) S-CPU flag read
  case 0x2300:
    data.bit(0,3) = io.cmeg;
    data.bit(4)   = io.cpu_nvsw;
    data.bit(5)   = io.chdma_irqfl;
    data.bit(6)   = io.cpu_ivsw;
    data.bit(7)   = io.cpu_irqfl;
    return data;

  //(VC) version code register
  case 0x230e:
    return data;  //does not actually exist on real hardware ... always returns open bus

  }

  return data;
}

auto SA1::readIOSA1(uint24 address, uint8 data) -> uint8 {
  synchronize(cpu);

  switch(0x2200 | address.bit(0,8)) {

  //(CFR) SA-1 flag read
  case 0x2301:
    data.bit(0,3) = io.smeg;
    data.bit(4)   = io.sa1_nmifl;
    data.bit(5)   = io.dma_irqfl;
    data.bit(6)   = io.timer_irqfl;
    data.bit(7)   = io.sa1_irqfl;
    return data;

  //(HCR) hcounter read
  case 0x2302:
    //latch counters
    io.hcr = status.hcounter >> 2;
    io.vcr = status.vcounter;
    return io.hcr.byte(0);

  case 0x2303:
    return io.hcr.byte(1);

  //(VCR) vcounter read
  case 0x2304: return io.vcr.byte(0);
  case 0x2305: return io.vcr.byte(1);

  //(MR) arithmetic result
  case 0x2306: return io.mr.byte(0);
  case 0x2307: return io.mr.byte(1);
  case 0x2308: return io.mr.byte(2);
  case 0x2309: return io.mr.byte(3);
  case 0x230a: return io.mr.byte(4);

  //(OF) arithmetic overflow flag
  case 0x230b: return io.overflow << 7;

  //(VDPL) variable-length data read port low
  case 0x230c: {
    uint24 data;
    data.byte(0) = readVBR(io.va + 0);
    data.byte(1) = readVBR(io.va + 1);
    data.byte(2) = readVBR(io.va + 2);
    data >>= io.vbit;

    return data.byte(0);
  }

  //(VDPH) variable-length data read port high
  case 0x230d: {
    uint24 data;
    data.byte(0) = readVBR(io.va + 0);
    data.byte(1) = readVBR(io.va + 1);
    data.byte(2) = readVBR(io.va + 2);
    data >>= io.vbit;

    if(io.hl == 1) {
      //auto-increment mode
      io.vbit += io.vb;
      io.va += io.vbit >> 3;
      io.vbit &= 7;
    }

    return data.byte(1);
  }

  }

  return 0xff;  //unverified
}

auto SA1::writeIOCPU(uint24 address, uint8 data) -> void {
  cpu.synchronize(sa1);

  switch(0x2200 | address.bit(0,8)) {

  //(CCNT) SA-1 control
  case 0x2200:
    if(io.sa1_resb && !data.bit(5)) {
      //reset SA-1 CPU (PC bank set to 0x00)
      r.pc.d = io.crv;
    }

    io.smeg     = data.bit(0,3);
    io.sa1_nmi  = data.bit(4);
    io.sa1_resb = data.bit(5);
    io.sa1_rdyb = data.bit(6);
    io.sa1_irq  = data.bit(7);

    if(io.sa1_irq) {
      io.sa1_irqfl = true;
      if(io.sa1_irqen) io.sa1_irqcl = 0;
    }

    if(io.sa1_nmi) {
      io.sa1_nmifl = true;
      if(io.sa1_nmien) io.sa1_nmicl = 0;
    }

    return;

  //(SIE) S-CPU interrupt enable
  case 0x2201:
    if(!io.chdma_irqen && data.bit(5)) {
      if(io.chdma_irqfl) {
        io.chdma_irqcl = 0;
        cpu.irq(1);
      }
    }

    if(!io.cpu_irqen && data.bit(7)) {
      if(io.cpu_irqfl) {
        io.cpu_irqcl = 0;
        cpu.irq(1);
      }
    }

    io.chdma_irqen = data.bit(5);
    io.cpu_irqen   = data.bit(7);
    return;

  //(SIC) S-CPU interrupt clear
  case 0x2202:
    io.chdma_irqcl = data.bit(5);
    io.cpu_irqcl   = data.bit(7);

    if(io.chdma_irqcl) io.chdma_irqfl = 0;
    if(io.cpu_irqcl  ) io.cpu_irqfl   = 0;

    if(!io.chdma_irqfl && !io.cpu_irqfl) cpu.irq(0);
    return;

  //(CRV) SA-1 reset vector
  case 0x2203: io.crv.byte(0) = data; return;
  case 0x2204: io.crv.byte(1) = data; return;

  //(CNV) SA-1 NMI vector
  case 0x2205: io.cnv.byte(0) = data; return;
  case 0x2206: io.cnv.byte(1) = data; return;

  //(CIV) SA-1 IRQ vector
  case 0x2207: io.civ.byte(0) = data; return;
  case 0x2208: io.civ.byte(1) = data; return;

  //(CXB) Super MMC bank C
  case 0x2220:
    io.cb     = data.bit(0,2);
    io.cbmode = data.bit(7);
    return;

  //(DXB) Super MMC bank D
  case 0x2221:
    io.db     = data.bit(0,2);
    io.dbmode = data.bit(7);
    return;

  //(EXB) Super MMC bank E
  case 0x2222:
    io.eb     = data.bit(0,2);
    io.ebmode = data.bit(7);
    return;

  //(FXB) Super MMC bank F
  case 0x2223:
    io.fb     = data.bit(0,2);
    io.fbmode = data.bit(7);
    return;

  //(BMAPS) S-CPU BW-RAM address mapping
  case 0x2224:
    io.sbm = data.bit(0,4);
    return;

  //(SWBE) S-CPU BW-RAM write enable
  case 0x2226:
    io.swen = data.bit(7);
    return;

  //(BWPA) BW-RAM write-protected area
  case 0x2228:
    io.bwp = data.bit(0,3);
    return;

  //(SIWP) S-CPU I-RAM write protection
  case 0x2229:
    io.siwp = data;
    return;

  case 0x2231: case 0x2232: case 0x2233: case 0x2234: case 0x2235: case 0x2236: case 0x2237:
    return writeIOShared(address, data);

  }
}

auto SA1::writeIOSA1(uint24 address, uint8 data) -> void {
  synchronize(cpu);

  switch(0x2200 | address.bit(0,8)) {

  //(SCNT) S-CPU control
  case 0x2209:
    io.cmeg     = data.bit(0,3);
    io.cpu_nvsw = data.bit(4);
    io.cpu_ivsw = data.bit(6);
    io.cpu_irq  = data.bit(7);

    if(io.cpu_irq) {
      io.cpu_irqfl = 1;
      if(io.cpu_irqen) {
        io.cpu_irqcl = 0;
        cpu.irq(1);
      }
    }
    return;

  //(CIE) SA-1 interrupt enable
  case 0x220a:
    if(!io.sa1_nmien   && data.bit(4) && io.sa1_nmifl  ) io.sa1_nmicl   = 0;
    if(!io.dma_irqen   && data.bit(5) && io.dma_irqfl  ) io.dma_irqcl   = 0;
    if(!io.timer_irqen && data.bit(6) && io.timer_irqfl) io.timer_irqcl = 0;
    if(!io.sa1_irqen   && data.bit(7) && io.sa1_irqfl  ) io.sa1_irqcl   = 0;

    io.sa1_nmien   = data.bit(4);
    io.dma_irqen   = data.bit(5);
    io.timer_irqen = data.bit(6);
    io.sa1_irqen   = data.bit(7);
    return;

  //(CIC) SA-1 interrupt clear
  case 0x220b:
    io.sa1_nmicl   = data.bit(4);
    io.dma_irqcl   = data.bit(5);
    io.timer_irqcl = data.bit(6);
    io.sa1_irqcl   = data.bit(7);

    if(io.sa1_nmicl)   io.sa1_nmifl   = 0;
    if(io.sa1_irqcl)   io.sa1_irqfl   = 0;
    if(io.timer_irqcl) io.timer_irqfl = 0;
    if(io.dma_irqcl)   io.dma_irqfl   = 0;
    return;

  //(SNV) S-CPU NMI vector
  case 0x220c: io.snv.byte(0) = data; return;
  case 0x220d: io.snv.byte(1) = data; return;

  //(SIV) S-CPU IRQ vector
  case 0x220e: io.siv.byte(0) = data; return;
  case 0x220f: io.siv.byte(1) = data; return;

  //(TMC) H/V timer control
  case 0x2210:
    io.hen    = data.bit(0);
    io.ven    = data.bit(1);
    io.hvselb = data.bit(7);
    return;

  //(CTR) SA-1 timer restart
  case 0x2211:
    status.vcounter = 0;
    status.hcounter = 0;
    return;

  //(HCNT) H-count
  case 0x2212: io.hcnt.byte(0) = data; return;
  case 0x2213: io.hcnt.byte(1) = data; return;

  //(VCNT) V-count
  case 0x2214: io.vcnt.byte(0) = data; return;
  case 0x2215: io.vcnt.byte(1) = data; return;

  //(BMAP) SA-1 BW-RAM address mapping
  case 0x2225:
    io.cbm  = data.bit(0,6);
    io.sw46 = data.bit(7);
    return;

  //(CWBE) SA-1 BW-RAM write enable
  case 0x2227:
    io.cwen = data.bit(7);
    return;

  //(CIWP) SA-1 I-RAM write protection
  case 0x222a:
    io.ciwp = data;
    return;

  //(DCNT) DMA control
  case 0x2230:
    io.sd    = data.bit(0,1);
    io.dd    = data.bit(2);
    io.cdsel = data.bit(4);
    io.cden  = data.bit(5);
    io.dprio = data.bit(6);
    io.dmaen = data.bit(7);
    if(io.dmaen == 0) dma.line = 0;
    return;

  case 0x2231: case 0x2232: case 0x2233: case 0x2234: case 0x2235: case 0x2236: case 0x2237:
    return writeIOShared(address, data);

  //(DTC) DMA terminal counter
  case 0x2238: io.dtc.byte(0) = data; return;
  case 0x2239: io.dtc.byte(1) = data; return;

  //(BBF) BW-RAM bitmap format
  case 0x223f: io.bbf = data.bit(7); return;

  //(BRF) bitmap register files
  case 0x2240: io.brf[ 0] = data; return;
  case 0x2241: io.brf[ 1] = data; return;
  case 0x2242: io.brf[ 2] = data; return;
  case 0x2243: io.brf[ 3] = data; return;
  case 0x2244: io.brf[ 4] = data; return;
  case 0x2245: io.brf[ 5] = data; return;
  case 0x2246: io.brf[ 6] = data; return;
  case 0x2247: io.brf[ 7] = data;
    if(io.dmaen) {
      if(io.cden == 1 && io.cdsel == 0) {
        dmaCC2();
      }
    }
    return;
  case 0x2248: io.brf[ 8] = data; return;
  case 0x2249: io.brf[ 9] = data; return;
  case 0x224a: io.brf[10] = data; return;
  case 0x224b: io.brf[11] = data; return;
  case 0x224c: io.brf[12] = data; return;
  case 0x224d: io.brf[13] = data; return;
  case 0x224e: io.brf[14] = data; return;
  case 0x224f: io.brf[15] = data;
    if(io.dmaen) {
      if(io.cden == 1 && io.cdsel == 0) {
        dmaCC2();
      }
    }
    return;

  //(MCNT) arithmetic control
  case 0x2250:
    io.md  = data.bit(0);
    io.acm = data.bit(1);
    if(io.acm) io.mr = 0;
    return;

  //(MAL) multiplicand / dividend low
  case 0x2251:
    io.ma.byte(0) = data;
    return;

  //(MAH) multiplicand / dividend high
  case 0x2252:
    io.ma.byte(1) = data;
    return;

  //(MBL) multiplier / divisor low
  case 0x2253:
    io.mb.byte(0) = data;
    return;

  //(MBH) multiplier / divisor high
  //multiplication / cumulative sum only resets MB
  //division resets both MA and MB
  case 0x2254:
    io.mb.byte(1) = data;

    if(io.acm == 0) {
      if(io.md == 0) {
        //signed multiplication
        io.mr = uint32((int16)io.ma * (int16)io.mb);
        io.mb = 0;
      } else {
        //unsigned division
        if(io.mb == 0) {
          io.mr = 0;
        } else {
          int16 dividend = io.ma;
          uint16 divisor = io.mb;
          uint16 remainder = dividend >= 0 ? uint16(dividend % divisor) : uint16((dividend % divisor + divisor) % divisor);
          uint16 quotient = (dividend - remainder) / divisor;
          io.mr = remainder << 16 | quotient;
        }
        io.ma = 0;
        io.mb = 0;
      }
    } else {
      //sigma (accumulative multiplication)
      io.mr += (int16)io.ma * (int16)io.mb;
      io.overflow = io.mr >> 40;
      io.mr = (uint40)io.mr;
      io.mb = 0;
    }
    return;

  //(VBD) variable-length bit processing
  case 0x2258:
    io.vb = data.bit(0,3);
    io.hl = data.bit(7);

    if(io.vb == 0) io.vb = 16;

    if(io.hl == 0) {
      //fixed mode
      io.vbit += io.vb;
      io.va += io.vbit >> 3;
      io.vbit &= 7;
    }
    return;

  //(VDA) variable-length bit game pak ROM start address
  case 0x2259: io.va.byte(0) = data; return;
  case 0x225a: io.va.byte(1) = data; return;
  case 0x225b: io.va.byte(2) = data; io.vbit = 0; return;

  }
}

auto SA1::writeIOShared(uint24 address, uint8 data) -> void {
  switch(0x2200 | address.bit(0,8)) {

  //(CDMA) character conversion DMA parameters
  case 0x2231:
    io.dmacb   = data.bit(0,1);
    io.dmasize = data.bit(2,4);;
    io.chdend  = data.bit(7);

    if(io.dmacb   > 2) io.dmacb   = 2;
    if(io.dmasize > 5) io.dmasize = 5;
    if(io.chdend == 1) bwram.dma  = 0;
    return;

  //(SDA) DMA source device start address
  case 0x2232: io.dsa.byte(0) = data; return;
  case 0x2233: io.dsa.byte(1) = data; return;
  case 0x2234: io.dsa.byte(2) = data; return;

  //(DDA) DMA destination start address
  case 0x2235: io.dda.byte(0) = data; return;
  case 0x2236: io.dda.byte(1) = data;
    if(io.dmaen) {
      if(io.cden == 0 && io.dd == DMA::DestIRAM) {
        dmaNormal();
      } else if(io.cden == 1 && io.cdsel == 1) {
        dmaCC1();
      }
    }
    return;
  case 0x2237: io.dda.byte(2) = data;
    if(io.dmaen) {
      if(io.cden == 0 && io.dd == DMA::DestBWRAM) {
        dmaNormal();
      }
    }
    return;

  }
}
