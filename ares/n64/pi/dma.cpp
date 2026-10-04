auto PI::dmaRead() -> void {
  io.readLength = (io.readLength | 1) + 1;

  auto& bsd = bsdForAddress(io.pbusAddress);
  u32 pageMask = (1 << (bsd.pageSize + 2)) - 1;
  busAddress(io.pbusAddress);

  for(u32 address = 0; address < io.readLength; address += 2) {
    u32 cursor = io.pbusAddress + address;
    if(address && (cursor & pageMask) == 0) busAddress(cursor);
    u16 data = rdram.ram.read<Half>(io.dramAddress + address, RBusDevice::PI_DMA);
    busWriteHalf(data);
  }
}

auto PI::dmaWrite() -> void {
  u8 mem[128];
  i32 length = io.writeLength+1;
  i32 maxBlockSize = 128;
  bool firstBlock = true;

  if constexpr(Accuracy::CPU::Recompiler) {
    cpu.recompiler.invalidateRange(io.dramAddress, (length + 1) & ~1);
  }

  auto& bsd = bsdForAddress(io.pbusAddress);
  u32 pageMask = (1 << (bsd.pageSize + 2)) - 1;
  bool addressSelected = false;

  while(length > 0) {
    i32 misalign = io.dramAddress & 7;
    i32 distEndOfRow = 0x800-(io.dramAddress&0x7ff);
    i32 blockLen = min(maxBlockSize-misalign, distEndOfRow);
    i32 curLen = min(length, blockLen);

    for(int i=0; i<curLen; i+=2) {
      if(!addressSelected || (io.pbusAddress & pageMask) == 0) {
        busAddress(io.pbusAddress);
        addressSelected = true;
      }
      u16 data = busReadHalf();
      mem[i+0] = data >> 8;
      mem[i+1] = data >> 0;
      io.pbusAddress += 2;
      length -= 2;
    }

    if(firstBlock && curLen < 127-misalign) {
      for(i32 i = 0; i < curLen-misalign; i++) {
        rdram.ram.write<Byte>(io.dramAddress++, mem[i], RBusDevice::PI_DMA);
      }
    } else {
      for(i32 i = 0; i < curLen-misalign; i+=2) {
        rdram.ram.write<Byte>(io.dramAddress++, mem[i+0], RBusDevice::PI_DMA);
        rdram.ram.write<Byte>(io.dramAddress++, mem[i+1], RBusDevice::PI_DMA);
      }
    }

    io.dramAddress = (io.dramAddress + 7) & ~7;
    io.writeLength = curLen <= 8 ? 127-misalign : 127;
    firstBlock = false;
    maxBlockSize = distEndOfRow < 8 ? 128-misalign : 128;
  }
}

auto PI::dmaFinished() -> void {
  io.dmaBusy = 0;
  io.interrupt = 1;
  mi.raise(MI::IRQ::PI);
}

auto PI::dmaDuration(bool read) -> u32 {
  auto len = read ? io.readLength : io.writeLength;
  len = (len | 1) + 1;

  auto& bsd = bsdForAddress(io.pbusAddress);

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

  if(pbusFirstPage == pbusLastPage) {
    if(len == 128) numBuffers = 1;
    else partialBytes = len;
  } else {
    bool fullFirst = (pbusFirst & pageMask) == 0;
    bool fullLast  = ((pbusLast + 2) & pageMask) == 0;

    if(fullFirst) numBuffers++;
    else           partialBytes += pageSize - (pbusFirst & pageMask);
    if(fullLast)  numBuffers++;
    else           partialBytes += (pbusLast & pageMask) + 2;

    if(pbusFirstPage + 1 < pbusLastPage)
      numBuffers += (pbusPages - 2) * pageSize / 128;
  }

  u32 cycles = 0;
  cycles += (14 + bsd.latency + 1) * pbusPages;
  cycles += (bsd.pulseWidth + 1 + bsd.releaseDuration + 1) * len / 2;
  cycles += numBuffers * 28;
  cycles += partialBytes * 1;
  return cycles * 3;
}
