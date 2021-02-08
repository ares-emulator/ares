auto uPD96050::readSR() -> n8 {
  return regs.sr >> 8;
}

auto uPD96050::writeSR(n8 data) -> void {
}

auto uPD96050::readDR() -> n8 {
  if(regs.sr.drc == 0) {
    //16-bit
    if(regs.sr.drs == 0) {
      regs.sr.drs = 1;
      return regs.dr >> 0;
    } else {
      regs.sr.rqm = 0;
      regs.sr.drs = 0;
      return regs.dr >> 8;
    }
  } else {
    //8-bit
    regs.sr.rqm = 0;
    return regs.dr >> 0;
  }
}

auto uPD96050::writeDR(n8 data) -> void {
  if(regs.sr.drc == 0) {
    //16-bit
    if(regs.sr.drs == 0) {
      regs.sr.drs = 1;
      regs.dr = (regs.dr & 0xff00) | (data << 0);
    } else {
      regs.sr.rqm = 0;
      regs.sr.drs = 0;
      regs.dr = (data << 8) | (regs.dr & 0x00ff);
    }
  } else {
    //8-bit
    regs.sr.rqm = 0;
    regs.dr = (regs.dr & 0xff00) | (data << 0);
  }
}

auto uPD96050::readDP(n12 address) -> n8 {
  bool hi = address & 1;
  address = (address >> 1) & 2047;

  if(hi == false) {
    return dataRAM[address] >> 0;
  } else {
    return dataRAM[address] >> 8;
  }
}

auto uPD96050::writeDP(n12 address, n8 data) -> void {
  bool hi = address & 1;
  address = (address >> 1) & 2047;

  if(hi == false) {
    dataRAM[address] = (dataRAM[address] & 0xff00) | (data << 0);
  } else {
    dataRAM[address] = (dataRAM[address] & 0x00ff) | (data << 8);
  }
}
