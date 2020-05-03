static const vector<string> registerNames = {
  "AI_DRAM_ADDR",
  "AI_LEN",
  "AI_CONTROL",
  "AI_STATUS",
  "AI_DACRATE",
  "AI_BITRATE",
};

auto AI::readIO(u32 address) -> u32 {
  address = (address & 0xfffff) >> 2;
  u32 data = 0;

  if(address == 0) {
    //AI_DRAM_ADDR
  }

  if(address == 1) {
    //AI_LEN
  }

  if(address == 2) {
    //AI_CONTROL
  }

  if(address == 3) {
    //AI_STATUS
  }

  if(address == 4) {
    //AI_DACRATE
  }

  if(address == 5) {
    //AI_BITRATE
  }

  print("* ", registerNames(address, "AI_UNKNOWN"), " => ", hex(data, 8L), "\n");
  return data;
}

auto AI::writeIO(u32 address, u32 data) -> void {
  address = (address & 0xfffff) >> 2;

  if(address == 0) {
    //AI_DRAM_ADDR
  }

  if(address == 1) {
    //AI_LEN
  }

  if(address == 2) {
    //AI_CONTROL
  }

  if(address == 3) {
    //AI_STATUS
  }

  if(address == 4) {
    //AI_DACRATE
  }

  if(address == 5) {
    //AI_BITRATE
  }

  print("* ", registerNames(address, "AI_UNKNOWN"), " <= ", hex(data, 8L), "\n");
}
