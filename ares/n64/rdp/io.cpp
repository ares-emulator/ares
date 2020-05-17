static const vector<string> registerNames = {
  "DPS_TBIST",
  "DPS_TEST_MODE",
  "DPS_BUFTEST_ADDR",
  "DPS_BUFTEST_DATA",
};

auto RDP::readIO(u32 address) -> u32 {
  address = (address & 0xfffff) >> 2;
  uint32 data;

  if(address == 0) {
    //DPS_TBIST
    data.bit(0)    = io.bist.check;
    data.bit(1)    = io.bist.go;
    data.bit(2)    = io.bist.done;
    data.bit(3,10) = io.bist.fail;
  }

  if(address == 1) {
    //DPS_TEST_MODE
    data.bit(0) = io.test.enable;
  }

  if(address == 2) {
    //DPS_BUFTEST_ADDR
    data.bit(0,6) = io.test.address;
  }

  if(address == 3) {
    //DPS_BUFTEST_DATA
    data.bit(0,31) = io.test.data;
  }

//print("* ", registerNames(address, "DPS_UNKNOWN"), " => ", hex(data, 8L), "\n");
  return data;
}

auto RDP::writeIO(u32 address, uint32 data) -> void {
  address = (address & 0xfffff) >> 2;

  if(address == 0) {
    //DPS_TBIST
    io.bist.check = data.bit(0);
    io.bist.go    = data.bit(1);
    if(data.bit(2)) io.bist.done = 0;
  }

  if(address == 1) {
    //DPS_TEST_MODE
    io.test.enable = data.bit(0);
  }

  if(address == 2) {
    //DPS_BUFTEST_ADDR
    io.test.address = data.bit(0,6);
  }

  if(address == 3) {
    //DPS_BUFTEST_DATA
    io.test.data = data.bit(0,31);
  }

//print("* ", registerNames(address, "DPS_UNKNOWN"), " <= ", hex(data, 8L), "\n");
}
