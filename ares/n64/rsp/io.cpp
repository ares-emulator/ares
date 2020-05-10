static const vector<string> registerNames = {
  "SP_PBUS_ADDRESS",
  "SP_DRAM_ADDRESS",
  "SP_READ_LENGTH",
  "SP_WRITE_LENGTH",
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

  if(address == 0) data = getControlRegister(0);
  if(address == 1) data = getControlRegister(1);
  if(address == 2) data = getControlRegister(2);
  if(address == 3) data = getControlRegister(3);
  if(address == 4) data = getControlRegister(4);
  if(address == 5) data = getControlRegister(5);
  if(address == 6) data = getControlRegister(6);
  if(address == 7) data = getControlRegister(7);

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

  if(address == 0) setControlRegister(0, data);
  if(address == 1) setControlRegister(1, data);
  if(address == 2) setControlRegister(2, data);
  if(address == 3) setControlRegister(3, data);
  if(address == 4) setControlRegister(4, data);
  if(address == 5) setControlRegister(5, data);
  if(address == 6) setControlRegister(6, data);
  if(address == 7) setControlRegister(7, data);

  if(address == 8) {
    //SP_PC_REG
    core.pc = uint12(data >> 0);
  }

  if(address == 9) {
    //SP_IBIST
  }

  print("* ", registerNames(address, "SP_UNKNOWN"), " <= ", hex(data, 8L), "\n");
}
