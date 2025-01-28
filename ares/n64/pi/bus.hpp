inline auto PI::readWord(u32 address, Thread& thread) -> u32 {
  if(address <= 0x046f'ffff) return ioRead(address);

  if (unlikely(io.ioBusy)) {
    debug(unusual, "[PI::readWord] PI read to 0x", hex(address, 8L), " will not behave as expected because PI writing is in progress");
    thread.step(writeForceFinish() * 2);
    return io.busLatch;
  }
  if (system._BB()) {
    thread.step(1000 * 2);

    bb_nand.io.xferLen = 0x210;
    bb_aes.chainIV = true;
    u32 index = atbLookup(address - 0x10);
    if (index == -1u) {
      debug(unusual, "[PI::readWord] ATB MISS!");
      // No entry found, raise bus error on data
      cpu.exception.busData();
      cpu.pipeline.exception();
      return 0;
    }

    BB_ATB::Entry* entry = &bb_atb.entries[index];

    // The offset is the difference between the target and base PI addresses.
    // It is also the difference between the target and base NAND addresses.
    u32 offset = (address - 0x10) & bb_atb.addressMasks[index];

    u32 curPageAddr = -1u;

    if (entry->ivSource) {
      // If this entry is an IV source, get the IV from the PI buffer
      aes.setIV(bb_nand.buffer, 0x4D0);
    } else {
      if (offset >= entry->maxOffset) {
        // Out of range of this entry, raise bus error on data
        cpu.exception.busData();
        cpu.pipeline.exception();
        return 0;
      }
      // Convert the offset to page address + offset
      bb_nand.io.pageNumber = curPageAddr = (entry->nandAddr + offset) & ~(0x200-1);
      bb_nand.io.bufferSel = (bb_nand.io.pageNumber & 0x200) >> 9;
      // Read the page for the IV
      nandCommandFinished();
      // Set the IV at the offset
      aes.setIV(bb_nand.buffer, bb_nand.io.bufferSel * 0x200 + offset & (0x200-1) & ~(0x10-1));
    }

    offset += 0x10;

    if (offset >= entry->maxOffset) {
      // ATB entry has changed, if it is to be valid at all the next address
      // must be contained inside the next entry as they are sorted by pbusAddress.
      index++;
      entry++;

      // Get the new address offset
      offset = address & bb_atb.addressMasks[index];
      // If it's oob again, raise bus error on data
      if (offset >= entry->maxOffset) {
        cpu.exception.busData();
        cpu.pipeline.exception();
        return 0;
      }
    }

    // Compute new page info
    u32 newPageAddr = (entry->nandAddr + offset) & ~(0x200-1);
    u32 curPageOffset = offset & (0x200-1);

    if (curPageAddr != newPageAddr) {
      // Page has changed, read it
      bb_nand.io.pageNumber = curPageAddr = newPageAddr;
      bb_nand.io.bufferSel = (bb_nand.io.pageNumber & 0x200) >> 9;
      nandCommandFinished();
    }

    // Decrypt just 0x10 bytes at the location we're going to read
    bb_aes.dataSize = 0; // 0x10
    bb_aes.bufferOffset = bb_nand.io.bufferSel * 0x20 + curPageOffset / 0x10;
    aesCommandFinished();

    io.busLatch = bb_nand.buffer.read<Word>(bb_nand.io.bufferSel * 0x200 + curPageOffset);
  } else {
    thread.step(250 * 2);
    io.busLatch = busRead<Word>(address);
  }
  return io.busLatch;
}

template <u32 Size>
inline auto PI::busRead(u32 address) -> u32 {
  static_assert(Size == Half || Size == Word);  //PI bus will do 32-bit (CPU) or 16-bit (DMA) only

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

inline auto PI::writeWord(u32 address, u32 data, Thread& thread) -> void {
  if(address <= 0x046f'ffff) return ioWrite(address, data);
  if(system._BB()) {
    debug(unusual, "[PI::writeWord] Tried to write to PI bus on iQue");
    return;
  }

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
