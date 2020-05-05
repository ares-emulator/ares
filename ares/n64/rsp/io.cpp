static const vector<string> registerNames = {
  "SP_MEM_ADDR",
  "SP_DRAM_ADDR",
  "SP_RD_LEN",
  "SP_WR_LEN",
  "SP_STATUS",
  "SP_DMA_FULL",
  "SP_DMA_BUSY",
  "SP_SEMAPHORE",
  "SP_PC_REG",
  "SP_IBIST",
};

auto RSP::readIO(u32 address) -> u32 {
  if(address >= 0x0404'0000 && address <= 0x0404'001f) {
    address = (address - 0x0404'0000 >> 2) + 0;
  } else if(address >= 0x0408'0000 && address <= 0x0408'0007) {
    address = (address - 0x0408'0000 >> 2) + 8;
  } else {
    address = 10;  //SP_UNKNOWN
  }
  u32 data = 0;

  if(address == 0) {
    //SP_MEM_ADDR
    data |= io.memAddress <<  0;
    data |= io.memSource  << 12;
  }

  if(address == 1) {
    //SP_DRAM_ADDR
    data |= io.dramAddress << 0;
  }

  if(address == 2) {
    //SP_RD_LEN
    data |= io.readLength <<  0;
    data |= io.readCount  << 12;
    data |= io.readSkip   << 20;
  }

  if(address == 3) {
    //SP_WR_LEN
    data |= io.writeLength <<  0;
    data |= io.writeCount  << 12;
    data |= io.writeSkip   << 20;
  }

  if(address == 4) {
    //SP_STATUS
    data |= io.halt             << 0;
    data |= io.interruptOnBreak << 6;
  }

  if(address == 5) {
    //SP_DMA_FULL
  }

  if(address == 6) {
    //SP_DMA_BUSY
  }

  if(address == 7) {
    //SP_SEMAPHORE
  }

  if(address == 8) {
    //SP_PC_REG
    data |= uint12(core.pc) << 0;
  }

  if(address == 9) {
    //SP_IBIST
  }

  print("* ", registerNames(address, "SP_UNKNOWN"), " => ", hex(data, 8L), "\n");
  return data;
}

auto RSP::writeIO(u32 address, u32 data) -> void {
  if(address >= 0x0404'0000 && address <= 0x0404'001f) {
    address = (address - 0x0404'0000 >> 2) + 0;
  } else if(address >= 0x0408'0000 && address <= 0x0408'0007) {
    address = (address - 0x0408'0000 >> 2) + 8;
  } else {
    address = 10;  //SP_UNKNOWN
  }

  if(address == 0) {
    //SP_MEM_ADDR
    io.memAddress = uint12(data >>  0);
    io.memSource  =  uint1(data >> 12);
  }

  if(address == 1) {
    //SP_DRAM_ADDR
    io.dramAddress = uint24(data >> 0);
  }

  if(address == 2) {
    //SP_RD_LEN
    io.readLength = uint12(data >>  0) + 1;
    io.readCount  =  uint8(data >> 12);
    io.readSkip   = uint12(data >> 20);
    auto memory = io.memSource == 0 ? &dmem : &imem;
    for(u32 address = 0; address < io.readLength; address += 4) {
      auto data = rdram.ram.readWord(io.dramAddress + address);
      memory->writeWord(io.memAddress + address, data);
    }
  }

  if(address == 3) {
    //SP_WR_LEN
    io.writeLength = uint12(data >>  0) + 1;
    io.writeCount  =  uint8(data >> 12);
    io.writeSkip   = uint12(data >> 20);
    auto memory = io.memSource == 0 ? &dmem : &imem;
    for(u32 address = 0; address < io.writeLength; address += 4) {
      auto data = memory->readWord(io.memAddress + address);
      rdram.ram.writeWord(io.dramAddress + address, data);
    }
  }

  if(address == 4) {
    //SP_STATUS
    if(uint1(data >> 0)) io.halt = 0;
    if(uint1(data >> 1)) io.halt = 1;
    if(uint1(data >> 7)) io.interruptOnBreak = 0;
    if(uint1(data >> 8)) io.interruptOnBreak = 1;
  }

  if(address == 5) {
    //SP_DMA_FULL
  }

  if(address == 6) {
    //SP_DMA_BUSY
  }

  if(address == 7) {
    //SP_SEMAPHORE
  }

  if(address == 8) {
    //SP_PC_REG
    core.pc = uint12(data >> 0);
  }

  if(address == 9) {
    //SP_IBIST
  }

  print("* ", registerNames(address, "SP_UNKNOWN"), " <= ", hex(data, 8L), "\n");
}
