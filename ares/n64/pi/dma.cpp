auto PI::dmaRead() -> void {
  io.readLength = (io.readLength | 1) + 1;

  u32 lastCacheline = 0xffff'ffff;
  for(u32 address = 0; address < io.readLength; address += 2) {
    u16 data = rdram.ram.read<Half>(io.dramAddress + address, "PI DMA");
    busWrite<Half>(io.pbusAddress + address, data);
  }
}

auto PI::dmaWriteDuration() -> u32 {
  BSD bsd;
  switch (io.pbusAddress.bit(24,31)) {
    case 0x05:               bsd = bsd2; break; 
    case range8(0x08, 0x0F): bsd = bsd2; break;
    default:                 bsd = bsd1; break;
  }

  int blockLen = (pi.runningDMA.blockLen() + 1) & ~1;
  int rdramLen = blockLen - pi.runningDMA.misalign;
  int pageShift = bsd.pageSize + 2;
  int pageSize = 1 << pageShift;

  int cyclesInitial = 0;
  if(pi.runningDMA.firstBlock)
    cyclesInitial = 4 * 3; // Initial setup time

  // PI page selection time
  int cyclePiSel = 0;
  int firstPage = (io.pbusAddress >> pageShift);
  int lastPage = (io.pbusAddress + blockLen - 1) >> pageShift;
  if (pi.runningDMA.firstBlock || firstPage != lastPage)
    cyclePiSel = (14 + bsd.latency + 1) * 3;

  // PI transfer time for the block
  int cyclesPiTx = ((bsd.pulseWidth + 1 + bsd.releaseDuration + 1) * blockLen/2) * 3;

  // RDRAM row opening time
  int cyclesRdramRow = 0;
  int cyclesRdramMasking = 0;
  int cyclesRdramSetup = 0;
  int cyclesRdramTx = 0;

  // RDRAM masking
  if(pi.runningDMA.misalign || ((io.dramAddress + blockLen) & 7) != 0)
    cyclesRdramMasking = 21 * 3;

  if(rdramLen > 0) {
    // RDRAM row opening
    if((io.dramAddress & 0x7ff) == 0 && !pi.runningDMA.firstBlock)
      cyclesRdramRow = 6 * 3; 

    // RDRAM burst setup
    cyclesRdramSetup = 6 * 3;

    // RDRAM writeback time at 350 MiB/s
    cyclesRdramTx = (i64)rdramLen * (62500000 * 3) / (350 * 1024 * 1024);
  }

  return cyclesInitial + cyclePiSel + cyclesPiTx + cyclesRdramRow + cyclesRdramMasking + cyclesRdramSetup + cyclesRdramTx;
}


auto PI::dmaWrite() -> void {
  u8 mem[128];

  if(!io.dmaBusy) {
    io.dmaBusy = 1;
    pi.runningDMA = {};
    pi.runningDMA.firstBlock = 1;
    pi.runningDMA.maxBlockSize = 128;
    pi.runningDMA.length = io.writeLength+1;
    pi.runningDMA.misalign = io.dramAddress & 7;
    pi.runningDMA.distEndOfRow = 0x800-(io.dramAddress&0x7ff);
    queue.insert(Queue::PI_DMA_Write, dmaWriteDuration());
    return;
  }

  i32 curLen = pi.runningDMA.blockLen();

  for (int i=0; i<curLen; i+=2) {
    u16 data = busRead<Half>(io.pbusAddress);
    mem[i+0] = data >> 8;
    mem[i+1] = data >> 0;
    io.pbusAddress += 2;
    pi.runningDMA.length -= 2;
  }

  if constexpr(Accuracy::CPU::Recompiler) {
    cpu.recompiler.invalidateRange(io.dramAddress, curLen);
  }
  
  if (pi.runningDMA.firstBlock && curLen < 127-pi.runningDMA.misalign) {
    for (i32 i = 0; i < curLen-pi.runningDMA.misalign; i++) {
      rdram.ram.write<Byte>(io.dramAddress++, mem[i], "PI DMA");
    }
  } else {
    for (i32 i = 0; i < curLen-pi.runningDMA.misalign; i+=2) {
      rdram.ram.write<Byte>(io.dramAddress++, mem[i+0], "PI DMA");
      rdram.ram.write<Byte>(io.dramAddress++, mem[i+1], "PI DMA");
    }
  }

  io.dramAddress = (io.dramAddress + 7) & ~7;
  io.writeLength = curLen <= 8 ? 127-pi.runningDMA.misalign : 127;

  if (pi.runningDMA.length <= 0) {
    dmaFinished();
    return;
  }

  pi.runningDMA.firstBlock = 0;
  pi.runningDMA.maxBlockSize = pi.runningDMA.distEndOfRow < 8 ? 128-pi.runningDMA.misalign : 128;
  pi.runningDMA.misalign = io.dramAddress & 7;
  pi.runningDMA.distEndOfRow = 0x800-(io.dramAddress&0x7ff);
  queue.insert(Queue::PI_DMA_Write, dmaWriteDuration());
}

auto PI::dmaFinished() -> void {
  pi.runningDMA = {};
  io.dmaBusy = 0;
  io.interrupt = 1;
  mi.raise(MI::IRQ::PI);
}

auto PI::dmaReadDuration() -> u32 {
  // FIXME: old algorithm, we keep it for one-shot DMA reads for now
  auto len = io.readLength;
  len = (len | 1) + 1;

  BSD bsd;
  switch (io.pbusAddress.bit(24,31)) {
    case 0x05:               bsd = bsd2; break; 
    case range8(0x08, 0x0F): bsd = bsd2; break;
    default:                 bsd = bsd1; break;
  }

  auto pageShift = bsd.pageSize + 2;
  auto pageSize = 1 << pageShift;
  auto pageMask = pageSize - 1;
  auto pbusFirst = io.pbusAddress;
  auto pbusLast  = io.pbusAddress + len - 2;

  auto pbusFirstPage = pbusFirst >> pageShift;
  auto pbusLastPage  = pbusLast  >> pageShift;
  auto pbusPages = pbusLastPage - pbusFirstPage + 1;
  auto numBuffers = 0;
  auto partialBytes = 0;

  if (pbusFirstPage == pbusLastPage) {
    if (len == 128) numBuffers = 1;
    else partialBytes = len;
  } else {
    bool fullFirst = (pbusFirst & pageMask) == 0;
    bool fullLast  = ((pbusLast + 2) & pageMask) == 0;

    if (fullFirst) numBuffers++;
    else           partialBytes += pageSize - (pbusFirst & pageMask);
    if (fullLast)  numBuffers++;
    else           partialBytes += (pbusLast & pageMask) + 2;

    if (pbusFirstPage + 1 < pbusLastPage)
      numBuffers += (pbusPages - 2) * pageSize / 128;
  }

  u32 cycles = 0;
  cycles += (14 + bsd.latency + 1) * pbusPages;
  cycles += (bsd.pulseWidth + 1 + bsd.releaseDuration + 1) * len / 2;
  cycles += numBuffers * 28;
  cycles += partialBytes * 1;
  return cycles * 3;
}
