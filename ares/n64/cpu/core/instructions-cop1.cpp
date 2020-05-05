auto CPU::instructionBC1() -> void {
  bool condition = OP >> 16 & 1;
  bool likely    = OP >> 17 & 1;
  if(cop1.cf[pipeline.instruction >> 18 & 7] == condition) IP = PC + (IMMi16 << 2);
  else if(likely) PC += 4;
}

auto CPU::instructionCFC1() -> void {
  static const uint shift[8] = {23, 25, 26, 27, 28, 29, 30, 31};
  if(RDn == 31) {
    u32 result = cop1.cr[31] & ~0xfe80'0000;
    for(uint n : range(8)) {
      if(cop1.cf[n]) result |= 1 << shift[n];
    }
    RT.u64 = i32(result);
    return;
  }
  RT.u64 = i32(cop1.cr[RDn]);
}

auto CPU::instructionCTC1() -> void {
  static const uint shift[8] = {23, 25, 26, 27, 28, 29, 30, 31};
  cop1.cr[RDn] = RT.u32;
  if(RDn == 31) {
    for(uint n : range(8)) {
      cop1.cf[n] = RT.u32 >> shift[n] & 1;
    }
  }
}

auto CPU::instructionDMFC1() -> void {
  RT.u64 = FPR[RDn].u64;
}

auto CPU::instructionDMTC1() -> void {
  FPR[RDn].u64 = RT.u64;
}

auto CPU::instructionFABS() -> void {
  if(FPU_SINGLE) {
    FD.f32 = fabs(FS.f32);
  } else {
    FD.f64 = fabs(FS.f64);
  }
}

auto CPU::instructionFADD() -> void {
  if(FPU_SINGLE) {
    FD.f32 = FS.f32 + FT.f32;
  } else {
    FD.f64 = FS.f64 + FT.f64;
  }
}

auto CPU::instructionFCEILL() -> void {
  if(FPU_SINGLE) {
    FD.u64 = i64(ceil(FS.f32));
  } else {
    FD.u64 = i64(ceil(FS.f64));
  }
}

auto CPU::instructionFCEILW() -> void {
  if(FPU_SINGLE) {
    FD.u32 = i32(ceil(FS.f32));
  } else {
    FD.u32 = i32(ceil(FS.f64));
  }
}

#define CF cop1.cf[pipeline.instruction >> 8 & 7]

auto CPU::instructionFCEQ() -> void {
  if(FPU_SINGLE) {
    CF = FS.f32 == FT.f32;
  } else {
    CF = FS.f64 == FT.f64;
  }
}

auto CPU::instructionFCF() -> void {
  CF = 0;
}

auto CPU::instructionFCOLE() -> void {
  if(FPU_SINGLE) {
    CF = FS.f32 <= FT.f32;
  } else {
    CF = FS.f64 <= FT.f64;
  }
}

auto CPU::instructionFCOLT() -> void {
  if(FPU_SINGLE) {
    CF = FS.f32 < FT.f32;
  } else {
    CF = FS.f64 < FT.f64;
  }
}

auto CPU::instructionFCUEQ() -> void {
  if(FPU_SINGLE) {
    CF = FS.f32 == FT.f32;
  } else {
    CF = FS.f64 == FT.f64;
  }
}

auto CPU::instructionFCULE() -> void {
  if(FPU_SINGLE) {
    CF = FS.f32 <= FT.f32;
  } else {
    CF = FS.f64 <= FT.f64;
  }
}

auto CPU::instructionFCULT() -> void {
  if(FPU_SINGLE) {
    CF = FS.f32 < FT.f32;
  } else {
    CF = FS.f64 < FT.f64;
  }
}

auto CPU::instructionFCUN() -> void {
  CF = 0;
}

#undef CF

auto CPU::instructionFCVTS() -> void {
  if(FPU_INTEGER) {
    if(FPU_SINGLE) {
      FD.f32 = FS.i32;
    } else {
      FD.f32 = FS.i64;
    }
  } else {
    FD.f32 = FS.f64;
  }
}

auto CPU::instructionFCVTD() -> void {
  if(FPU_INTEGER) {
    if(FPU_SINGLE) {
      FD.f64 = FS.i32;
    } else {
      FD.f64 = FS.i64;
    }
  } else {
    FD.f64 = FS.f32;
  }
}

auto CPU::instructionFCVTL() -> void {
  if(FPU_SINGLE) {
    FD.i64 = i64(FS.f32);
  } else {
    FD.i64 = i64(FS.f64);
  }
}

auto CPU::instructionFCVTW() -> void {
  if(FPU_SINGLE) {
    FD.i32 = i32(FS.f32);
  } else {
    FD.i32 = i64(FS.f64);
  }
}

auto CPU::instructionFDIV() -> void {
  if(FPU_SINGLE) {
    if(FT.i32) {
      FD.f32 = FS.f32 / FT.f32;
    } else {
      //todo: check CCR31
      exception(FloatingPointError);
    }
  } else {
    if(FT.i64) {
      FD.f64 = FS.f64 / FT.f64;
    } else {
      //todo: check CCR31
      exception(FloatingPointError);
    }
  }
}

auto CPU::instructionFFLOORL() -> void {
  if(FPU_SINGLE) {
    FD.i64 = i64(floor(FS.f32));
  } else {
    FD.i64 = i64(floor(FS.f64));
  }
}

auto CPU::instructionFFLOORW() -> void {
  if(FPU_SINGLE) {
    FD.i32 = i32(floor(FS.f32));
  } else {
    FD.i32 = i32(floor(FS.f64));
  }
}

auto CPU::instructionFMOV() -> void {
  if(FPU_SINGLE) {
    FD.f32 = FS.f32;
  } else {
    FD.f64 = FS.f64;
  }
}

auto CPU::instructionFMUL() -> void {
  if(FPU_SINGLE) {
    FD.f32 = FS.f32 * FT.f32;
  } else {
    FD.f64 = FS.f64 * FT.f64;
  }
}

auto CPU::instructionFNEG() -> void {
  if(FPU_SINGLE) {
    FD.f32 = -FS.f32;
  } else {
    FD.f64 = -FS.f64;
  }
}

auto CPU::instructionFROUNDL() -> void {
  if(FPU_SINGLE) {
    FD.i64 = i64(nearbyint(FS.f32));
  } else {
    FD.i64 = i64(nearbyint(FS.f64));
  }
}

auto CPU::instructionFROUNDW() -> void {
  if(FPU_SINGLE) {
    FD.i32 = i32(nearbyint(FS.f32));
  } else {
    FD.i32 = i32(nearbyint(FS.f64));
  }
}

auto CPU::instructionFSQRT() -> void {
  if(FPU_SINGLE) {
    FD.f32 = sqrt(FS.f32);
  } else {
    FD.f64 = sqrt(FS.f64);
  }
}

auto CPU::instructionFSUB() -> void {
  if(FPU_SINGLE) {
    FD.f32 = FS.f32 - FT.f32;
  } else {
    FD.f64 = FS.f64 - FT.f64;
  }
}

auto CPU::instructionFTRUNCL() -> void {
  if(FPU_SINGLE) {
    FD.i64 = i64(FS.f32 < 0 ? ceil(FS.f32) : floor(FS.f32));
  } else {
    FD.i64 = i64(FS.f64 < 0 ? ceil(FS.f64) : floor(FS.f64));
  }
}

auto CPU::instructionFTRUNCW() -> void {
  if(FPU_SINGLE) {
    FD.i32 = i32(FS.f32 < 0 ? ceil(FS.f32) : floor(FS.f32));
  } else {
    FD.i32 = i32(FS.f64 < 0 ? ceil(FS.f64) : floor(FS.f64));
  }
}

auto CPU::instructionLWC1() -> void {
  if(!STATUS_COP1) return exception(CoprocessorUnusable);
  if(auto data = readWord(RS.u32 + IMMi16)) cop1.r[RTn].u64 = *data;
}

auto CPU::instructionMFC1() -> void {
  RT.u64 = FPR[RDn].i32;
}

auto CPU::instructionMTC1() -> void {
  FPR[RDn].u32 = RT.u32;
}

auto CPU::instructionSWC1() -> void {
  if(!STATUS_COP1) return exception(CoprocessorUnusable);
  writeWord(RS.u32 + IMMi16, cop1.r[RTn].u32);
}
