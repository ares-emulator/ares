auto RSP::getControlRegister(uint5 index) -> u64 {
  uint64 data;
  switch(index) {
  case  0:  //DMA_CACHE
    data.bit( 0,11) = scc.dma.memAddress;
    data.bit(12)    = scc.dma.memSource;
    break;
  case  1:  //DMA_DRAM
    data.bit(0,23) = scc.dma.dramAddress;
    break;
  case  2:  //DMA_READ_LENGTH
    data.bit( 0,11) = scc.dma.read.length;
    data.bit(12,19) = scc.dma.read.count;
    data.bit(20,31) = scc.dma.read.skip;
    break;
  case  3:  //DMA_WRITE_LENGTH
    data.bit( 0,11) = scc.dma.write.length;
    data.bit(12,19) = scc.dma.write.count;
    data.bit(20,31) = scc.dma.write.skip;
    break;
  case  4:  //SP_STATUS
    data.bit( 0) = scc.status.halted;
    data.bit( 1) = scc.status.broken;
    data.bit( 2) = scc.dma.busy;
    data.bit( 3) = scc.dma.full;
    data.bit( 4) = scc.status.ioFull;
    data.bit( 5) = scc.status.singleStep;
    data.bit( 6) = scc.status.interruptOnBreak;
    data.bit( 7) = scc.status.signal[0];
    data.bit( 8) = scc.status.signal[1];
    data.bit( 9) = scc.status.signal[2];
    data.bit(10) = scc.status.signal[3];
    data.bit(11) = scc.status.signal[4];
    data.bit(12) = scc.status.signal[5];
    data.bit(13) = scc.status.signal[6];
    data.bit(14) = scc.status.signal[7];
    break;
  case  5:  //DMA_FULL
    data.bit(0) = scc.dma.full;
    break;
  case  6:  //DMA_BUSY
    data.bit(0) = scc.dma.busy;
    break;
  case  7:  //SP_SEMAPHORE
    scc.semaphore = 1;
    data.bit(0) = scc.semaphore;
    break;
  case  8:  //CMD_START
    break;
  case  9:  //CMD_END
    break;
  case 10:  //CMD_CURRENT
    break;
  case 11:  //CMD_STATUS
    break;
  case 12:  //CMD_CLOCK
    break;
  case 13:  //CMD_BUSY
    break;
  case 14:  //CMD_PIPE_BUSY
    break;
  case 15:  //CMD_TMEM_BUSY
    break;
  }
  return data;
}

auto RSP::setControlRegister(uint5 index, uint64 data) -> void {
  switch(index) {
  case  0:  //DMA_CACHE
    scc.dma.memAddress = data.bit( 0,11);
    scc.dma.memSource  = data.bit(12);
    break;
  case  1:  //DMA_DRAM
    scc.dma.dramAddress = data.bit(0,23);
    break;
  case  2: {//DMA_READ_LENGTH
    scc.dma.read.length = data.bit( 0,11);
    scc.dma.read.count  = data.bit(12,19);
    scc.dma.read.skip   = data.bit(20,31);
    auto memory = scc.dma.memSource == 0 ? &dmem : &imem;
    auto source = scc.dma.dramAddress & ~7;
    auto target = scc.dma.memAddress  & ~7;
    auto length = (scc.dma.read.length | 7) + 1;
    auto count  = scc.dma.read.count + 1;
    auto skip   = scc.dma.read.skip;
    for(u32 line = 0; line < count; line++) {
      auto base = line * (length + skip);
      for(u32 offset = 0; offset < length; offset += 8) {
        u64 data = rdram.ram.readDouble(source + base + offset);
        memory->writeDouble(target + offset, data);
      }
    }
  } break;
  case  3: {//DMA_WRITE_LENGTH
    scc.dma.write.length = data.bit( 0,11);
    scc.dma.write.count  = data.bit(12,19);
    scc.dma.write.skip   = data.bit(20,31);
    auto memory = scc.dma.memSource == 0 ? &dmem : &imem;
    auto source = scc.dma.memAddress  & ~7;
    auto target = scc.dma.dramAddress & ~7;
    auto length = (scc.dma.write.length | 7) + 1;
    auto count  = scc.dma.write.count + 1;
    auto skip   = scc.dma.write.skip;
    for(u32 line = 0; line < count; line++) {
      auto base = line * (length + skip);
      for(u32 offset = 0; offset < length; offset += 8) {
        u64 data = memory->readDouble(source + offset);
        rdram.ram.writeDouble(target + base + offset, data);
      }
    }
  } break;
  case  4:  //SP_STATUS
    if(data.bit( 0)) scc.status.halted = 0;
    if(data.bit( 1)) scc.status.halted = 1;
    if(data.bit( 2)) scc.status.broken = 0;
    if(data.bit( 3)) mi.irq.sp.line = 0;
    if(data.bit( 4)) mi.irq.sp.line = 1;
    if(data.bit( 5)) scc.status.singleStep = 0;
    if(data.bit( 6)) scc.status.singleStep = 1;
    if(data.bit( 7)) scc.status.interruptOnBreak = 0;
    if(data.bit( 8)) scc.status.interruptOnBreak = 1;
    if(data.bit( 9)) scc.status.signal[0] = 0;
    if(data.bit(10)) scc.status.signal[0] = 1;
    if(data.bit(11)) scc.status.signal[1] = 0;
    if(data.bit(12)) scc.status.signal[1] = 1;
    if(data.bit(13)) scc.status.signal[2] = 0;
    if(data.bit(14)) scc.status.signal[2] = 1;
    if(data.bit(15)) scc.status.signal[3] = 0;
    if(data.bit(16)) scc.status.signal[3] = 1;
    if(data.bit(17)) scc.status.signal[4] = 0;
    if(data.bit(18)) scc.status.signal[4] = 1;
    if(data.bit(19)) scc.status.signal[5] = 0;
    if(data.bit(20)) scc.status.signal[5] = 1;
    if(data.bit(21)) scc.status.signal[6] = 0;
    if(data.bit(22)) scc.status.signal[6] = 1;
    if(data.bit(23)) scc.status.signal[7] = 0;
    if(data.bit(24)) scc.status.signal[7] = 1;
    break;
  case  5:  //DMA_FULL (read-only)
    break;
  case  6:  //DMA_BUSY (read-only)
    break;
  case  7:  //SP_SEMAPHORE
    if(!data.bit(0)) scc.semaphore = 0;
    break;
  case  8:  //CMD_START
    break;
  case  9:  //CMD_END
    break;
  case 10:  //CMD_CURRENT
    break;
  case 11:  //CMD_STATUS
    break;
  case 12:  //CMD_CLOCK
    break;
  case 13:  //CMD_BUSY
    break;
  case 14:  //CMD_PIPE_BUSY
    break;
  case 15:  //CMD_TMEM_BUSY
    break;
  }
}
