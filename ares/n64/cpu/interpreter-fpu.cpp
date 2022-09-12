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
      case 0: fenv.setRound(float_env::toNearest);  break;
      case 1: fenv.setRound(float_env::towardZero); break;
      case 2: fenv.setRound(float_env::upward);     break;
      case 3: fenv.setRound(float_env::downward);   break;
      }
    }
  } break;
  }
}

auto CPU::fpeDivisionByZero() -> bool {
  fpu.csr.cause.divisionByZero = 1;
  if(fpu.csr.enable.divisionByZero) return true;
  fpu.csr.flag.divisionByZero = 1;
  return false;
}

auto CPU::fpeInexact() -> bool {
  fpu.csr.cause.inexact = 1;
  if(fpu.csr.enable.inexact) return true;
  fpu.csr.flag.inexact = 1;
  return false;
}

auto CPU::fpeUnderflow() -> bool {
  fpu.csr.cause.underflow = 1;
  if(fpu.csr.enable.underflow) return true;
  fpu.csr.flag.underflow = 1;
  return false;
}

auto CPU::fpeOverflow() -> bool {
  fpu.csr.cause.overflow = 1;
  if(fpu.csr.enable.overflow) return true;
  fpu.csr.flag.overflow = 1;
  return false;
}

auto CPU::fpeInvalidOperation() -> bool {
  fpu.csr.cause.invalidOperation = 1;
  if(fpu.csr.enable.invalidOperation) return true;
  fpu.csr.flag.invalidOperation = 1;
  return false;
}

auto CPU::checkFPUExceptions() -> bool {
  u32 exc = fenv.testExcept(float_env::divByZero
                          | float_env::inexact
                          | float_env::underflow
                          | float_env::overflow
                          | float_env::invalid);
  if (!exc) return false;

  bool raise = false;
  if(exc & float_env::divByZero) raise |= fpeDivisionByZero();
  if(exc & float_env::inexact)   raise |= fpeInexact();
  if(exc & float_env::underflow) raise |= fpeUnderflow();
  if(exc & float_env::overflow)  raise |= fpeOverflow();
  if(exc & float_env::invalid)   raise |= fpeInvalidOperation();
  if(raise) exception.floatingPoint();
  return raise;
}

#define CHECK_FPE(type, operation) ({ \
  fenv.clearExcept(); \
  volatile type res = operation; \
  if (checkFPUExceptions()) return; \
  (res); \
})

#define CF fpu.csr.compare
#define FD(type) fgr<type>(fd)
#define FS(type) fgr<type>(fs)
#define FT(type) fgr<type>(ft)

auto CPU::BC1(bool value, bool likely, s16 imm) -> void {
  if(!scc.status.enable.coprocessor1) return exception.coprocessor1();
  if(CF == value) branch.take(ipu.pc + 4 + (imm << 2));
  else if(likely) branch.discard();
  else branch.notTaken();
}

auto CPU::CFC1(r64& rt, u8 rd) -> void {
  if(!scc.status.enable.coprocessor1) return exception.coprocessor1();
  rt.u64 = s32(getControlRegisterFPU(rd));
}

auto CPU::CTC1(cr64& rt, u8 rd) -> void {
  if(!scc.status.enable.coprocessor1) return exception.coprocessor1();
  setControlRegisterFPU(rd, rt.u32);
}

auto CPU::DMFC1(r64& rt, u8 fs) -> void {
  if(!scc.status.enable.coprocessor1) return exception.coprocessor1();
  rt.u64 = FS(u64);
}

auto CPU::DMTC1(cr64& rt, u8 fs) -> void {
  if(!scc.status.enable.coprocessor1) return exception.coprocessor1();
  FS(u64) = rt.u64;
}

auto CPU::FABS_S(u8 fd, u8 fs) -> void {
  if(!scc.status.enable.coprocessor1) return exception.coprocessor1();
  FD(f32) = fabs(FS(f32));
}

auto CPU::FABS_D(u8 fd, u8 fs) -> void {
  if(!scc.status.enable.coprocessor1) return exception.coprocessor1();
  FD(f64) = fabs(FS(f64));
}

auto CPU::FADD_S(u8 fd, u8 fs, u8 ft) -> void {
  if(!scc.status.enable.coprocessor1) return exception.coprocessor1();
  FD(f32) = CHECK_FPE(f32, FS(f32) + FT(f32));
}

auto CPU::FADD_D(u8 fd, u8 fs, u8 ft) -> void {
  if(!scc.status.enable.coprocessor1) return exception.coprocessor1();
  FD(f64) = CHECK_FPE(f64, FS(f64) + FT(f64));
}

auto CPU::FCEIL_L_S(u8 fd, u8 fs) -> void {
  if(!scc.status.enable.coprocessor1) return exception.coprocessor1();
  FD(s64) = ceil(FS(f32));
}

auto CPU::FCEIL_L_D(u8 fd, u8 fs) -> void {
  if(!scc.status.enable.coprocessor1) return exception.coprocessor1();
  FD(s64) = ceil(FS(f64));
}

auto CPU::FCEIL_W_S(u8 fd, u8 fs) -> void {
  if(!scc.status.enable.coprocessor1) return exception.coprocessor1();
  FD(s32) = ceil(FS(f32));
}

auto CPU::FCEIL_W_D(u8 fd, u8 fs) -> void {
  if(!scc.status.enable.coprocessor1) return exception.coprocessor1();
  FD(s32) = ceil(FS(f64));
}

#define  XORDERED(type, value, quiet) \
  if(isnan(FS(type)) || isnan(FT(type))) { \
    if constexpr(!quiet) { \
      if(fpeInvalidOperation()) return exception.floatingPoint(); \
    } \
    CF = value; \
    return; \
  }
#define   ORDERED(type, value) XORDERED(type, value, 0)
#define UNORDERED(type, value) XORDERED(type, value, 1)

auto CPU::FC_EQ_S(u8 fs, u8 ft) -> void {
  if(!scc.status.enable.coprocessor1) return exception.coprocessor1();
  UNORDERED(f32, 0); CF = FS(f32) == FT(f32);
}

auto CPU::FC_EQ_D(u8 fs, u8 ft) -> void {
  if(!scc.status.enable.coprocessor1) return exception.coprocessor1();
  UNORDERED(f64, 0); CF = FS(f64) == FT(f64);
}

auto CPU::FC_F_S(u8 fs, u8 ft) -> void {
  if(!scc.status.enable.coprocessor1) return exception.coprocessor1();
  UNORDERED(f32, 0); CF = 0;
}

auto CPU::FC_F_D(u8 fs, u8 ft) -> void {
  if(!scc.status.enable.coprocessor1) return exception.coprocessor1();
  UNORDERED(f64, 0); CF = 0;
}

auto CPU::FC_LE_S(u8 fs, u8 ft) -> void {
  if(!scc.status.enable.coprocessor1) return exception.coprocessor1();
  ORDERED(f32, 0); CF = FS(f32) <= FT(f32);
}

auto CPU::FC_LE_D(u8 fs, u8 ft) -> void {
  if(!scc.status.enable.coprocessor1) return exception.coprocessor1();
  ORDERED(f64, 0); CF = FS(f64) <= FT(f64);
}

auto CPU::FC_LT_S(u8 fs, u8 ft) -> void {
  if(!scc.status.enable.coprocessor1) return exception.coprocessor1();
  ORDERED(f32, 0); CF = FS(f32) < FT(f32);
}

auto CPU::FC_LT_D(u8 fs, u8 ft) -> void {
  if(!scc.status.enable.coprocessor1) return exception.coprocessor1();
  ORDERED(f64, 0); CF = FS(f64) < FT(f64);
}

auto CPU::FC_NGE_S(u8 fs, u8 ft) -> void {
  if(!scc.status.enable.coprocessor1) return exception.coprocessor1();
  ORDERED(f32, 1); CF = FS(f32) < FT(f32);
}

auto CPU::FC_NGE_D(u8 fs, u8 ft) -> void {
  if(!scc.status.enable.coprocessor1) return exception.coprocessor1();
  ORDERED(f64, 1); CF = FS(f64) < FT(f64);
}

auto CPU::FC_NGL_S(u8 fs, u8 ft) -> void {
  if(!scc.status.enable.coprocessor1) return exception.coprocessor1();
  ORDERED(f32, 1); CF = FS(f32) == FT(f32);
}

auto CPU::FC_NGL_D(u8 fs, u8 ft) -> void {
  if(!scc.status.enable.coprocessor1) return exception.coprocessor1();
  ORDERED(f64, 1); CF = FS(f64) == FT(f64);
}

auto CPU::FC_NGLE_S(u8 fs, u8 ft) -> void {
  if(!scc.status.enable.coprocessor1) return exception.coprocessor1();
  ORDERED(f32, 1); CF = 0;
}

auto CPU::FC_NGLE_D(u8 fs, u8 ft) -> void {
  if(!scc.status.enable.coprocessor1) return exception.coprocessor1();
  ORDERED(f64, 1); CF = 0;
}

auto CPU::FC_NGT_S(u8 fs, u8 ft) -> void {
  if(!scc.status.enable.coprocessor1) return exception.coprocessor1();
  ORDERED(f32, 1); CF = FS(f32) <= FT(f32);
}

auto CPU::FC_NGT_D(u8 fs, u8 ft) -> void {
  if(!scc.status.enable.coprocessor1) return exception.coprocessor1();
  ORDERED(f64, 1); CF = FS(f64) <= FT(f64);
}

auto CPU::FC_OLE_S(u8 fs, u8 ft) -> void {
  if(!scc.status.enable.coprocessor1) return exception.coprocessor1();
  UNORDERED(f32, 0); CF = FS(f32) <= FT(f32);
}

auto CPU::FC_OLE_D(u8 fs, u8 ft) -> void {
  if(!scc.status.enable.coprocessor1) return exception.coprocessor1();
  UNORDERED(f64, 0); CF = FS(f64) <= FT(f64);
}

auto CPU::FC_OLT_S(u8 fs, u8 ft) -> void {
  if(!scc.status.enable.coprocessor1) return exception.coprocessor1();
  UNORDERED(f32, 0); CF = FS(f32) < FT(f32);
}

auto CPU::FC_OLT_D(u8 fs, u8 ft) -> void {
  if(!scc.status.enable.coprocessor1) return exception.coprocessor1();
  UNORDERED(f64, 0); CF = FS(f64) < FT(f64);
}

auto CPU::FC_SEQ_S(u8 fs, u8 ft) -> void {
  if(!scc.status.enable.coprocessor1) return exception.coprocessor1();
  ORDERED(f32, 0); CF = FS(f32) == FT(f32);
}

auto CPU::FC_SEQ_D(u8 fs, u8 ft) -> void {
  if(!scc.status.enable.coprocessor1) return exception.coprocessor1();
  ORDERED(f64, 0); CF = FS(f64) == FT(f64);
}

auto CPU::FC_SF_S(u8 fs, u8 ft) -> void {
  if(!scc.status.enable.coprocessor1) return exception.coprocessor1();
  ORDERED(f32, 0); CF = 0;
}

auto CPU::FC_SF_D(u8 fs, u8 ft) -> void {
  if(!scc.status.enable.coprocessor1) return exception.coprocessor1();
  ORDERED(f64, 0); CF = 0;
}

auto CPU::FC_UEQ_S(u8 fs, u8 ft) -> void {
  if(!scc.status.enable.coprocessor1) return exception.coprocessor1();
  UNORDERED(f32, 1); CF = FS(f32) == FT(f32);
}

auto CPU::FC_UEQ_D(u8 fs, u8 ft) -> void {
  if(!scc.status.enable.coprocessor1) return exception.coprocessor1();
  UNORDERED(f64, 1); CF = FS(f64) == FT(f64);
}

auto CPU::FC_ULE_S(u8 fs, u8 ft) -> void {
  if(!scc.status.enable.coprocessor1) return exception.coprocessor1();
  UNORDERED(f32, 1); CF = FS(f32) <= FT(f32);
}

auto CPU::FC_ULE_D(u8 fs, u8 ft) -> void {
  if(!scc.status.enable.coprocessor1) return exception.coprocessor1();
  UNORDERED(f64, 1); CF = FS(f64) <= FT(f64);
}

auto CPU::FC_ULT_S(u8 fs, u8 ft) -> void {
  if(!scc.status.enable.coprocessor1) return exception.coprocessor1();
  UNORDERED(f32, 1); CF = FS(f32) < FT(f32);
}

auto CPU::FC_ULT_D(u8 fs, u8 ft) -> void {
  if(!scc.status.enable.coprocessor1) return exception.coprocessor1();
  UNORDERED(f64, 1); CF = FS(f64) < FT(f64);
}

auto CPU::FC_UN_S(u8 fs, u8 ft) -> void {
  if(!scc.status.enable.coprocessor1) return exception.coprocessor1();
  UNORDERED(f32, 1); CF = 0;
}

auto CPU::FC_UN_D(u8 fs, u8 ft) -> void {
  if(!scc.status.enable.coprocessor1) return exception.coprocessor1();
  UNORDERED(f64, 1); CF = 0;
}

#undef   ORDERED
#undef UNORDERED

auto CPU::FCVT_S_D(u8 fd, u8 fs) -> void {
  if(!scc.status.enable.coprocessor1) return exception.coprocessor1();
  FD(f32) = FS(f64);
}

auto CPU::FCVT_S_W(u8 fd, u8 fs) -> void {
  if(!scc.status.enable.coprocessor1) return exception.coprocessor1();
  FD(f32) = FS(s32);
}

auto CPU::FCVT_S_L(u8 fd, u8 fs) -> void {
  if(!scc.status.enable.coprocessor1) return exception.coprocessor1();
  FD(f32) = FS(s64);
}

auto CPU::FCVT_D_S(u8 fd, u8 fs) -> void {
  if(!scc.status.enable.coprocessor1) return exception.coprocessor1();
  FD(f64) = FS(f32);
}

auto CPU::FCVT_D_W(u8 fd, u8 fs) -> void {
  if(!scc.status.enable.coprocessor1) return exception.coprocessor1();
  FD(f64) = FS(s32);
}

auto CPU::FCVT_D_L(u8 fd, u8 fs) -> void {
  if(!scc.status.enable.coprocessor1) return exception.coprocessor1();
  FD(f64) = FS(s64);
}

auto CPU::FCVT_L_S(u8 fd, u8 fs) -> void {
  if(!scc.status.enable.coprocessor1) return exception.coprocessor1();
  FD(s64) = FS(f32);
}

auto CPU::FCVT_L_D(u8 fd, u8 fs) -> void {
  if(!scc.status.enable.coprocessor1) return exception.coprocessor1();
  FD(s64) = FS(f64);
}

auto CPU::FCVT_W_S(u8 fd, u8 fs) -> void {
  if(!scc.status.enable.coprocessor1) return exception.coprocessor1();
  FD(s32) = FS(f32);
}

auto CPU::FCVT_W_D(u8 fd, u8 fs) -> void {
  if(!scc.status.enable.coprocessor1) return exception.coprocessor1();
  FD(s32) = FS(f64);
}

auto CPU::FDIV_S(u8 fd, u8 fs, u8 ft) -> void {
  if(!scc.status.enable.coprocessor1) return exception.coprocessor1();
  FD(f32) = CHECK_FPE(f32, FS(f32) / FT(f32));
}

auto CPU::FDIV_D(u8 fd, u8 fs, u8 ft) -> void {
  if(!scc.status.enable.coprocessor1) return exception.coprocessor1();
  FD(f64) = CHECK_FPE(f64, FS(f64) / FT(f64));
}

auto CPU::FFLOOR_L_S(u8 fd, u8 fs) -> void {
  if(!scc.status.enable.coprocessor1) return exception.coprocessor1();
  FD(s64) = floor(FS(f32));
}

auto CPU::FFLOOR_L_D(u8 fd, u8 fs) -> void {
  if(!scc.status.enable.coprocessor1) return exception.coprocessor1();
  FD(s64) = floor(FS(f64));
}

auto CPU::FFLOOR_W_S(u8 fd, u8 fs) -> void {
  if(!scc.status.enable.coprocessor1) return exception.coprocessor1();
  FD(s32) = floor(FS(f32));
}

auto CPU::FFLOOR_W_D(u8 fd, u8 fs) -> void {
  if(!scc.status.enable.coprocessor1) return exception.coprocessor1();
  FD(s32) = floor(FS(f64));
}

auto CPU::FMOV_S(u8 fd, u8 fs) -> void {
  if(!scc.status.enable.coprocessor1) return exception.coprocessor1();
  FD(f32) = FS(f32);
}

auto CPU::FMOV_D(u8 fd, u8 fs) -> void {
  if(!scc.status.enable.coprocessor1) return exception.coprocessor1();
  FD(f64) = FS(f64);
}

auto CPU::FMUL_S(u8 fd, u8 fs, u8 ft) -> void {
  if(!scc.status.enable.coprocessor1) return exception.coprocessor1();
  FD(f32) = CHECK_FPE(f32, FS(f32) * FT(f32));
}

auto CPU::FMUL_D(u8 fd, u8 fs, u8 ft) -> void {
  if(!scc.status.enable.coprocessor1) return exception.coprocessor1();
  FD(f64) = CHECK_FPE(f64, FS(f64) * FT(f64));
}

auto CPU::FNEG_S(u8 fd, u8 fs) -> void {
  if(!scc.status.enable.coprocessor1) return exception.coprocessor1();
  FD(f32) = CHECK_FPE(f32, -FS(f32));
}

auto CPU::FNEG_D(u8 fd, u8 fs) -> void {
  if(!scc.status.enable.coprocessor1) return exception.coprocessor1();
  FD(f64) = CHECK_FPE(f64, -FS(f64));
}

auto CPU::FROUND_L_S(u8 fd, u8 fs) -> void {
  if(!scc.status.enable.coprocessor1) return exception.coprocessor1();
  FD(s64) = nearbyint(FS(f32));
}

auto CPU::FROUND_L_D(u8 fd, u8 fs) -> void {
  if(!scc.status.enable.coprocessor1) return exception.coprocessor1();
  FD(s64) = nearbyint(FS(f64));
}

auto CPU::FROUND_W_S(u8 fd, u8 fs) -> void {
  if(!scc.status.enable.coprocessor1) return exception.coprocessor1();
  FD(s32) = nearbyint(FS(f32));
}

auto CPU::FROUND_W_D(u8 fd, u8 fs) -> void {
  if(!scc.status.enable.coprocessor1) return exception.coprocessor1();
  FD(s32) = nearbyint(FS(f64));
}

auto CPU::FSQRT_S(u8 fd, u8 fs) -> void {
  if(!scc.status.enable.coprocessor1) return exception.coprocessor1();
  FD(f32) = sqrt(FS(f32));
}

auto CPU::FSQRT_D(u8 fd, u8 fs) -> void {
  if(!scc.status.enable.coprocessor1) return exception.coprocessor1();
  FD(f64) = sqrt(FS(f64));
}

auto CPU::FSUB_S(u8 fd, u8 fs, u8 ft) -> void {
  if(!scc.status.enable.coprocessor1) return exception.coprocessor1();
  FD(f32) = CHECK_FPE(f32, FS(f32) - FT(f32));
}

auto CPU::FSUB_D(u8 fd, u8 fs, u8 ft) -> void {
  if(!scc.status.enable.coprocessor1) return exception.coprocessor1();
  FD(f64) = CHECK_FPE(f64, FS(f64) - FT(f64));
}

auto CPU::FTRUNC_L_S(u8 fd, u8 fs) -> void {
  if(!scc.status.enable.coprocessor1) return exception.coprocessor1();
  FD(s64) = FS(f32) < 0 ? ceil(FS(f32)) : floor(FS(f32));
}

auto CPU::FTRUNC_L_D(u8 fd, u8 fs) -> void {
  if(!scc.status.enable.coprocessor1) return exception.coprocessor1();
  FD(s64) = FS(f64) < 0 ? ceil(FS(f64)) : floor(FS(f64));
}

auto CPU::FTRUNC_W_S(u8 fd, u8 fs) -> void {
  if(!scc.status.enable.coprocessor1) return exception.coprocessor1();
  FD(s32) = FS(f32) < 0 ? ceil(FS(f32)) : floor(FS(f32));
}

auto CPU::FTRUNC_W_D(u8 fd, u8 fs) -> void {
  if(!scc.status.enable.coprocessor1) return exception.coprocessor1();
  FD(s32) = FS(f64) < 0 ? ceil(FS(f64)) : floor(FS(f64));
}

auto CPU::LDC1(u8 ft, cr64& rs, s16 imm) -> void {
  if(!scc.status.enable.coprocessor1) return exception.coprocessor1();
  if(auto data = read<Dual>(rs.u64 + imm)) FT(u64) = *data;
}

auto CPU::LWC1(u8 ft, cr64& rs, s16 imm) -> void {
  if(!scc.status.enable.coprocessor1) return exception.coprocessor1();
  if(auto data = read<Word>(rs.u64 + imm)) FT(u32) = *data;
}

auto CPU::MFC1(r64& rt, u8 fs) -> void {
  if(!scc.status.enable.coprocessor1) return exception.coprocessor1();
  rt.u64 = FS(s32);
}

auto CPU::MTC1(cr64& rt, u8 fs) -> void {
  if(!scc.status.enable.coprocessor1) return exception.coprocessor1();
  FS(s32) = rt.u32;
}

auto CPU::SDC1(u8 ft, cr64& rs, s16 imm) -> void {
  if(!scc.status.enable.coprocessor1) return exception.coprocessor1();
  write<Dual>(rs.u64 + imm, FT(u64));
}

auto CPU::SWC1(u8 ft, cr64& rs, s16 imm) -> void {
  if(!scc.status.enable.coprocessor1) return exception.coprocessor1();
  write<Word>(rs.u64 + imm, FT(u32));
}

auto CPU::COP1INVALID() -> void {
  if(!scc.status.enable.coprocessor1) return exception.coprocessor1();
  exception.floatingPoint();
}

#undef CF
#undef FD
#undef FS
#undef FT
#undef CHECK_FPE
