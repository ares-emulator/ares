auto PI::bufferDMARead() -> void {
  for (auto i : range(io.readLength)) {
    bb_nand.buffer.write<Byte>(io.pbusAddress++, rdram.ram.read<Byte>(io.dramAddress++, "PI Buffer DMA"));
  }
}

auto PI::bufferDMAWrite() -> void {
  if constexpr(Accuracy::CPU::Recompiler) {
    cpu.recompiler.invalidateRange(io.dramAddress, io.writeLength);
  }

  for (auto i : range(io.writeLength)) {
    rdram.ram.write<Byte>(io.dramAddress++, bb_nand.buffer.read<Byte>(io.pbusAddress++), "PI Buffer DMA");
  }
}

auto PI::dmaRead() -> void {
  if(system._BB()) {
    debug(unusual, "[PI::dmaRead] Tried to write to PI bus on iQue");
    return;
  }
  io.readLength = (io.readLength | 1) + 1;

  u32 lastCacheline = 0xffff'ffff;
  for(u32 address = 0; address < io.readLength; address += 2) {
    u16 data = rdram.ram.read<Half>(io.dramAddress + address, "PI DMA");
    busWrite<Half>(io.pbusAddress + address, data);
  }
}

inline auto PI::atbMatch(u32 address, u32 i) -> bool {
  return (address & ~bb_atb.addressMasks[i]) == bb_atb.pbusAddresses[i];
}

auto PI::atbLookup(u32 address) -> u32 {
  u32 i;
  if (bb_atb.entryCached != -1u && atbMatch(address, bb_atb.entryCached)) {
    // If cached and matches, use this
    i = bb_atb.entryCached;
  } else {
    // Otherwise, search for an entry that contains this address
    i = -1u;
    u32 l = 0, r = BB_ATB::MaxEntries - 1;
    while(l <= r) {
      u32 m = l + (r - l >> 1);
      if(atbMatch(address, m)) {
        i = m;
        break;
      }
      if(address < bb_atb.pbusAddresses[m])
        r = m - 1;
      else
        l = m + 1;
    }
    // Cache this result
    bb_atb.entryCached = i;
  }
  return i;
}

auto PI::dmaWrite() -> void {
  u8 mem[128];
  i32 length = io.writeLength+1;
  i32 maxBlockSize = 128;
  bool firstBlock = true;

  if constexpr(Accuracy::CPU::Recompiler) {
    cpu.recompiler.invalidateRange(io.dramAddress, (length + 1) & ~1);
  }

  if (system._BB()) {
    bb_aes.chainIV = true;
    // First, the ATB searches for the entry corresponding to the current address - 0x10,
    // in order to obtain the AES IV.
    u32 index = atbLookup(io.pbusAddress - 0x10);
    if (index == -1u) {
      debug(unusual, "[PI::dmaWrite] ATB MISS!");
      // No entry found, raise bus error on data
      cpu.exception.busData();
      cpu.pipeline.exception();
      return;
    }

    BB_ATB::Entry* entry = &bb_atb.entries[index];

    // The offset is the difference between the target and base PI addresses.
    // It is also the difference between the target and base NAND addresses.
    u32 offset = (io.pbusAddress - 0x10) & bb_atb.addressMasks[index];

    u32 endAlign = (io.pbusAddress + length + 0xF) & ~0xF;

    u32 curPageAddr = -1u;

    if (entry->ivSource) {
      // If this entry is an IV source, get the IV from the PI buffer
      aes.setIV(bb_nand.buffer, 0x4D0);
    } else {
      if (offset >= entry->maxOffset) {
        // Out of range of this entry, raise bus error on data
        cpu.exception.busData();
        cpu.pipeline.exception();
        return;
      }
      u32 pageOffset = offset & (0x200-1) & ~(0x10-1);

      // Convert the offset to page address + offset
      bb_nand.io.pageNumber = curPageAddr = (entry->nandAddr + offset) & ~(0x200-1);
      bb_nand.io.command = (pageOffset >= 0x100) ? NAND::Command::Read1 : NAND::Command::Read0;
      bb_nand.io.xferLen = (pageOffset >= 0x100) ? 0x110 : 0x210;
      bb_nand.io.bufferSel = (bb_nand.io.pageNumber & 0x200) >> 9;
      // Read the page for the IV
      nandCommandFinished();
      // Set the IV at the offset
      aes.setIV(bb_nand.buffer, bb_nand.io.bufferSel * 0x200 + pageOffset);

      pageOffset += 0x10;
      if (pageOffset != 0x200) {
        bb_aes.dataSize = min(endAlign - (io.pbusAddress & ~0xF), 0x200 - pageOffset) / 0x10 - 1;
        bb_aes.bufferOffset = bb_nand.io.bufferSel * 0x20 + pageOffset / 0x10;
        aesCommandFinished();
      }
    }

    offset += 0x10;

    while (length > 0) {
      i32 misalign = io.dramAddress & 7;
      i32 distEndOfRow = 0x800-(io.dramAddress&0x7ff);
      i32 blockLen = min(maxBlockSize-misalign, distEndOfRow);
      i32 curLen = min(length, blockLen);

      for (int i=0; i<curLen; i+=2) {
        if (offset >= entry->maxOffset) {
          // ATB entry has changed, if it is to be valid at all the next address
          // must be contained inside the next entry as they are sorted by pbusAddress.
          index++;
          entry++;

          // Get the new address offset
          offset = io.pbusAddress & bb_atb.addressMasks[index];
          // If it's oob again, raise bus error on data
          if (offset >= entry->maxOffset) {
            cpu.exception.busData();
            cpu.pipeline.exception();
            return;
          }
        }

        // Compute new page info
        u32 newPageAddr = (entry->nandAddr + offset) & ~(0x200-1);
        u32 curPageOffset = offset & (0x200-1);

        if (curPageAddr != newPageAddr) {
          // Page has changed, read and decrypt it
          bb_nand.io.pageNumber = curPageAddr = newPageAddr;
          bb_nand.io.command = (curPageOffset >= 0x100) ? NAND::Command::Read1 : NAND::Command::Read0;
          bb_nand.io.xferLen = (curPageOffset >= 0x100) ? 0x110 : 0x210;
          bb_nand.io.bufferSel = (bb_nand.io.pageNumber & 0x200) >> 9;
          nandCommandFinished();
          bb_aes.dataSize = min(endAlign - (io.pbusAddress & ~0xF), 0x200 - (curPageOffset & ~(0x10-1))) / 0x10 - 1;
          bb_aes.bufferOffset = bb_nand.io.bufferSel * 0x20 + curPageOffset / 0x10;
          aesCommandFinished();
        }

        // Read the data into RAM
        u16 data = bb_nand.buffer.read<Half>(bb_nand.io.bufferSel * 0x200 + curPageOffset);
#if 0
        if (bb_atb.entries[1].nandAddr == 0x02900000) {
          u16 dataCompare = gameROM.read<Half>(io.pbusAddress);

          if (data != dataCompare) {
            printf("Data comparison failed @ 0x%08X (Got 0x%04X expected 0x%04X)\n", u32(io.pbusAddress), data, dataCompare);
            printf("ATB Entry was %u\n", index);
            assert(false);
          }
        }
#endif
        mem[i+0] = data >> 8;
        mem[i+1] = data >> 0;

        offset += 2;
        io.pbusAddress += 2;
        length -= 2;
      }

      if (firstBlock && curLen < 127-misalign) {
        for (i32 i = 0; i < curLen-misalign; i++) {
          rdram.ram.write<Byte>(io.dramAddress++, mem[i], "PI DMA");
        }
      } else {
        for (i32 i = 0; i < curLen-misalign; i+=2) {
          rdram.ram.write<Byte>(io.dramAddress++, mem[i+0], "PI DMA");
          rdram.ram.write<Byte>(io.dramAddress++, mem[i+1], "PI DMA");
        }
      }

      io.dramAddress = (io.dramAddress + 7) & ~7;
      io.writeLength = curLen <= 8 ? 127-misalign : 127;
      firstBlock = false;
      maxBlockSize = distEndOfRow < 8 ? 128-misalign : 128;
    }
  } else {
    while (length > 0) {
      i32 misalign = io.dramAddress & 7;
      i32 distEndOfRow = 0x800-(io.dramAddress&0x7ff);
      i32 blockLen = min(maxBlockSize-misalign, distEndOfRow);
      i32 curLen = min(length, blockLen);

      for (int i=0; i<curLen; i+=2) {
        u16 data = busRead<Half>(io.pbusAddress);
        mem[i+0] = data >> 8;
        mem[i+1] = data >> 0;
        io.pbusAddress += 2;
        length -= 2;
      }

      if (firstBlock && curLen < 127-misalign) {
        for (i32 i = 0; i < curLen-misalign; i++) {
          rdram.ram.write<Byte>(io.dramAddress++, mem[i], "PI DMA");
        }
      } else {
        for (i32 i = 0; i < curLen-misalign; i+=2) {
          rdram.ram.write<Byte>(io.dramAddress++, mem[i+0], "PI DMA");
          rdram.ram.write<Byte>(io.dramAddress++, mem[i+1], "PI DMA");
        }
      }

      io.dramAddress = (io.dramAddress + 7) & ~7;
      io.writeLength = curLen <= 8 ? 127-misalign : 127;
      firstBlock = false;
      maxBlockSize = distEndOfRow < 8 ? 128-misalign : 128;
    }
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
