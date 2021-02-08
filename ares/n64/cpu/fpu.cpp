auto CPU::FPU::setFloatingPointMode(bool mode) -> void {
  if(mode == 0) {
    //32x64-bit -> 16x64-bit
  } else {
    //16x64-bit -> 32x64-bit
  }
}

template<> auto CPU::fgr<s32>(u32 index) -> s32& {
  if(scc.status.floatingPointMode) {
    return fpu.r[index].s32;
  } else if(index & 1) {
    return fpu.r[index & ~1].s32h;
  } else {
    return fpu.r[index & ~1].s32;
  }
}

template<> auto CPU::fgr<u32>(u32 index) -> u32& {
  return (u32&)fgr<s32>(index);
}

template<> auto CPU::fgr<f32>(u32 index) -> f32& {
  if(scc.status.floatingPointMode) {
    return fpu.r[index].f32;
  } else if(index & 1) {
    return fpu.r[index & ~1].f32h;
  } else {
    return fpu.r[index & ~1].f32;
  }
}

template<> auto CPU::fgr<s64>(u32 index) -> s64& {
  if(scc.status.floatingPointMode) {
    return fpu.r[index].s64;
  } else {
    return fpu.r[index & ~1].s64;
  }
}

template<> auto CPU::fgr<u64>(u32 index) -> u64& {
  return (u64&)fgr<s64>(index);
}

template<> auto CPU::fgr<f64>(u32 index) -> f64& {
  if(scc.status.floatingPointMode) {
    return fpu.r[index].f64;
  } else {
    return fpu.r[index & ~1].f64;
  }
}

auto CPU::getControlRegisterFPU(n5 index) -> u32 {
  n32 data;
  switch(index) {
  case  0:  //coprocessor revision identifier
    data.bit(0, 7) = fpu.coprocessor.revision;
    data.bit(8,15) = fpu.coprocessor.implementation;
    break;
  case 31:  //control / status register
    data.bit( 0) = fpu.csr.roundMode.bit(0);
    data.bit( 1) = fpu.csr.roundMode.bit(1);
    data.bit( 2) = fpu.csr.flag.inexact;
    data.bit( 3) = fpu.csr.flag.underflow;
    data.bit( 4) = fpu.csr.flag.overflow;
    data.bit( 5) = fpu.csr.flag.divisionByZero;
    data.bit( 6) = fpu.csr.flag.invalidOperation;
    data.bit( 7) = fpu.csr.enable.inexact;
    data.bit( 8) = fpu.csr.enable.underflow;
    data.bit( 9) = fpu.csr.enable.overflow;
    data.bit(10) = fpu.csr.enable.divisionByZero;
    data.bit(11) = fpu.csr.enable.invalidOperation;
    data.bit(12) = fpu.csr.cause.inexact;
    data.bit(13) = fpu.csr.cause.underflow;
    data.bit(14) = fpu.csr.cause.overflow;
    data.bit(15) = fpu.csr.cause.divisionByZero;
    data.bit(16) = fpu.csr.cause.invalidOperation;
    data.bit(17) = fpu.csr.cause.unimplementedOperation;
    data.bit(23) = fpu.csr.compare;
    data.bit(24) = fpu.csr.flushed;
    break;
  }
  return data;
}

auto CPU::setControlRegisterFPU(n5 index, n32 data) -> void {
  //read-only variables are defined but commented out for documentation purposes
  switch(index) {
  case  0:  //coprocessor revision identifier
  //fpu.coprocessor.revision       = data.bit(0, 7);
  //fpu.coprocessor.implementation = data.bit(8,15);
    break;
  case 31: {//control / status register
    u32 roundModePrevious = fpu.csr.roundMode;
    fpu.csr.roundMode.bit(0)             = data.bit( 0);
    fpu.csr.roundMode.bit(1)             = data.bit( 1);
    fpu.csr.flag.inexact                 = data.bit( 2);
    fpu.csr.flag.underflow               = data.bit( 3);
    fpu.csr.flag.overflow                = data.bit( 4);
    fpu.csr.flag.divisionByZero          = data.bit( 5);
    fpu.csr.flag.invalidOperation        = data.bit( 6);
    fpu.csr.enable.inexact               = data.bit( 7);
    fpu.csr.enable.underflow             = data.bit( 8);
    fpu.csr.enable.overflow              = data.bit( 9);
    fpu.csr.enable.divisionByZero        = data.bit(10);
    fpu.csr.enable.invalidOperation      = data.bit(11);
    fpu.csr.cause.inexact                = data.bit(12);
    fpu.csr.cause.underflow              = data.bit(13);
    fpu.csr.cause.overflow               = data.bit(14);
    fpu.csr.cause.divisionByZero         = data.bit(15);
    fpu.csr.cause.invalidOperation       = data.bit(16);
    fpu.csr.cause.unimplementedOperation = data.bit(17);
    fpu.csr.compare                      = data.bit(23);
    fpu.csr.flushed                      = data.bit(24);

    if(fpu.csr.roundMode != roundModePrevious) {
      switch(fpu.csr.roundMode) {
      case 0: fesetround(FE_TONEAREST);  break;
      case 1: fesetround(FE_TOWARDZERO); break;
      case 2: fesetround(FE_UPWARD);     break;
      case 3: fesetround(FE_DOWNWARD);   break;
      }
    }
  } break;
  }
}

#define CF fpu.csr.compare
#define FD(type) fgr<type>(fd)
#define FS(type) fgr<type>(fs)
#define FT(type) fgr<type>(ft)

auto CPU::instructionBC1(bool value, bool likely, s16 imm) -> void {
  if(CF == value) branch.take(ipu.pc + 4 + (imm << 2));
  else if(likely) branch.discard();
}

auto CPU::instructionCFC1(r64& rt, u8 rd) -> void {
  rt.u64 = s32(getControlRegisterFPU(rd));
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

auto CPU::instructionFCEIL_L_S(u8 fd, u8 fs) -> void { FD(s64) = ceil(FS(f32)); }
auto CPU::instructionFCEIL_L_D(u8 fd, u8 fs) -> void { FD(s64) = ceil(FS(f64)); }

auto CPU::instructionFCEIL_W_S(u8 fd, u8 fs) -> void { FD(s32) = ceil(FS(f32)); }
auto CPU::instructionFCEIL_W_D(u8 fd, u8 fs) -> void { FD(s32) = ceil(FS(f64)); }

#define   ORDERED(type, value) if(isnan(FS(type)) || isnan(FT(type))) { CF = value; return exception.floatingPoint(); }
#define UNORDERED(type, value) if(isnan(FS(type)) || isnan(FT(type))) { CF = value; return; }

auto CPU::instructionFC_EQ_S(u8 fs, u8 ft) -> void { UNORDERED(f32, 0); CF = FS(f32) == FT(f32); }
auto CPU::instructionFC_EQ_D(u8 fs, u8 ft) -> void { UNORDERED(f64, 0); CF = FS(f64) == FT(f64); }

auto CPU::instructionFC_F_S(u8 fs, u8 ft) -> void { UNORDERED(f32, 0); CF = 0; }
auto CPU::instructionFC_F_D(u8 fs, u8 ft) -> void { UNORDERED(f64, 0); CF = 0; }

auto CPU::instructionFC_LE_S(u8 fs, u8 ft) -> void { ORDERED(f32, 0); CF = FS(f32) <= FT(f32); }
auto CPU::instructionFC_LE_D(u8 fs, u8 ft) -> void { ORDERED(f64, 0); CF = FS(f64) <= FT(f64); }

auto CPU::instructionFC_LT_S(u8 fs, u8 ft) -> void { ORDERED(f32, 0); CF = FS(f32) < FT(f32); }
auto CPU::instructionFC_LT_D(u8 fs, u8 ft) -> void { ORDERED(f64, 0); CF = FS(f64) < FT(f64); }

auto CPU::instructionFC_NGE_S(u8 fs, u8 ft) -> void { ORDERED(f32, 1); CF = FS(f32) < FT(f32); }
auto CPU::instructionFC_NGE_D(u8 fs, u8 ft) -> void { ORDERED(f64, 1); CF = FS(f64) < FT(f64); }

auto CPU::instructionFC_NGL_S(u8 fs, u8 ft) -> void { ORDERED(f32, 1); CF = FS(f32) == FT(f32); }
auto CPU::instructionFC_NGL_D(u8 fs, u8 ft) -> void { ORDERED(f64, 1); CF = FS(f64) == FT(f64); }

auto CPU::instructionFC_NGLE_S(u8 fs, u8 ft) -> void { ORDERED(f32, 1); CF = 0; }
auto CPU::instructionFC_NGLE_D(u8 fs, u8 ft) -> void { ORDERED(f64, 1); CF = 0; }

auto CPU::instructionFC_NGT_S(u8 fs, u8 ft) -> void { ORDERED(f32, 1); CF = FS(f32) <= FT(f32); }
auto CPU::instructionFC_NGT_D(u8 fs, u8 ft) -> void { ORDERED(f64, 1); CF = FS(f64) <= FT(f64); }

auto CPU::instructionFC_OLE_S(u8 fs, u8 ft) -> void { UNORDERED(f32, 0); CF = FS(f32) <= FT(f32); }
auto CPU::instructionFC_OLE_D(u8 fs, u8 ft) -> void { UNORDERED(f64, 0); CF = FS(f64) <= FT(f64); }

auto CPU::instructionFC_OLT_S(u8 fs, u8 ft) -> void { UNORDERED(f32, 0); CF = FS(f32) < FT(f32); }
auto CPU::instructionFC_OLT_D(u8 fs, u8 ft) -> void { UNORDERED(f64, 0); CF = FS(f64) < FT(f64); }

auto CPU::instructionFC_SEQ_S(u8 fs, u8 ft) -> void { ORDERED(f32, 0); CF = FS(f32) == FT(f32); }
auto CPU::instructionFC_SEQ_D(u8 fs, u8 ft) -> void { ORDERED(f64, 0); CF = FS(f64) == FT(f64); }

auto CPU::instructionFC_SF_S(u8 fs, u8 ft) -> void { ORDERED(f32, 0); CF = 0; }
auto CPU::instructionFC_SF_D(u8 fs, u8 ft) -> void { ORDERED(f64, 0); CF = 0; }

auto CPU::instructionFC_UEQ_S(u8 fs, u8 ft) -> void { UNORDERED(f32, 1); CF = FS(f32) == FT(f32); }
auto CPU::instructionFC_UEQ_D(u8 fs, u8 ft) -> void { UNORDERED(f64, 1); CF = FS(f64) == FT(f64); }

auto CPU::instructionFC_ULE_S(u8 fs, u8 ft) -> void { UNORDERED(f32, 1); CF = FS(f32) <= FT(f32); }
auto CPU::instructionFC_ULE_D(u8 fs, u8 ft) -> void { UNORDERED(f64, 1); CF = FS(f64) <= FT(f64); }

auto CPU::instructionFC_ULT_S(u8 fs, u8 ft) -> void { UNORDERED(f32, 1); CF = FS(f32) < FT(f32); }
auto CPU::instructionFC_ULT_D(u8 fs, u8 ft) -> void { UNORDERED(f64, 1); CF = FS(f64) < FT(f64); }

auto CPU::instructionFC_UN_S(u8 fs, u8 ft) -> void { UNORDERED(f32, 1); CF = 0; }
auto CPU::instructionFC_UN_D(u8 fs, u8 ft) -> void { UNORDERED(f64, 1); CF = 0; }

#undef   ORDERED
#undef UNORDERED

auto CPU::instructionFCVT_S_D(u8 fd, u8 fs) -> void { FD(f32) = FS(f64); }
auto CPU::instructionFCVT_S_W(u8 fd, u8 fs) -> void { FD(f32) = FS(s32); }
auto CPU::instructionFCVT_S_L(u8 fd, u8 fs) -> void { FD(f32) = FS(s64); }

auto CPU::instructionFCVT_D_S(u8 fd, u8 fs) -> void { FD(f64) = FS(f32); }
auto CPU::instructionFCVT_D_W(u8 fd, u8 fs) -> void { FD(f64) = FS(s32); }
auto CPU::instructionFCVT_D_L(u8 fd, u8 fs) -> void { FD(f64) = FS(s64); }

auto CPU::instructionFCVT_L_S(u8 fd, u8 fs) -> void { FD(s64) = FS(f32); }
auto CPU::instructionFCVT_L_D(u8 fd, u8 fs) -> void { FD(s64) = FS(f64); }

auto CPU::instructionFCVT_W_S(u8 fd, u8 fs) -> void { FD(s32) = FS(f32); }
auto CPU::instructionFCVT_W_D(u8 fd, u8 fs) -> void { FD(s32) = FS(f64); }

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

auto CPU::instructionFFLOOR_L_S(u8 fd, u8 fs) -> void { FD(s64) = floor(FS(f32)); }
auto CPU::instructionFFLOOR_L_D(u8 fd, u8 fs) -> void { FD(s64) = floor(FS(f64)); }

auto CPU::instructionFFLOOR_W_S(u8 fd, u8 fs) -> void { FD(s32) = floor(FS(f32)); }
auto CPU::instructionFFLOOR_W_D(u8 fd, u8 fs) -> void { FD(s32) = floor(FS(f64)); }

auto CPU::instructionFMOV_S(u8 fd, u8 fs) -> void { FD(f32) = FS(f32); }
auto CPU::instructionFMOV_D(u8 fd, u8 fs) -> void { FD(f64) = FS(f64); }

auto CPU::instructionFMUL_S(u8 fd, u8 fs, u8 ft) -> void { FD(f32) = FS(f32) * FT(f32); }
auto CPU::instructionFMUL_D(u8 fd, u8 fs, u8 ft) -> void { FD(f64) = FS(f64) * FT(f64); }

auto CPU::instructionFNEG_S(u8 fd, u8 fs) -> void { FD(f32) = -FS(f32); }
auto CPU::instructionFNEG_D(u8 fd, u8 fs) -> void { FD(f64) = -FS(f64); }

auto CPU::instructionFROUND_L_S(u8 fd, u8 fs) -> void { FD(s64) = nearbyint(FS(f32)); }
auto CPU::instructionFROUND_L_D(u8 fd, u8 fs) -> void { FD(s64) = nearbyint(FS(f64)); }

auto CPU::instructionFROUND_W_S(u8 fd, u8 fs) -> void { FD(s32) = nearbyint(FS(f32)); }
auto CPU::instructionFROUND_W_D(u8 fd, u8 fs) -> void { FD(s32) = nearbyint(FS(f64)); }

auto CPU::instructionFSQRT_S(u8 fd, u8 fs) -> void { FD(f32) = sqrt(FS(f32)); }
auto CPU::instructionFSQRT_D(u8 fd, u8 fs) -> void { FD(f64) = sqrt(FS(f64)); }

auto CPU::instructionFSUB_S(u8 fd, u8 fs, u8 ft) -> void { FD(f32) = FS(f32) - FT(f32); }
auto CPU::instructionFSUB_D(u8 fd, u8 fs, u8 ft) -> void { FD(f64) = FS(f64) - FT(f64); }

auto CPU::instructionFTRUNC_L_S(u8 fd, u8 fs) -> void { FD(s64) = FS(f32) < 0 ? ceil(FS(f32)) : floor(FS(f32)); }
auto CPU::instructionFTRUNC_L_D(u8 fd, u8 fs) -> void { FD(s64) = FS(f64) < 0 ? ceil(FS(f64)) : floor(FS(f64)); }

auto CPU::instructionFTRUNC_W_S(u8 fd, u8 fs) -> void { FD(s32) = FS(f32) < 0 ? ceil(FS(f32)) : floor(FS(f32)); }
auto CPU::instructionFTRUNC_W_D(u8 fd, u8 fs) -> void { FD(s32) = FS(f64) < 0 ? ceil(FS(f64)) : floor(FS(f64)); }

auto CPU::instructionLDC1(u8 ft, cr64& rs, s16 imm) -> void {
  if(!scc.status.enable.coprocessor1) return exception.coprocessor1();
  if(auto data = readDual(rs.u32 + imm)) FT(u64) = *data;
}

auto CPU::instructionLWC1(u8 ft, cr64& rs, s16 imm) -> void {
  if(!scc.status.enable.coprocessor1) return exception.coprocessor1();
  if(auto data = readWord(rs.u32 + imm)) FT(u32) = *data;
}

auto CPU::instructionMFC1(r64& rt, u8 fs) -> void {
  rt.u64 = FS(s32);
}

auto CPU::instructionMTC1(cr64& rt, u8 fs) -> void {
  FS(s32) = rt.u32;
}

auto CPU::instructionSDC1(u8 ft, cr64& rs, s16 imm) -> void {
  if(!scc.status.enable.coprocessor1) return exception.coprocessor1();
  writeDual(rs.u32 + imm, FT(u64));
}

auto CPU::instructionSWC1(u8 ft, cr64& rs, s16 imm) -> void {
  if(!scc.status.enable.coprocessor1) return exception.coprocessor1();
  writeWord(rs.u32 + imm, FT(u32));
}

#undef CF
#undef FD
#undef FS
#undef FT
