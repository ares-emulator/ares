static const vector<string> registerNamesSCC = {
  "SP_PBUS_ADDRESS",
  "SP_DRAM_ADDRESS",
  "SP_READ_LENGTH",
  "SP_WRITE_LENGTH",
  "SP_STATUS",
  "SP_DMA_FULL",
  "SP_DMA_BUSY",
  "SP_SEMAPHORE",
};

auto RSP::readSCC(u32 address) -> u32 {
  address = (address & 0x3ffff) >> 2;
  uint32 data;

  if(address == 0) {
    //SP_PBUS_ADDRESS
    data.bit( 0,11) = dma.memAddress;
    data.bit(12)    = dma.memSource;
  }

  if(address == 1) {
    //SP_DRAM_ADDRESS
    data.bit(0,23) = dma.dramAddress;
  }

  if(address == 2) {
    //SP_READ_LENGTH
    data.bit( 0,11) = dma.read.length;
    data.bit(12,19) = dma.read.count;
    data.bit(20,31) = dma.read.skip;
  }

  if(address == 3) {
    //SP_WRITE_LENGTH
    data.bit( 0,11) = dma.write.length;
    data.bit(12,19) = dma.write.count;
    data.bit(20,31) = dma.write.skip;
  }

  if(address == 4) {
    //SP_STATUS
    data.bit( 0) = status.halted;
    data.bit( 1) = status.broken;
    data.bit( 2) = dma.busy;
    data.bit( 3) = dma.full;
    data.bit( 4) = status.full;
    data.bit( 5) = status.singleStep;
    data.bit( 6) = status.interruptOnBreak;
    data.bit( 7) = status.signal[0];
    data.bit( 8) = status.signal[1];
    data.bit( 9) = status.signal[2];
    data.bit(10) = status.signal[3];
    data.bit(11) = status.signal[4];
    data.bit(12) = status.signal[5];
    data.bit(13) = status.signal[6];
    data.bit(14) = status.signal[7];
  }

  if(address == 5) {
    //SP_DMA_FULL
    data.bit(0) = dma.full;
  }

  if(address == 6) {
    //SP_DMA_BUSY
    data.bit(0) = dma.busy;
  }

  if(address == 7) {
    //SP_SEMAPHORE
    data.bit(0) = status.semaphore;
    status.semaphore = 1;
  }

  if(debugger.tracer.io->enabled()) {
    debugger.io({registerNamesSCC(address, "SP_UNKNOWN"), " => ", hex(data, 8L)});
  }
  return data;
}

auto RSP::writeSCC(u32 address, uint32 data) -> void {
  address = (address & 0x3ffff) >> 2;

  if(address == 0) {
    //SP_PBUS_ADDRESS
    dma.memAddress = data.bit( 0,11);
    dma.memSource  = data.bit(12);
  }

  if(address == 1) {
    //SP_DRAM_ADDRESS
    dma.dramAddress = data.bit(0,23);
  }

  if(address == 2) {
    //SP_READ_LENGTH
    dma.read.length = data.bit( 0,11);
    dma.read.count  = data.bit(12,19);
    dma.read.skip   = data.bit(20,31);

    auto& memory = dma.memSource == 0 ? dmem : imem;
    auto source  = dma.dramAddress & ~7;
    auto target  = dma.memAddress  & ~7;
    auto length  = (dma.read.length | 7) + 1;
    auto count   = dma.read.count + 1;
    auto skip    = dma.read.skip;
    for(u32 line = 0; line < count; line++) {
      auto base = line * (length + skip);
      for(u32 offset = 0; offset < length; offset += 8) {
        u64 data = rdram.ram.readDouble(source + base + offset);
        memory.writeDouble(target + offset, data);
      }
    }
  }

  if(address == 3) {
    //SP_WRITE_LENGTH
    dma.write.length = data.bit( 0,11);
    dma.write.count  = data.bit(12,19);
    dma.write.skip   = data.bit(20,31);

    auto& memory = dma.memSource == 0 ? dmem : imem;
    auto source  = dma.memAddress  & ~7;
    auto target  = dma.dramAddress & ~7;
    auto length  = (dma.write.length | 7) + 1;
    auto count   = dma.write.count + 1;
    auto skip    = dma.write.skip;
    for(u32 line = 0; line < count; line++) {
      auto base = line * (length + skip);
      for(u32 offset = 0; offset < length; offset += 8) {
        u64 data = memory.readDouble(source + offset);
        rdram.ram.writeDouble(target + base + offset, data);
      }
    }
  }

  if(address == 4) {
    //SP_STATUS
    if(data.bit( 0)) status.halted = 0;
    if(data.bit( 1)) status.halted = 1;
    if(data.bit( 2)) status.broken = 0;
    if(data.bit( 3)) mi.lower(MI::IRQ::SP);
    if(data.bit( 4)) mi.raise(MI::IRQ::SP);
    if(data.bit( 5)) status.singleStep = 0;
    if(data.bit( 6)) status.singleStep = 1;
    if(data.bit( 7)) status.interruptOnBreak = 0;
    if(data.bit( 8)) status.interruptOnBreak = 1;
    if(data.bit( 9)) status.signal[0] = 0;
    if(data.bit(10)) status.signal[0] = 1;
    if(data.bit(11)) status.signal[1] = 0;
    if(data.bit(12)) status.signal[1] = 1;
    if(data.bit(13)) status.signal[2] = 0;
    if(data.bit(14)) status.signal[2] = 1;
    if(data.bit(15)) status.signal[3] = 0;
    if(data.bit(16)) status.signal[3] = 1;
    if(data.bit(17)) status.signal[4] = 0;
    if(data.bit(18)) status.signal[4] = 1;
    if(data.bit(19)) status.signal[5] = 0;
    if(data.bit(20)) status.signal[5] = 1;
    if(data.bit(21)) status.signal[6] = 0;
    if(data.bit(22)) status.signal[6] = 1;
    if(data.bit(23)) status.signal[7] = 0;
    if(data.bit(24)) status.signal[7] = 1;
  }

  if(address == 5) {
    //SP_DMA_FULL (read-only)
  }

  if(address == 6) {
    //SP_DMA_BUSY (read-only)
  }

  if(address == 7) {
    //SP_SEMAPHORE
    if(!data.bit(0)) status.semaphore = 0;
  }

  if(debugger.tracer.io->enabled()) {
    debugger.io({registerNamesSCC(address, "SP_UNKNOWN"), " <= ", hex(data, 8L)});
  }
}
