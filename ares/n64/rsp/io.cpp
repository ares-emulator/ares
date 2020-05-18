static const vector<string> registerNames = {
  "SP_PC_REG",
  "SP_IBIST",
};

auto RSP::readIO(u32 address) -> u32 {
  address = (address & 0x7ffff) >> 2;
  uint32 data;

  if(address == 0) {
    //SP_PC_REG
    data.bit(0,11) = core.pc;
  }

  if(address == 1) {
    //SP_IBIST
  }

//print("* ", registerNames(address, "SP_UNKNOWN"), " => ", hex(data, 8L), "\n");
  return data;
}

auto RSP::writeIO(u32 address, uint32 data) -> void {
  address = (address & 0x7ffff) >> 2;

  if(address == 0) {
    //SP_PC_REG
    core.pc = data.bit(0,11);
  }

  if(address == 1) {
    //SP_IBIST
  }

//print("* ", registerNames(address, "SP_UNKNOWN"), " <= ", hex(data, 8L), "\n");
}
