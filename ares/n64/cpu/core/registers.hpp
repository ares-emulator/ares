#if !defined(VR4300_REGISTERS_HPP)
  #define VR4300_REGISTERS_HPP

  //ionstruction decoding
  #define SA  (pipeline.instruction >>  6 & 31)
  #define RDn (pipeline.instruction >> 11 & 31)
  #define RTn (pipeline.instruction >> 16 & 31)
  #define RSn (pipeline.instruction >> 21 & 31)

  #define IMMi16 i16(pipeline.instruction)
  #define IMMu16 u16(pipeline.instruction)
  #define IMMu26 (pipeline.instruction & 0x03ffffff)

  //core
  #define GPR core.r
  #define RA  core.r[31]
  #define LO  core.lo
  #define HI  core.hi
  #define PC  core.pc
  #define IP  core.ip

  #define RD GPR[RDn]
  #define RT GPR[RTn]
  #define RS GPR[RSn]

  //COP0
  #define LLBIT  cop0.llbit
  #define SPR    cop0.r
  #define LL     SPR[COP0::Register::LLAddr].u64
  #define CAUSE  SPR[COP0::Register::Cause].u64
  #define STATUS SPR[COP0::Register::Status].u64

  #define STATUS_FR   (STATUS & 1 << 26)
  #define STATUS_COP0 (STATUS & 1 << 28)
  #define STATUS_COP1 (STATUS & 1 << 29)
  #define STATUS_COP2 (STATUS & 1 << 30)
  #define STATUS_COP3 (STATUS & 1 << 31)

  //COP1
  #define FDn (pipeline.instruction >>  6 & 31)
  #define FSn (pipeline.instruction >> 11 & 31)
  #define FTn (pipeline.instruction >> 16 & 31)
  #define FRn (pipeline.instruction >> 21 & 31)

  #define FPR cop1.r
  #define FD  FPR[FDn]
  #define FS  FPR[FSn]
  #define FT  FPR[FTn]
  #define FR  FPR[FRn]

  #define FPU_SINGLE  ((pipeline.instruction & 1 << 21) == 0)
  #define FPU_DOUBLE  ((pipeline.instruction & 1 << 21) != 0)
  #define FPU_INTEGER ((pipeline.instruction & 1 << 23) == 0)
  #define FPU_REAL    ((pipeline.instruction & 1 << 23) != 0)
#else
  #undef VR4300_REGISTERS_HPP

  //instruction decoding
  #undef SA
  #undef RDn
  #undef RTn
  #undef RSn

  #undef IMMi16
  #undef IMMu16
  #undef IMMu26

  //core
  #undef GPR
  #undef RA
  #undef LO
  #undef HI
  #undef PC
  #undef IP

  #undef RD
  #undef RT
  #undef RS

  //COP0
  #undef LLBIT
  #undef SPR
  #undef LL
  #undef CAUSE
  #undef STATUS

  #undef STATUS_FR
  #undef STATUS_COP0
  #undef STATUS_COP1
  #undef STATUS_COP2
  #undef STATUS_COP3

  //COP1
  #undef FDn
  #undef FSn
  #undef FTn
  #undef FRn

  #undef FPR
  #undef FD
  #undef FS
  #undef FT
  #undef FR

  #undef FPU_SINGLE
  #undef FPU_DOUBLE
  #undef FPU_INTEGER
  #undef FPU_REAL
#endif
