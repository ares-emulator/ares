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
  }

  if(address == 1) {
    //SP_DRAM_ADDR
  }

  if(address == 2) {
    //SP_RD_LEN
  }

  if(address == 3) {
    //SP_WR_LEN
  }

  if(address == 4) {
    //SP_STATUS
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
  }

  if(address == 1) {
    //SP_DRAM_ADDR
  }

  if(address == 2) {
    //SP_RD_LEN
  }

  if(address == 3) {
    //SP_WR_LEN
  }

  if(address == 4) {
    //SP_STATUS
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
  }

  if(address == 9) {
    //SP_IBIST
  }

  print("* ", registerNames(address, "SP_UNKNOWN"), " <= ", hex(data, 8L), "\n");
}
