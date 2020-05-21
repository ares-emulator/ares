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

  if(debugger.tracer.io->enabled()) {
    debugger.io({registerNames(address, "SP_UNKNOWN"), " => ", hex(data, 8L)});
  }
  return data;
}

auto RSP::writeIO(u32 address, uint32 data) -> void {
  address = (address & 0x7ffff) >> 2;

  if(address == 0) {
    //SP_PC_REG
    if(branch.state == Branch::Take) {
      branch.pc = data.bit(0,11) & ~3;
    } else {
      core.pc = data.bit(0,11) & ~3;
    }
  }

  if(address == 1) {
    //SP_IBIST
  }

  if(debugger.tracer.io->enabled()) {
    debugger.io({registerNames(address, "SP_UNKNOWN"), " <= ", hex(data, 8L)});
  }
}
