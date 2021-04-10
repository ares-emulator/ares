auto RI::readWord(u32 address) -> u32 {
  address = (address & 0xfffff) >> 2;
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
    //RI_CURRENT_LOAD
    data = io.currentLoad;
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
    //RI_RERROR
    data = io.readError;
  }

  if(address == 7) {
    //RI_WERROR
    data = io.writeError;
  }

  debugger.io(Read, address, data);
  return data;
}

auto RI::writeWord(u32 address, u32 data_) -> void {
  address = (address & 0xfffff) >> 2;
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
    //RI_RERROR
    io.readError = data;
  }

  if(address == 7) {
    //RI_WERROR
    io.writeError = data;
  }

  debugger.io(Write, address, data);
}
