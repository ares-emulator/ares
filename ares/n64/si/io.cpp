static const vector<string> registerNames = {
  "SI_DRAM_ADDR",
  "SI_PIF_ADDR_RD64B",
  "SI_RESERVED_2",
  "SI_RESERVED_3",
  "SI_PIF_ADDR_WR64B",
  "SI_RESERVED_5",
  "SI_STATUS",
};

auto SI::readIO(u32 address) -> u32 {
  address = (address & 0xfffff) >> 2;
  u32 data = 0;

  if(address == 0) {
    //SI_DRAM_ADDR
  }

  if(address == 1) {
    //SI_PIF_ADDR_RD64B
  }

  if(address == 2) {
    //SI_RESERVED_2
  }

  if(address == 3) {
    //SI_RESERVED_3
  }

  if(address == 4) {
    //SI_PIF_ADDR_WR64B
  }

  if(address == 5) {
    //SI_RESERVED_5
  }

  if(address == 6) {
    //SI_STATUS
  }

  print("* ", registerNames(address, "SI_UNKNOWN"), " => ", hex(data, 8L), "\n");
  return data;
}

auto SI::writeIO(u32 address, u32 data) -> void {
  address = (address & 0xfffff) >> 2;

  if(address == 0) {
    //SI_DRAM_ADDR
  }

  if(address == 1) {
    //SI_PIF_ADDR_RD64B
  }

  if(address == 2) {
    //SI_RESERVED_2
  }

  if(address == 3) {
    //SI_RESERVED_3
  }

  if(address == 4) {
    //SI_PIF_ADDR_WR64B
  }

  if(address == 5) {
    //SI_RESERVED_5
  }

  if(address == 6) {
    //SI_STATUS
  }

  print("* ", registerNames(address, "SI_UNKNOWN"), " <= ", hex(data, 8L), "\n");
}
