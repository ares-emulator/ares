auto RDRAM::readIO(u32 address) -> u32 {
  uint32 data = 0;

  if(address == 0x0470'0000) {
    //RI_MODE
    return data;
  }

  if(address == 0x0470'0004) {
    //RI_CONFIG
    data |= io.currentControlInput  << 0;
    data |= io.currentControlEnable << 6;
    return data;
  }

  if(address == 0x0470'0008) {
    //RI_CURRENT_LOAD
    return data;
  }

  if(address == 0x0470'000c) {
    //RI_SELECT
    return data;
  }

  if(address == 0x0470'0010) {
    //RI_REFRESH
    return data;
  }

  if(address == 0x0470'0014) {
    //RI_LATENCY
    return data;
  }

  if(address == 0x0470'0018) {
    //RI_WERROR
    return data;
  }

  return data;
}

auto RDRAM::writeIO(u32 address, u32 data) -> void {
  if(address == 0x0470'0000) {
    //RI_MODE
    return;
  }

  if(address == 0x0470'0004) {
    //RI_CONFIG
    io.currentControlInput  = data >> 0 & 63;
    io.currentControlEnable = data >> 6 & 1;
    return;
  }

  if(address == 0x0470'0008) {
    //RI_CURRENT_LOAD
    return;
  }

  if(address == 0x0470'000c) {
    //RI_SELECT
    return;
  }

  if(address == 0x0470'0010) {
    //RI_REFRESH
    return;
  }

  if(address == 0x0470'0014) {
    //RI_LATENCY
    return;
  }

  if(address == 0x0470'0018) {
    //RI_WERROR
    return;
  }
}
