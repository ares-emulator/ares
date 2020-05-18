#define CF fpu.csr.compare
#define FD(type) fgr<type>(OP >>  6 & 31)
#define FS(type) fgr<type>(OP >> 11 & 31)
#define FT(type) fgr<type>(OP >> 16 & 31)
#define FPU_SINGLE  ((OP & 1 << 21) == 0)
#define FPU_DOUBLE  ((OP & 1 << 21) != 0)
#define FPU_REAL    ((OP & 1 << 23) == 0)
#define FPU_INTEGER ((OP & 1 << 23) != 0)

auto CPU::instructionBC1() -> void {
  bool condition = OP >> 16 & 1;
  bool likely    = OP >> 17 & 1;
  if(CF == condition) branch.take(PC + 4 + (IMMi16 << 2));
  else if(likely) branch.discard();
}

auto CPU::instructionCFC1() -> void {
  RT.u64 = i32(getControlRegisterFPU(RDn));
}

auto CPU::instructionCTC1() -> void {
  setControlRegisterFPU(RDn, RT.u32);
}

auto CPU::instructionDMFC1() -> void {
  RT.u64 = FS(u64);
}

auto CPU::instructionDMTC1() -> void {
  FS(u64) = RT.u64;
}

auto CPU::instructionFABS() -> void {
  if(FPU_SINGLE) {
    FD(f32) = fabs(FS(f32));
  } else {
    FD(f64) = fabs(FS(f64));
  }
}

auto CPU::instructionFADD() -> void {
  if(FPU_SINGLE) {
    FD(f32) = FS(f32) + FT(f32);
  } else {
    FD(f64) = FS(f64) + FT(f64);
  }
}

auto CPU::instructionFCEILL() -> void {
  if(FPU_SINGLE) {
    FD(i64) = ceil(FS(f32));
  } else {
    FD(i64) = ceil(FS(f64));
  }
}

auto CPU::instructionFCEILW() -> void {
  if(FPU_SINGLE) {
    FD(i32) = ceil(FS(f32));
  } else {
    FD(i32) = ceil(FS(f64));
  }
}

auto CPU::instructionFCEQ() -> void {
  if(FPU_SINGLE) {
    CF = FS(f32) == FT(f32);
  } else {
    CF = FS(f64) == FT(f64);
  }
}

auto CPU::instructionFCF() -> void {
  CF = 0;
}

auto CPU::instructionFCOLE() -> void {
  if(FPU_SINGLE) {
    CF = FS(f32) <= FT(f32);
  } else {
    CF = FS(f64) <= FT(f64);
  }
}

auto CPU::instructionFCOLT() -> void {
  if(FPU_SINGLE) {
    CF = FS(f32) < FT(f32);
  } else {
    CF = FS(f64) < FT(f64);
  }
}

auto CPU::instructionFCUEQ() -> void {
  if(FPU_SINGLE) {
    CF = FS(f32) == FT(f32);
  } else {
    CF = FS(f64) == FT(f64);
  }
}

auto CPU::instructionFCULE() -> void {
  if(FPU_SINGLE) {
    CF = FS(f32) <= FT(f32);
  } else {
    CF = FS(f64) <= FT(f64);
  }
}

auto CPU::instructionFCULT() -> void {
  if(FPU_SINGLE) {
    CF = FS(f32) < FT(f32);
  } else {
    CF = FS(f64) < FT(f64);
  }
}

auto CPU::instructionFCUN() -> void {
  CF = 0;
}

auto CPU::instructionFCVTS() -> void {
  if(FPU_REAL) {
    FD(f32) = FS(f64);
  } else if(FPU_SINGLE) {
    FD(f32) = FS(i32);
  } else {
    FD(f32) = FS(i64);
  }
}

auto CPU::instructionFCVTD() -> void {
  if(FPU_REAL) {
    FD(f64) = FS(f32);
  } else if(FPU_SINGLE) {
    FD(f64) = FS(i32);
  } else {
    FD(f64) = FS(i64);
  }
}

auto CPU::instructionFCVTL() -> void {
  if(FPU_SINGLE) {
    FD(i64) = FS(f32);
  } else {
    FD(i64) = FS(f64);
  }
}

auto CPU::instructionFCVTW() -> void {
  if(FPU_SINGLE) {
    FD(i32) = FS(f32);
  } else {
    FD(i32) = FS(f64);
  }
}

auto CPU::instructionFDIV() -> void {
  if(FPU_SINGLE) {
    if(FT(f32)) {
      FD(f32) = FS(f32) / FT(f32);
    } else if(fpu.csr.enable.divisionByZero) {
      fpu.csr.flag.divisionByZero = 1;
      fpu.csr.cause.divisionByZero = 1;
      exception.floatingPoint();
    }
  } else {
    if(FT(f64)) {
      FD(f64) = FS(f64) / FT(f64);
    } else if(fpu.csr.enable.divisionByZero) {
      fpu.csr.flag.divisionByZero = 1;
      fpu.csr.cause.divisionByZero = 1;
      exception.floatingPoint();
    }
  }
}

auto CPU::instructionFFLOORL() -> void {
  if(FPU_SINGLE) {
    FD(i64) = floor(FS(f32));
  } else {
    FD(i64) = floor(FS(f64));
  }
}

auto CPU::instructionFFLOORW() -> void {
  if(FPU_SINGLE) {
    FD(i32) = floor(FS(f32));
  } else {
    FD(i32) = floor(FS(f64));
  }
}

auto CPU::instructionFMOV() -> void {
  if(FPU_SINGLE) {
    FD(f32) = FS(f32);
  } else {
    FD(f64) = FS(f64);
  }
}

auto CPU::instructionFMUL() -> void {
  if(FPU_SINGLE) {
    FD(f32) = FS(f32) * FT(f32);
  } else {
    FD(f64) = FS(f64) * FT(f64);
  }
}

auto CPU::instructionFNEG() -> void {
  if(FPU_SINGLE) {
    FD(f32) = -FS(f32);
  } else {
    FD(f64) = -FS(f64);
  }
}

auto CPU::instructionFROUNDL() -> void {
  if(FPU_SINGLE) {
    FD(i64) = nearbyint(FS(f32));
  } else {
    FD(i64) = nearbyint(FS(f64));
  }
}

auto CPU::instructionFROUNDW() -> void {
  if(FPU_SINGLE) {
    FD(i32) = nearbyint(FS(f32));
  } else {
    FD(i32) = nearbyint(FS(f64));
  }
}

auto CPU::instructionFSQRT() -> void {
  if(FPU_SINGLE) {
    FD(f32) = sqrt(FS(f32));
  } else {
    FD(f64) = sqrt(FS(f64));
  }
}

auto CPU::instructionFSUB() -> void {
  if(FPU_SINGLE) {
    FD(f32) = FS(f32) - FT(f32);
  } else {
    FD(f64) = FS(f64) - FT(f64);
  }
}

auto CPU::instructionFTRUNCL() -> void {
  if(FPU_SINGLE) {
    FD(i64) = FS(f32) < 0 ? ceil(FS(f32)) : floor(FS(f32));
  } else {
    FD(i64) = FS(f64) < 0 ? ceil(FS(f64)) : floor(FS(f64));
  }
}

auto CPU::instructionFTRUNCW() -> void {
  if(FPU_SINGLE) {
    FD(i32) = FS(f32) < 0 ? ceil(FS(f32)) : floor(FS(f32));
  } else {
    FD(i32) = FS(f64) < 0 ? ceil(FS(f64)) : floor(FS(f64));
  }
}

auto CPU::instructionLDC1() -> void {
  if(!scc.status.enable.coprocessor1) return exception.coprocessor1();
  if(auto data = readDouble(RS.u32 + IMMi16)) FT(u64) = *data;
}

auto CPU::instructionLWC1() -> void {
  if(!scc.status.enable.coprocessor1) return exception.coprocessor1();
  if(auto data = readWord(RS.u32 + IMMi16)) FT(u32) = *data;
}

auto CPU::instructionMFC1() -> void {
  RT.u64 = FS(i32);
}

auto CPU::instructionMTC1() -> void {
  FS(i32) = RT.u32;
}

auto CPU::instructionSDC1() -> void {
  if(!scc.status.enable.coprocessor1) return exception.coprocessor1();
  writeDouble(RS.u32 + IMMi16, FT(u64));
}

auto CPU::instructionSWC1() -> void {
  if(!scc.status.enable.coprocessor1) return exception.coprocessor1();
  writeWord(RS.u32 + IMMi16, FT(u32));
}

#undef CF
#undef FD
#undef FS
#undef FT
#undef FPU_SINGLE
#undef FPU_DOUBLE
#undef FPU_REAL
#undef FPU_INTEGER
