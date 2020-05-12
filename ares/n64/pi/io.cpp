static const vector<string> registerNames = {
  "PI_DRAM_ADDRESS",
  "PI_PBUS_ADDRESS",
  "PI_READ_LENGTH",
  "PI_WRITE_LENGTH",
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
  uint32 data;

  if(address == 0) {
    //PI_DRAM_ADDRESS
    data = io.dramAddress;
  }

  if(address == 1) {
    //PI_CART_ADDRESS
    data = io.pbusAddress;
  }

  if(address == 2) {
    //PI_READ_LENGTH
    data = io.readLength;
  }

  if(address == 3) {
    //PI_WRITE_LENGTH
    data = io.writeLength;
  }

  if(address == 4) {
    //PI_STATUS
    data.bit(0) = io.dmaBusy;
    data.bit(1) = io.ioBusy;
    data.bit(2) = io.error;
    data.bit(3) = io.interrupt;
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

//print("* ", registerNames(address, "PI_UNKNOWN"), " => ", hex(data, 8L), "\n");
  return data;
}

auto PI::writeIO(u32 address, uint32 data) -> void {
  address = (address & 0xfffff) >> 2;

  //only PI_STATUS can be written while PI is busy
  if(address != 4 && (io.dmaBusy || io.ioBusy)) {
    io.error = 1;
    return;
  }

  if(address == 0) {
    //PI_DRAM_ADDRESS
    io.dramAddress = uint24(data) & ~7;
  }

  if(address == 1) {
    //PI_PBUS_ADDRESS
    io.pbusAddress = uint29(data) & ~1;
  }

  if(address == 2) {
    //PI_READ_LENGTH
    io.readLength = (uint24(data) | 7) + 1;
    for(u32 address = 0; address < io.readLength; address += 2) {
      u16 data = rdram.ram.readHalf(io.dramAddress + address);
      bus.writeHalf(io.pbusAddress + address, data);
    }
    io.interrupt = 1;
    mi.raise(MI::IRQ::PI);
  }

  if(address == 3) {
    //PI_WRITE_LENGTH
    io.writeLength = (uint24(data) | 7) + 1;
    for(u32 address = 0; address < io.writeLength; address += 2) {
      u16 data = bus.readHalf(io.pbusAddress + address);
      rdram.ram.writeHalf(io.dramAddress + address, data);
    }
    io.interrupt = 1;
    mi.raise(MI::IRQ::PI);
  }

  if(address == 4) {
    //PI_STATUS
    if(data.bit(0)) io.error = 0;
    if(data.bit(1)) io.interrupt = 0, mi.lower(MI::IRQ::PI);
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

//print("* ", registerNames(address, "PI_UNKNOWN"), " <= ", hex(data, 8L), "\n");
}
