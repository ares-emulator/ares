auto RI::readWord(u32 address, Thread& thread) -> u32 {
  address = (address & 0x1f) >> 2;
  n32 data = 0;

  if(address == 0) {
    //RI_MODE
    data = io.mode;
  }

  if(address == 1) {
    //RI_CONFIG
    data = io.config;
  }

  if(address == 2) {
    //RI_CURRENT_LOAD (write-only; unintended read returns mixed bits)
    data.bit(0) = io.error.bit(0);
    data.bit(1) = 1;
    data.bit(2) = 1;
    data.bit(3) = io.mode.bit(3);
    data.bit(4) = io.select.bit(4);
  }

  if(address == 3) {
    //RI_SELECT
    data = io.select;
  }

  if(address == 4) {
    //RI_REFRESH
    data = io.refresh;
  }

  if(address == 5) {
    //RI_LATENCY
    data = io.latency;
  }

  if(address == 6) {
    //RI_ERROR
    data = io.error;
  }

  if(address == 7) {
    //RI_BANK_STATUS
    data = io.bankStatus;
  }

  debugger.io(Read, address, data);
  return data;
}

auto RI::writeWord(u32 address, u32 data_, Thread& thread) -> void {
  address = (address & 0x1f) >> 2;
  n32 data = data_;

  if(address == 0) {
    //RI_MODE
    io.mode = data;
  }

  if(address == 1) {
    //RI_CONFIG
    io.config = data;
  }

  if(address == 2) {
    //RI_CURRENT_LOAD
    io.currentLoad = data;
    io.currentLoaded = 1;
  }

  if(address == 3) {
    //RI_SELECT
    io.select = data;
  }

  if(address == 4) {
    //RI_REFRESH
    io.refresh = data;
  }

  if(address == 5) {
    //RI_LATENCY
    io.latency = data;
  }

  if(address == 6) {
    //RI_ERROR
    io.error = 0;
  }

  if(address == 7) {
    //RI_BANK_STATUS
    io.bankStatus = 0x00ff;
  }

  debugger.io(Write, address, data);
}
