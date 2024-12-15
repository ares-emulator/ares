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
  auto& region = !dma.current.pbusRegion ? dmem : imem;

  if(dma.busy.read) {
    if constexpr(Accuracy::RSP::Recompiler) {
      if(dma.current.pbusRegion) {
        recompiler.invalidate(dma.current.pbusAddress, dma.current.length + 8);
      }
    }
    for(u32 i = 0; i <= dma.current.length; i += 8) {
        u64 data = rdram.ram.read<Dual>(dma.current.dramAddress, nullptr);
        region.write<Dual>(dma.current.pbusAddress, data);
        if (system.homebrewMode) {
          rsp.debugger.dmaReadWord(dma.current.dramAddress, dma.current.pbusRegion, dma.current.pbusAddress);
        }
        dma.current.dramAddress += 8;
        dma.current.pbusAddress += 8;
    }
  }
  if(dma.busy.write) {
    for(u32 i = 0; i <= dma.current.length; i += 8) {
        u64 data = region.read<Dual>(dma.current.pbusAddress);
        rdram.ram.write<Dual>(dma.current.dramAddress, data, "RSP DMA");
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
