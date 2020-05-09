static const vector<string> registerNames = {
  "RI_MODE",
  "RI_CONFIG",
  "RI_CURRENT_LOAD",
  "RI_SELECT",
  "RI_REFRESH",
  "RI_LATENCY",
  "RI_RERROR",
  "RI_WERROR",
};

auto RI::readIO(u32 address) -> u32 {
  address = (address & 0xfffff) >> 2;
  uint32 data = 0;

  if(address == 0) {
    //RI_MODE
  }

  if(address == 1) {
    //RI_CONFIG
  }

  if(address == 2) {
    //RI_CURRENT_LOAD
  }

  if(address == 3) {
    //RI_SELECT
  }

  if(address == 4) {
    //RI_REFRESH
  }

  if(address == 5) {
    //RI_LATENCY
  }

  if(address == 6) {
    //RI_RERROR
  }

  if(address == 7) {
    //RI_WERROR
  }

//print("* ", registerNames(address, "RI_UNKNOWN"), " => ", hex(data, 8L), "\n");
  return data;
}

auto RI::writeIO(u32 address, u32 data) -> void {
  address = (address & 0xfffff) >> 2;

  if(address == 0) {
    //RI_MODE
  }

  if(address == 1) {
    //RI_CONFIG
  }

  if(address == 2) {
    //RI_CURRENT_LOAD
  }

  if(address == 3) {
    //RI_SELECT
  }

  if(address == 4) {
    //RI_REFRESH
  }

  if(address == 5) {
    //RI_LATENCY
  }

  if(address == 6) {
    //RI_RERROR
  }

  if(address == 7) {
    //RI_WERROR
  }

//print("* ", registerNames(address, "RI_UNKNOWN"), " <= ", hex(data, 8L), "\n");
}
