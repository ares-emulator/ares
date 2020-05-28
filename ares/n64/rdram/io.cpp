static const vector<string> registerNames = {
  "RDRAM_CONFIG",
  "RDRAM_DEVICE_ID",
  "RDRAM_DELAY",
  "RDRAM_MODE",
  "RDRAM_REF_INTERVAL",
  "RDRAM_REF_ROW",
  "RDRAM_RAS_INTERVAL",
  "RDRAM_MIN_INTERVAL",
  "RDRAM_ADDR_SELECT",
  "RDRAM_DEVICE_MANUF",
};

auto RDRAM::readWord(u32 address) -> u32 {
  address = (address & 0xfffff) >> 2;
  u32 data = 0;

  if(address == 0) {
    //RDRAM_CONFIG
  }

  if(address == 1) {
    //RDRAM_DEVICE_ID
  }

  if(address == 2) {
    //RDRAM_DELAY
  }

  if(address == 3) {
    //RDRAM_MODE
  }

  if(address == 4) {
    //RDRAM_REF_INTERVAL
  }

  if(address == 5) {
    //RDRAM_REF_ROW
  }

  if(address == 6) {
    //RDRAM_RAS_INTERVAL
  }

  if(address == 7) {
    //RDRAM_MIN_INTERVAL
  }

  if(address == 8) {
    //RDRAM_ADDR_SELECT
  }

  if(address == 9) {
    //RDRAM_DEVICE_MANUF
  }

  if(debugger.tracer.io->enabled()) {
    debugger.io({registerNames(address, "RDRAM_UNKNOWN"), " => ", hex(data, 8L)});
  }
  return data;
}

auto RDRAM::writeWord(u32 address, u32 data) -> void {
  address = (address & 0xfffff) >> 2;

  if(address == 0) {
    //RDRAM_CONFIG
  }

  if(address == 1) {
    //RDRAM_DEVICE_ID
  }

  if(address == 2) {
    //RDRAM_DELAY
  }

  if(address == 3) {
    //RDRAM_MODE
  }

  if(address == 4) {
    //RDRAM_REF_INTERVAL
  }

  if(address == 5) {
    //RDRAM_REF_ROW
  }

  if(address == 6) {
    //RDRAM_RAS_INTERVAL
  }

  if(address == 7) {
    //RDRAM_MIN_INTERVAL
  }

  if(address == 8) {
    //RDRAM_ADDR_SELECT
  }

  if(address == 9) {
    //RDRAM_DEVICE_MANUF
  }

  if(debugger.tracer.io->enabled()) {
    debugger.io({registerNames(address, "RDRAM_UNKNOWN"), " <= ", hex(data, 8L)});
  }
}
