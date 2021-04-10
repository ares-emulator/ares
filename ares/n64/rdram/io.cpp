auto RDRAM::readWord(u32 address) -> u32 {
  address = (address & 0xfffff) >> 2;
  u32 data = 0;

  if(address == 0) {
    //RDRAM_CONFIG
    data = io.config;
  }

  if(address == 1) {
    //RDRAM_DEVICE_ID
    data = io.deviceID;
  }

  if(address == 2) {
    //RDRAM_DELAY
    data = io.delay;
  }

  if(address == 3) {
    //RDRAM_MODE
    data = io.mode;
  }

  if(address == 4) {
    //RDRAM_REF_INTERVAL
    data = io.refreshInterval;
  }

  if(address == 5) {
    //RDRAM_REF_ROW
    data = io.refreshRow;
  }

  if(address == 6) {
    //RDRAM_RAS_INTERVAL
    data = io.rasInterval;
  }

  if(address == 7) {
    //RDRAM_MIN_INTERVAL
    data = io.minInterval;
  }

  if(address == 8) {
    //RDRAM_ADDR_SELECT
    data = io.addressSelect;
  }

  if(address == 9) {
    //RDRAM_DEVICE_MANUF
    data = io.deviceManufacturer;
  }

  debugger.io(Read, address, data);
  return data;
}

auto RDRAM::writeWord(u32 address, u32 data) -> void {
  address = (address & 0xfffff) >> 2;

  if(address == 0) {
    //RDRAM_CONFIG
    io.config = data;
  }

  if(address == 1) {
    //RDRAM_DEVICE_ID
    io.deviceID = data;
  }

  if(address == 2) {
    //RDRAM_DELAY
    io.delay = data;
  }

  if(address == 3) {
    //RDRAM_MODE
    io.mode = data;
  }

  if(address == 4) {
    //RDRAM_REF_INTERVAL
    io.refreshInterval = data;
  }

  if(address == 5) {
    //RDRAM_REF_ROW
    io.refreshRow = data;
  }

  if(address == 6) {
    //RDRAM_RAS_INTERVAL
    io.rasInterval = data;
  }

  if(address == 7) {
    //RDRAM_MIN_INTERVAL
    io.minInterval = data;
  }

  if(address == 8) {
    //RDRAM_ADDR_SELECT
    io.addressSelect = data;
  }

  if(address == 9) {
    //RDRAM_DEVICE_MANUF
    io.deviceManufacturer = data;
  }

  debugger.io(Write, address, data);
}
