static const vector<string> spanRegisterNames = {
  "DPS_TBIST",
  "DPS_TEST_MODE",
  "DPS_BUFTEST_ADDR",
  "DPS_BUFTEST_DATA",
};

auto RDP::readSpan(u32 address) -> u32 {
  address = (address & 0xfffff) >> 2;
  u32 data = 0;

  if(address == 0) {
    //DPS_TBIST
  }

  if(address == 1) {
    //DPS_TEST_MODE
  }

  if(address == 2) {
    //DPS_BUFTEST_ADDR
  }

  if(address == 3) {
    //DPS_BUFTEST_DATA
  }

  print("* ", spanRegisterNames(address, "DPS_UNKNOWN"), " => ", hex(data, 8L), "\n");
  return data;
}

auto RDP::writeSpan(u32 address, u32 data) -> void {
  address = (address & 0xfffff) >> 2;

  if(address == 0) {
    //DPS_TBIST
  }

  if(address == 1) {
    //DPS_TEST_MODE
  }

  if(address == 2) {
    //DPS_BUFTEST_ADDR
  }

  if(address == 3) {
    //DPS_BUFTEST_DATA
  }

  print("* ", spanRegisterNames(address, "DPS_UNKNOWN"), " <= ", hex(data, 8L), "\n");
}
