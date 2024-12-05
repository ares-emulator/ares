inline auto PI::readWord(u32 address, Thread& thread) -> u32 {
  if(address <= 0x046f'ffff) return ioRead(address);

  if (unlikely(io.ioBusy)) {
    debug(unusual, "[PI::readWord] PI read to 0x", hex(address, 8L), " will not behave as expected because PI writing is in progress");
    thread.step(writeForceFinish() * 2);
    return io.busLatch;
  }
  thread.step(250 * 2);
  io.busLatch = busRead<Word>(address);
  return io.busLatch;
}

inline auto PI::atbMatch(u32 address, u32 i) -> bool {
  if ((address & ~bb_atb.addressMasks[i]) != bb_atb.pbusAddresses[i])
    return false;

  u32 offset = address & bb_atb.addressMasks[i];
  if (offset >= 0x4000 * bb_atb.entries[i].numBlocks)
    return false;

  return true;
}

template <u32 Size>
inline auto PI::busRead(u32 address) -> u32 {
  static_assert(Size == Half || Size == Word);  //PI bus will do 32-bit (CPU) or 16-bit (DMA) only

  if(system._BB()) {
    // ATB

    // Search for the entry
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
        if((address & ~bb_atb.addressMasks[m]) < bb_atb.pbusAddresses[m])
          r = m - 1;
        else
          l = m + 1;
      }

      // Check for a miss if no entry was found
      if (i == -1u) {
        printf("!! ATB MISS !!\n");
        cpu.exception.busData();
        cpu.pipeline.exception();
        return 0;
      }

      // Found an entry, cache it
      bb_atb.entryCached = i;
    }

    // Hit

    u32 offset = address & bb_atb.addressMasks[i];
    BB_ATB::Entry &entry = bb_atb.entries[i];

    if (!entry.dmaEnable && !entry.cpuEnable) {
      // TODO error of some sort
      printf("!! ATB Permission Error !!\n");
      cpu.exception.busData();
      cpu.pipeline.exception();
      return 0;
    }

    u32 pageNum = (entry.nandAddr + offset) & ~(0x200-1);
    u32 pageOffset = offset & (0x200-1);

    // This is a huge guess but there's surely no way there isn't something that at least resembles this
    if(pageNum != bb_atb.pageCached) {
      // Read the page into the PI buffer

      bb_nand.io.xferLen = 0x210;
      bb_aes.chainIV = true;

      // IV determination logic
      // TODO this is probably missing setting the IV in some situations, like when starting a DMA
      // in the middle of a page
      if (((address - 0x10) & ~bb_atb.addressMasks[i]) != bb_atb.pbusAddresses[i]) {
        BB_ATB::Entry &prevEntry = bb_atb.entries[i - 1];

        if (prevEntry.ivSource) {
          aes.setIV(bb_nand.buffer, 0x4D0);
        } else {
          // need to read last page of previous block for IV, note the blocks may not be
          // contiguous so this is not necessarily pageNum - 1
          bb_nand.io.pageNumber = prevEntry.nandAddr + 0x3E00;
          nandCommandFinished();
          aes.setIV(bb_nand.buffer, 0x200 - 0x10);
        }
      }

      // Read and decode the page
      bb_nand.io.pageNumber = pageNum;
      nandCommandFinished();
      aesCommandFinished();
      // Set this page as active
      bb_atb.pageCached = pageNum;
    }

    return bb_nand.buffer.read<Size>(bb_nand.io.bufferSel * 0x200 + pageOffset);
  } else {
    const u32 unmapped = (address & 0xFFFF) | (address << 16);

    if(address <= 0x04ff'ffff) return unmapped; //Address range not memory mapped, only accessible via DMA
    if(address <= 0x0500'03ff) {
        if(_DD()) return dd.c2s.read<Size>(address);
        return unmapped;
    }
    if(address <= 0x0500'04ff) {
        if(_DD()) return dd.ds.read<Size>(address);
        return unmapped;
    }
    if(address <= 0x0500'057f) {
        if(_DD()) return dd.read<Size>(address);
        return unmapped;
    }
    if(address <= 0x0500'05bf) {
        if(_DD()) return dd.ms.read<Size>(address);
        return unmapped;
    }
    if(address <= 0x05ff'ffff) return unmapped;
    if(address <= 0x063f'ffff) {
        if(_DD()) return dd.iplrom.read<Size>(address);
        return unmapped;
    }
    if(address <= 0x07ff'ffff) return unmapped;
    if(address <= 0x0fff'ffff) {
        if(cartridge.ram  ) return cartridge.ram.read<Size>(address);
        if(cartridge.flash) return cartridge.flash.read<Size>(address);
        return unmapped;
    }
    if(cartridge.isviewer.enabled() && address >= 0x13f0'0000 && address <= 0x13ff'ffff) {
        return cartridge.isviewer.read<Size>(address);
    }
    if(address <= 0x1000'0000 + cartridge.rom.size - 1) {
        return cartridge.rom.read<Size>(address);
    }
    return unmapped;
  }
}

inline auto PI::writeWord(u32 address, u32 data, Thread& thread) -> void {
  if(address <= 0x046f'ffff) return ioWrite(address, data);

  if(io.ioBusy) return;
  io.ioBusy = 1;
  io.busLatch = data;
  queue.insert(Queue::PI_BUS_Write, 400);
  return busWrite<Word>(address, data);
}

template <u32 Size>
inline auto PI::busWrite(u32 address, u32 data) -> void {
  static_assert(Size == Half || Size == Word);  //PI bus will do 32-bit (CPU) or 16-bit (DMA) only
  if(address <= 0x04ff'ffff) return; //Address range not memory mapped, only accessible via DMA
  if(address <= 0x0500'03ff) {
    if(_DD()) return dd.c2s.write<Size>(address, data);
    return;
  }
  if(address <= 0x0500'04ff) {
    if(_DD()) return dd.ds.write<Size>(address, data);
    return;
  }
  if(address <= 0x0500'057f) {
    if(_DD()) return dd.write<Size>(address, data);
    return;
  }
  if(address <= 0x0500'05bf) {
    if(_DD()) return dd.ms.write<Size>(address, data);
    return;
  }
  if(address <= 0x05ff'ffff) return;
  if(address <= 0x063f'ffff) {
    if(_DD()) return dd.iplrom.write<Size>(address, data);
    return;
  }
  if(address <= 0x07ff'ffff) return;
  if(address <= 0x0fff'ffff) {
    if(cartridge.ram  ) return cartridge.ram.write<Size>(address, data);
    if(cartridge.flash) return cartridge.flash.write<Size>(address, data);
    return;
  }
  if(address >= 0x13f0'0000 && address <= 0x13ff'ffff) {
    if(cartridge.isviewer.enabled()) {
      writeForceFinish(); //Debugging channel for homebrew, be gentle
      return cartridge.isviewer.write<Size>(address, data);      
    } else {
      debug(unhandled, "[PI::busWrite] attempt to write to ISViewer: ROM is too big so ISViewer is disabled");
    }
  }
  if(address <= 0x1000'0000 + cartridge.rom.size - 1) {
    return cartridge.rom.write<Size>(address, data);
  }
  if(address <= 0x7fff'ffff) return;
}

inline auto PI::writeFinished() -> void {
  io.ioBusy = 0;
}

inline auto PI::writeForceFinish() -> u32 {
  io.ioBusy = 0;
  return queue.remove(Queue::PI_BUS_Write);
}
