//direct data transfer
auto SA1::dmaNormal() -> void {
  while(io.dtc--) {
    n8  data = r.mdr;
    n24 source = io.dsa++;
    n24 target = io.dda++;

    if(io.sd == DMA::SourceROM && io.dd == DMA::DestBWRAM) {
      step();
      step();
      if(bwram.conflict()) step();
      if(bwram.conflict()) step();
      data = rom.readSA1(source, data);
      bwram.write(target, data);
    }

    if(io.sd == DMA::SourceROM && io.dd == DMA::DestIRAM) {
      step();
      if(iram.conflict() || rom.conflict()) step();
      if(iram.conflict()) step();
      data = rom.readSA1(source, data);
      iram.write(target, data);
    }

    if(io.sd == DMA::SourceBWRAM && io.dd == DMA::DestIRAM) {
      step();
      step();
      if(bwram.conflict() || iram.conflict()) step();
      if(bwram.conflict()) step();
      data = bwram.read(source, data);
      iram.write(target, data);
    }

    if(io.sd == DMA::SourceIRAM && io.dd == DMA::DestBWRAM) {
      step();
      step();
      if(bwram.conflict() || iram.conflict()) step();
      if(bwram.conflict()) step();
      data = iram.read(source, data);
      bwram.write(target, data);
    }
  }

  io.dma_irqfl = true;
  if(io.dma_irqen) io.dma_irqcl = 0;
}

//type-1 character conversion
auto SA1::dmaCC1() -> void {
  bwram.dma = true;
  io.chdma_irqfl = true;
  if(io.chdma_irqen) {
    io.chdma_irqcl = 0;
    cpu.irq(1);
  }
}

//((byte & 6) << 3) + (byte & 1) explanation:
//transforms a byte index (0-7) into a planar index:
//result[] = {0, 1, 16, 17, 32, 33, 48, 49};
//works for 2bpp, 4bpp and 8bpp modes

//type-1 character conversion
auto SA1::dmaCC1Read(n24 address) -> n8 {
  //16 bytes/char (2bpp); 32 bytes/char (4bpp); 64 bytes/char (8bpp)
  u32 charmask = (1 << (6 - io.dmacb)) - 1;

  if((address & charmask) == 0) {
    //buffer next character to I-RAM
    u32 bpp = 2 << (2 - io.dmacb);
    u32 bpl = (8 << io.dmasize) >> io.dmacb;
    u32 bwmask = bwram.size() - 1;
    u32 tile = ((address - io.dsa) & bwmask) >> (6 - io.dmacb);
    u32 ty = (tile >> io.dmasize);
    u32 tx = tile & ((1 << io.dmasize) - 1);
    u32 bwaddr = io.dsa + ty * 8 * bpl + tx * bpp;

    for(u32 y : range(8)) {
      u64 data = 0;
      for(u32 byte : range(bpp)) {
        data |= (u64)bwram.read((bwaddr + byte) & bwmask) << (byte << 3);
      }
      bwaddr += bpl;

      u8 out[] = {0, 0, 0, 0, 0, 0, 0, 0};
      for(u32 x : range(8)) {
        out[0] |= (data & 1) << 7 - x; data >>= 1;
        out[1] |= (data & 1) << 7 - x; data >>= 1;
        if(io.dmacb == 2) continue;
        out[2] |= (data & 1) << 7 - x; data >>= 1;
        out[3] |= (data & 1) << 7 - x; data >>= 1;
        if(io.dmacb == 1) continue;
        out[4] |= (data & 1) << 7 - x; data >>= 1;
        out[5] |= (data & 1) << 7 - x; data >>= 1;
        out[6] |= (data & 1) << 7 - x; data >>= 1;
        out[7] |= (data & 1) << 7 - x; data >>= 1;
      }

      for(u32 byte : range(bpp)) {
        u32 p = io.dda + (y << 1) + ((byte & 6) << 3) + (byte & 1);
        iram.write(p & 0x07ff, out[byte]);
      }
    }
  }

  return iram.read((io.dda + (address & charmask)) & 0x07ff);
}

//type-2 character conversion
auto SA1::dmaCC2() -> void {
  //select register file index (0-7 or 8-15)
  const n8* brf = &io.brf[(dma.line & 1) << 3];
  u32 bpp = 2 << (2 - io.dmacb);
  u32 address = io.dda & 0x07ff;
  address &= ~((1 << (7 - io.dmacb)) - 1);
  address += (dma.line & 8) * bpp;
  address += (dma.line & 7) * 2;

  for(u32 byte : range(bpp)) {
    u8 output = 0;
    for(u32 bit : range(8)) {
      output |= ((brf[bit] >> byte) & 1) << (7 - bit);
    }
    iram.write(address + ((byte & 6) << 3) + (byte & 1), output);
  }

  dma.line = (dma.line + 1) & 15;
}
