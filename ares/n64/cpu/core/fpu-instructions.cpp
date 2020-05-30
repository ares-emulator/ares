#define CF fpu.csr.compare
#define FD(type) fgr<type>(fd)
#define FS(type) fgr<type>(fs)
#define FT(type) fgr<type>(ft)

auto CPU::instructionBC1(bool value, bool likely, i16 imm) -> void {
  if(CF == value) branch.take(PC + 4 + (imm << 2));
  else if(likely) branch.discard();
}

auto CPU::instructionCFC1(r64& rt, u8 rd) -> void {
  rt.u64 = i32(getControlRegisterFPU(rd));
}

auto CPU::instructionCTC1(cr64& rt, u8 rd) -> void {
  setControlRegisterFPU(rd, rt.u32);
}

auto CPU::instructionDMFC1(r64& rt, u8 fs) -> void {
  rt.u64 = FS(u64);
}

auto CPU::instructionDMTC1(cr64& rt, u8 fs) -> void {
  FS(u64) = rt.u64;
}

auto CPU::instructionFABS_S(u8 fd, u8 fs) -> void { FD(f32) = fabs(FS(f32)); }
auto CPU::instructionFABS_D(u8 fd, u8 fs) -> void { FD(f64) = fabs(FS(f64)); }

auto CPU::instructionFADD_S(u8 fd, u8 fs, u8 ft) -> void { FD(f32) = FS(f32) + FT(f32); }
auto CPU::instructionFADD_D(u8 fd, u8 fs, u8 ft) -> void { FD(f64) = FS(f64) + FT(f64); }

auto CPU::instructionFCEIL_L_S(u8 fd, u8 fs) -> void { FD(i64) = ceil(FS(f32)); }
auto CPU::instructionFCEIL_L_D(u8 fd, u8 fs) -> void { FD(i64) = ceil(FS(f64)); }

auto CPU::instructionFCEIL_W_S(u8 fd, u8 fs) -> void { FD(i32) = ceil(FS(f32)); }
auto CPU::instructionFCEIL_W_D(u8 fd, u8 fs) -> void { FD(i32) = ceil(FS(f64)); }

auto CPU::instructionFCEQ_S(u8 fs, u8 ft) -> void { CF = FS(f32) == FT(f32); }
auto CPU::instructionFCEQ_D(u8 fs, u8 ft) -> void { CF = FS(f64) == FT(f64); }

auto CPU::instructionFCF() -> void {
  CF = 0;
}

auto CPU::instructionFCOLE_S(u8 fs, u8 ft) -> void { CF = FS(f32) <= FT(f32); }
auto CPU::instructionFCOLE_D(u8 fs, u8 ft) -> void { CF = FS(f64) <= FT(f64); }

auto CPU::instructionFCOLT_S(u8 fs, u8 ft) -> void { CF = FS(f32) < FT(f32); }
auto CPU::instructionFCOLT_D(u8 fs, u8 ft) -> void { CF = FS(f64) < FT(f64); }

auto CPU::instructionFCUEQ_S(u8 fs, u8 ft) -> void { CF = FS(f32) == FT(f32); }
auto CPU::instructionFCUEQ_D(u8 fs, u8 ft) -> void { CF = FS(f64) == FT(f64); }

auto CPU::instructionFCULE_S(u8 fs, u8 ft) -> void { CF = FS(f32) <= FT(f32); }
auto CPU::instructionFCULE_D(u8 fs, u8 ft) -> void { CF = FS(f64) <= FT(f64); }

auto CPU::instructionFCULT_S(u8 fs, u8 ft) -> void { CF = FS(f32) < FT(f32); }
auto CPU::instructionFCULT_D(u8 fs, u8 ft) -> void { CF = FS(f64) < FT(f64); }

auto CPU::instructionFCUN() -> void {
  CF = 0;
}

auto CPU::instructionFCVT_S_D(u8 fd, u8 fs) -> void { FD(f32) = FS(f64); }
auto CPU::instructionFCVT_S_W(u8 fd, u8 fs) -> void { FD(f32) = FS(i32); }
auto CPU::instructionFCVT_S_L(u8 fd, u8 fs) -> void { FD(f32) = FS(i64); }

auto CPU::instructionFCVT_D_S(u8 fd, u8 fs) -> void { FD(f64) = FS(f32); }
auto CPU::instructionFCVT_D_W(u8 fd, u8 fs) -> void { FD(f64) = FS(i32); }
auto CPU::instructionFCVT_D_L(u8 fd, u8 fs) -> void { FD(f64) = FS(i64); }

auto CPU::instructionFCVT_L_S(u8 fd, u8 fs) -> void { FD(i64) = FS(f32); }
auto CPU::instructionFCVT_L_D(u8 fd, u8 fs) -> void { FD(i64) = FS(f64); }

auto CPU::instructionFCVT_W_S(u8 fd, u8 fs) -> void { FD(i32) = FS(f32); }
auto CPU::instructionFCVT_W_D(u8 fd, u8 fs) -> void { FD(i32) = FS(f64); }

auto CPU::instructionFDIV_S(u8 fd, u8 fs, u8 ft) -> void {
  if(FT(f32)) {
    FD(f32) = FS(f32) / FT(f32);
  } else if(fpu.csr.enable.divisionByZero) {
    fpu.csr.flag.divisionByZero = 1;
    fpu.csr.cause.divisionByZero = 1;
    exception.floatingPoint();
  }
}

auto CPU::instructionFDIV_D(u8 fd, u8 fs, u8 ft) -> void {
  if(FT(f64)) {
    FD(f64) = FS(f64) / FT(f64);
  } else if(fpu.csr.enable.divisionByZero) {
    fpu.csr.flag.divisionByZero = 1;
    fpu.csr.cause.divisionByZero = 1;
    exception.floatingPoint();
  }
}

auto CPU::instructionFFLOOR_L_S(u8 fd, u8 fs) -> void { FD(i64) = floor(FS(f32)); }
auto CPU::instructionFFLOOR_L_D(u8 fd, u8 fs) -> void { FD(i64) = floor(FS(f64)); }

auto CPU::instructionFFLOOR_W_S(u8 fd, u8 fs) -> void { FD(i32) = floor(FS(f32)); }
auto CPU::instructionFFLOOR_W_D(u8 fd, u8 fs) -> void { FD(i32) = floor(FS(f64)); }

auto CPU::instructionFMOV_S(u8 fd, u8 fs) -> void { FD(f32) = FS(f32); }
auto CPU::instructionFMOV_D(u8 fd, u8 fs) -> void { FD(f64) = FS(f64); }

auto CPU::instructionFMUL_S(u8 fd, u8 fs, u8 ft) -> void { FD(f32) = FS(f32) * FT(f32); }
auto CPU::instructionFMUL_D(u8 fd, u8 fs, u8 ft) -> void { FD(f64) = FS(f64) * FT(f64); }

auto CPU::instructionFNEG_S(u8 fd, u8 fs) -> void { FD(f32) = -FS(f32); }
auto CPU::instructionFNEG_D(u8 fd, u8 fs) -> void { FD(f64) = -FS(f64); }

auto CPU::instructionFROUND_L_S(u8 fd, u8 fs) -> void { FD(i64) = nearbyint(FS(f32)); }
auto CPU::instructionFROUND_L_D(u8 fd, u8 fs) -> void { FD(i64) = nearbyint(FS(f64)); }

auto CPU::instructionFROUND_W_S(u8 fd, u8 fs) -> void { FD(i32) = nearbyint(FS(f32)); }
auto CPU::instructionFROUND_W_D(u8 fd, u8 fs) -> void { FD(i32) = nearbyint(FS(f64)); }

auto CPU::instructionFSQRT_S(u8 fd, u8 fs) -> void { FD(f32) = sqrt(FS(f32)); }
auto CPU::instructionFSQRT_D(u8 fd, u8 fs) -> void { FD(f64) = sqrt(FS(f64)); }

auto CPU::instructionFSUB_S(u8 fd, u8 fs, u8 ft) -> void { FD(f32) = FS(f32) - FT(f32); }
auto CPU::instructionFSUB_D(u8 fd, u8 fs, u8 ft) -> void { FD(f64) = FS(f64) - FT(f64); }

auto CPU::instructionFTRUNC_L_S(u8 fd, u8 fs) -> void { FD(i64) = FS(f32) < 0 ? ceil(FS(f32)) : floor(FS(f32)); }
auto CPU::instructionFTRUNC_L_D(u8 fd, u8 fs) -> void { FD(i64) = FS(f64) < 0 ? ceil(FS(f64)) : floor(FS(f64)); }

auto CPU::instructionFTRUNC_W_S(u8 fd, u8 fs) -> void { FD(i32) = FS(f32) < 0 ? ceil(FS(f32)) : floor(FS(f32)); }
auto CPU::instructionFTRUNC_W_D(u8 fd, u8 fs) -> void { FD(i32) = FS(f64) < 0 ? ceil(FS(f64)) : floor(FS(f64)); }

auto CPU::instructionLDC1(u8 ft, cr64& rs, i16 imm) -> void {
  if(!scc.status.enable.coprocessor1) return exception.coprocessor1();
  if(auto data = readDouble(rs.u32 + imm)) FT(u64) = *data;
}

auto CPU::instructionLWC1(u8 ft, cr64& rs, i16 imm) -> void {
  if(!scc.status.enable.coprocessor1) return exception.coprocessor1();
  if(auto data = readWord(rs.u32 + imm)) FT(u32) = *data;
}

auto CPU::instructionMFC1(r64& rt, u8 fs) -> void {
  rt.u64 = FS(i32);
}

auto CPU::instructionMTC1(cr64& rt, u8 fs) -> void {
  FS(i32) = rt.u32;
}

auto CPU::instructionSDC1(u8 ft, cr64& rs, i16 imm) -> void {
  if(!scc.status.enable.coprocessor1) return exception.coprocessor1();
  writeDouble(rs.u32 + imm, FT(u64));
}

auto CPU::instructionSWC1(u8 ft, cr64& rs, i16 imm) -> void {
  if(!scc.status.enable.coprocessor1) return exception.coprocessor1();
  writeWord(rs.u32 + imm, FT(u32));
}

#undef CF
#undef FD
#undef FS
#undef FT
