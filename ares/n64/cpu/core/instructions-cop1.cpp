auto CPU::getCOP1u32(uint index) -> u32 {
  if(!STATUS_FR) {
    return ((u32*)&cop1.r[index & ~1])[index & 1];
  } else {
    return cop1.r[index];
  }
}

auto CPU::getCOP1u64(uint index) -> u64 {
  if(!STATUS_FR) {
    return cop1.r[index & ~1];
  } else {
    return cop1.r[index];
  }
}

auto CPU::setCOP1u32(uint index, u32 data) -> void {
  if(!STATUS_FR) {
    ((u32*)&cop1.r[index & ~1])[index & 1] = data;
  } else {
    cop1.r[index] = data;
  }
}

auto CPU::setCOP1u64(uint index, u64 data) -> void {
  if(!STATUS_FR) {
    cop1.r[index & ~1] = data;
  } else {
    cop1.r[index] = data;
  }
}

//

auto CPU::instructionBC1F() -> void {
  if(cop1.cf[pipeline.instruction >> 18 & 7] == 0) IP = PC + (IMM16i << 2);
}

auto CPU::instructionBC1FL() -> void {
  if(cop1.cf[pipeline.instruction >> 18 & 7] == 0) IP = PC + (IMM16i << 2);
  else PC += 4;
}

auto CPU::instructionBC1T() -> void {
  if(cop1.cf[pipeline.instruction >> 18 & 7] == 1) IP = PC + (IMM16i << 2);
}

auto CPU::instructionBC1TL() -> void {
  if(cop1.cf[pipeline.instruction >> 18 & 7] == 1) IP = PC + (IMM16i << 2);
  else PC += 4;
}

auto CPU::instructionCFC1() -> void {
  static const uint shift[8] = {23, 25, 26, 27, 28, 29, 30, 31};
  if(RDn == 31) {
    u32 result = cop1.cr[31] & ~0xfe80'0000;
    for(uint n : range(8)) {
      if(cop1.cf[n]) result |= 1 << shift[n];
    }
    wRT32i(result);
    return;
  }
  wRT32i(cop1.cr[RDn]);
}

auto CPU::instructionCTC1() -> void {
  static const uint shift[8] = {23, 25, 26, 27, 28, 29, 30, 31};
  cop1.cr[RDn] = RT32u;
  if(RDn == 31) {
    for(uint n : range(8)) {
      cop1.cf[n] = RT32u >> shift[n] & 1;
    }
  }
}

auto CPU::instructionDMFC1() -> void {
  wRT64u(getCOP1u64(RDn));
}

auto CPU::instructionDMTC1() -> void {
  setCOP1u64(RDn, RT64u);
}

auto CPU::instructionFABS() -> void {
  //todo
}

auto CPU::instructionFADD() -> void {
  //todo
}

auto CPU::instructionFCEILL() -> void {
  //todo
}

auto CPU::instructionFCEILW() -> void {
  //todo
}

auto CPU::instructionFCEQ() -> void {
  //todo
}

auto CPU::instructionFCF() -> void {
  //todo
}

auto CPU::instructionFCOLE() -> void {
  //todo
}

auto CPU::instructionFCOLT() -> void {
  //todo
}

auto CPU::instructionFCUEQ() -> void {
  //todo
}

auto CPU::instructionFCULE() -> void {
  //todo
}

auto CPU::instructionFCULT() -> void {
  //todo
}

auto CPU::instructionFCUN() -> void {
  //todo
}

auto CPU::instructionFCVTS() -> void {
  //todo
}

auto CPU::instructionFCVTD() -> void {
  //todo
}

auto CPU::instructionFCVTL() -> void {
  //todo
}

auto CPU::instructionFCVTW() -> void {
  //todo
}

auto CPU::instructionFDIV() -> void {
  //todo
}

auto CPU::instructionFFLOORL() -> void {
  //todo
}

auto CPU::instructionFFLOORW() -> void {
  //todo
}

auto CPU::instructionFMOV() -> void {
  //todo
}

auto CPU::instructionFMUL() -> void {
  //todo
}

auto CPU::instructionFNEG() -> void {
  //todo
}

auto CPU::instructionFROUNDL() -> void {
  //todo
}

auto CPU::instructionFROUNDW() -> void {
  //todo
}

auto CPU::instructionFSQRT() -> void {
  //todo
}

auto CPU::instructionFSUB() -> void {
  //todo
}

auto CPU::instructionFTRUNCL() -> void {
  //todo
}

auto CPU::instructionFTRUNCW() -> void {
  //todo
}

auto CPU::instructionMFC1() -> void {
  wRT32i(getCOP1u32(RDn));
}

auto CPU::instructionMTC1() -> void {
  setCOP1u32(RDn, RT32u);
}
