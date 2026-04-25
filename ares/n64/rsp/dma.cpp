auto RSP::dmaQueue(u32 clocks, Thread& thread) -> void {
  dma.clock = (Thread::clock - thread.clock) - clocks;
}

auto RSP::dmaStep(u32 clocks) -> void {
  if(dma.busy.any()) {
    dma.clock += clocks;
    if(dma.clock >= 0) {
      dmaTransferStep();
    }
  }
}

auto RSP::dmaTransferStart(Thread& thread) -> void {
  if(dma.busy.any()) return;
  if(dma.full.any()) {
    dma.current = dma.pending;
    dma.busy    = dma.full;
    dma.full    = {0,0};
    dmaQueue((dma.current.length+8) / 8 * 3, thread);
  }
}

auto RSP::dmaTransferStep() -> void {
  if(dma.busy.read) {
    if constexpr(Accuracy::RSP::Recompiler) {
      if(dma.current.pbusRegion) {
        recompiler.invalidate(dma.current.pbusAddress, dma.current.length + 8);
      }
    }
    for(u32 i = 0; i <= dma.current.length; i += 8) {
      if(dma.current.pbusRegion) {
        u64 data = rdram.ram.read<Dual>(dma.current.dramAddress, RBusDevice::SP_DMA);
        imem.write<Dual>(dma.current.pbusAddress, data);
      } else {
        u32 dataLo = rdram.ram.read<Word>(dma.current.dramAddress + 0, RBusDevice::SP_DMA);
        u32 dataHi = rdram.ram.read<Word>(dma.current.dramAddress + 4, RBusDevice::SP_DMA);
        dmem.write<Word>(dma.current.pbusAddress + 0, dataLo);
        dmem.write<Word>(dma.current.pbusAddress + 4, dataHi);
      }
      if(system.homebrewMode) {
        rsp.debugger.dmaReadWord(dma.current.dramAddress, dma.current.pbusRegion, dma.current.pbusAddress);
      }
      dma.current.dramAddress += 8;
      dma.current.pbusAddress += 8;
    }
  }
  if(dma.busy.write) {
    for(u32 i = 0; i <= dma.current.length; i += 8) {
      if(dma.current.pbusRegion) {
        u64 data = imem.read<Dual>(dma.current.pbusAddress);
        rdram.ram.write<Dual>(dma.current.dramAddress, data, RBusDevice::SP_DMA);
      } else {
        u32 dataLo = dmem.read<Word>(dma.current.pbusAddress + 0);
        u32 dataHi = dmem.read<Word>(dma.current.pbusAddress + 4);
        rdram.ram.write<Word>(dma.current.dramAddress + 0, dataLo, RBusDevice::SP_DMA);
        rdram.ram.write<Word>(dma.current.dramAddress + 4, dataHi, RBusDevice::SP_DMA);
      }
      dma.current.dramAddress += 8;
      dma.current.pbusAddress += 8;
    }
  }

  if(dma.current.count) {
    dma.current.count -= 1;
    dma.current.dramAddress += dma.current.skip;
    dmaQueue((dma.current.length+8) / 8 * 3, *this);
  } else {
    dma.busy = {0,0};
    dma.current.length = 0xFF8;
    dmaTransferStart(*this);
  }
}
