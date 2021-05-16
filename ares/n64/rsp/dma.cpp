auto RSP::dmaRead() -> void {
  auto target = dma.memSource == 0 ? 0x0400'0000 : 0x0400'1000;
  auto length = (dma.read.length | 7) + 1;
  for(u32 count : range(dma.read.count + 1)) {
    for(u32 offset = 0; offset < length; offset += 4) {
      u32 data = bus.read<Word>(dma.dramAddress + offset);
      bus.write<Word>(dma.memAddress + target + offset, data);
    }
    dma.dramAddress += length + dma.read.skip;
    dma.memAddress += length;
  }
  dma.busy = 0;
}

auto RSP::dmaWrite() -> void {
  auto source = dma.memSource == 0 ? 0x0400'0000 : 0x0400'1000;
  auto length = (dma.write.length | 7) + 1;
  for(u32 count : range(dma.write.count + 1)) {
    for(u32 offset = 0; offset < length; offset += 4) {
      u32 data = bus.read<Word>(dma.memAddress + source + offset);
      bus.write<Word>(dma.dramAddress + offset, data);
    }
    dma.dramAddress += length + dma.write.skip;
    dma.memAddress += length;
  }
  dma.busy = 0;
}
