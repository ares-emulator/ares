static const vector<string> registerNames = {
  "PI_DRAM_ADDR",
  "PI_CART_ADDR",
  "PI_RD_LEN",
  "PI_WR_LEN",
  "PI_STATUS",
  "PI_BSD_DOM1_LAT",
  "PI_BSD_DOM1_PWD",
  "PI_BSD_DOM1_PGS",
  "PI_BSD_DOM1_RLS",
  "PI_BSD_DOM2_LAT",
  "PI_BSD_DOM2_PWD",
  "PI_BSD_DOM2_PGS",
  "PI_BSD_DOM2_RLS",
};

auto PI::readIO(u32 address) -> u32 {
  address = (address & 0xfffff) >> 2;
  u32 data = 0;

  if(address == 0) {
    //PI_DRAM_ADDR
    data = io.dramAddress;
  }

  if(address == 1) {
    //PI_CART_ADDR
    data = io.cartAddress;
  }

  if(address == 2) {
    //PI_RD_LEN
    data = io.readLength;
  }

  if(address == 3) {
    //PI_WR_LEN
    data = io.writeLength;
  }

  if(address == 4) {
    //PI_STATUS
  }

  if(address == 5) {
    //PI_BSD_DOM1_LAT
  }

  if(address == 6) {
    //PI_BSD_DOM1_PWD
  }

  if(address == 7) {
    //PI_BSD_DOM1_PGS
  }

  if(address == 8) {
    //PI_BSD_DOM1_RLS
  }

  if(address == 9) {
    //PI_BSD_DOM2_LAT
  }

  if(address == 10) {
    //PI_BSD_DOM2_PWD
  }

  if(address == 11) {
    //PI_BSD_DOM2_PGS
  }

  if(address == 12) {
    //PI_BSD_DOM2_RLS
  }

  print("* ", registerNames(address, "PI_UNKNOWN"), " => ", hex(data, 8L), "\n");
  return data;
}

auto PI::writeIO(u32 address, u32 data) -> void {
  address = (address & 0xfffff) >> 2;

  if(address == 0) {
    //PI_DRAM_ADDR
    io.dramAddress = uint24(data);
  }

  if(address == 1) {
    //PI_CART_ADDR
    io.cartAddress = uint29(data);
  }

  if(address == 2) {
    //PI_RD_LEN
    io.readLength = uint24(data) + 1;
    for(u32 address = 0; address < io.writeLength; address += 4) {
      auto data = bus.readWord(io.dramAddress + address);
      bus.writeWord(io.cartAddress + address, data);
    }
  }

  if(address == 3) {
    //PI_WR_LEN
    io.writeLength = uint24(data) + 1;
    for(u32 address = 0; address < io.writeLength; address += 4) {
      auto data = bus.readWord(io.cartAddress + address);
      bus.writeWord(io.dramAddress + address, data);
    }
  }

  if(address == 4) {
    //PI_STATUS (read-only)
  }

  if(address == 5) {
    //PI_BSD_DOM1_LAT
  }

  if(address == 6) {
    //PI_BSD_DOM1_PWD
  }

  if(address == 7) {
    //PI_BSD_DOM1_PGS
  }

  if(address == 8) {
    //PI_BSD_DOM1_RLS
  }

  if(address == 9) {
    //PI_BSD_DOM2_LAT
  }

  if(address == 10) {
    //PI_BSD_DOM2_PWD
  }

  if(address == 11) {
    //PI_BSD_DOM2_PGS
  }

  if(address == 12) {
    //PI_BSD_DOM2_RLS
  }

  print("* ", registerNames(address, "PI_UNKNOWN"), " <= ", hex(data, 8L), "\n");
}
