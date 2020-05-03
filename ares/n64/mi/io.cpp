static const vector<string> registerNames = {
  "MI_INIT_MODE",
  "MI_VERSION",
  "MI_INTR",
  "MI_INTR_MASK",
};

auto MI::readIO(u32 address) -> u32 {
  address = (address & 0xfffff) >> 2;
  u32 data = 0;

  if(address == 0) {
    //MI_INIT_MODE
  }

  if(address == 1) {
    //MI_VERSION
    data = 0x01010101;  //unverified
  }

  if(address == 2) {
    //MI_INTR
  }

  if(address == 3) {
    //MI_INTR_MASK
  }

  print("* ", registerNames(address, "MI_UNKNOWN"), " => ", hex(data, 8L), "\n");
  return data;
}

auto MI::writeIO(u32 address, u32 data) -> void {
  address = (address & 0xfffff) >> 2;

  if(address == 0) {
    //MI_INIT_MODE
  }

  if(address == 1) {
    //MI_VERSION
  }

  if(address == 2) {
    //MI_INTR
  }

  if(address == 3) {
    //MI_INTR_MASK
  }

  print("* ", registerNames(address, "MI_UNKNOWN"), " <= ", hex(data, 8L), "\n");
}
