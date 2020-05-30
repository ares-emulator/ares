#if !defined(RSP_REGISTERS_HPP)
  #define RSP_REGISTERS_HPP

  //CPU
  #define GPR core.r
  #define PC  core.pc
  #define RA GPR[Core::Register::RA]

  //VU
  #define VPR vu.r

  #define ACCH vu.acch
  #define ACCM vu.accm
  #define ACCL vu.accl
  #define VCOH vu.vcoh
  #define VCOL vu.vcol
  #define VCCH vu.vcch
  #define VCCL vu.vccl
  #define VCE  vu.vce

  #define DIVIN  vu.divin
  #define DIVOUT vu.divout
  #define DIVDP  vu.divdp
#else
  #undef RSP_REGISTERS_HPP

  //CPU
  #undef GPR
  #undef PC
  #undef RA

  //VU
  #undef VPR

  #undef ACCH
  #undef ACCM
  #undef ACCL
  #undef VCOH
  #undef VCOL
  #undef VCCH
  #undef VCCL
  #undef VCE

  #undef DIVIN
  #undef DIVOUT
  #undef DIVDP
#endif
