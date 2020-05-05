#if !defined(RSP_REGISTERS_HPP)
  #define RSP_REGISTERS_HPP

  //instruction decoding
  #define OP     pipeline.instruction
  #define SA     (OP >>  6 & 31)
  #define RDn    (OP >> 11 & 31)
  #define RTn    (OP >> 16 & 31)
  #define RSn    (OP >> 21 & 31)
  #define IMMi16 i16(OP)
  #define IMMu16 u16(OP)
  #define IMMu26 (OP & 0x03ff'ffff)

  //core
  #define GPR core.r
  #define LO  core.lo
  #define HI  core.hi
  #define PC  core.pc
  #define IP  core.ip

  #define RD GPR[RDn]
  #define RT GPR[RTn]
  #define RS GPR[RSn]
  #define RA GPR[Core::Register::RA]

  //COP0
  #define LLBIT  cop0.llbit
  #define SPR    cop0.r
  #define LL     SPR[COP0::Register::LLAddr].u64
  #define CAUSE  SPR[COP0::Register::Cause].u64
  #define STATUS SPR[COP0::Register::Status].u64

  #define STATUS_COP0 (STATUS & 1 << 28)
  #define STATUS_COP1 (STATUS & 1 << 29)
  #define STATUS_COP2 (STATUS & 1 << 30)
  #define STATUS_COP3 (STATUS & 1 << 31)

  //COP2
  #define VDn (OP >>  6 & 31)
  #define VSn (OP >> 11 & 31)
  #define VTn (OP >> 16 & 31)
  #define DEn (OP >> 11 & 31)
  #define En  (OP >> 21 & 15)

  #define VPR cop2.r
  #define VD  VPR[VDn]
  #define VS  VPR[VSn]
  #define VT  VPR[VTn]

  #define ACCH cop2.acch
  #define ACCM cop2.accm
  #define ACCL cop2.accl
  #define VCOH cop2.vcoh
  #define VCOL cop2.vcol
  #define VCCH cop2.vcch
  #define VCCL cop2.vccl
  #define VCE  cop2.vce
#else
  #undef RSP_REGISTERS_HPP

  //instruction decoding
  #undef OP
  #undef SA
  #undef RDn
  #undef RTn
  #undef RSn
  #undef IMMi16
  #undef IMMu16
  #undef IMMu26

  //core
  #undef GPR
  #undef LO
  #undef HI
  #undef PC
  #undef IP

  #undef RD
  #undef RT
  #undef RS
  #undef RA

  //COP0
  #undef LLBIT
  #undef SPR
  #undef LL
  #undef CAUSE
  #undef STATUS

  #undef STATUS_COP0
  #undef STATUS_COP1
  #undef STATUS_COP2
  #undef STATUS_COP3

  //COP2
  #undef VDn
  #undef VSn
  #undef VTn
  #undef DEn
  #undef En

  #undef VPR
  #undef VD
  #undef VS
  #undef VT

  #undef ACCH
  #undef ACCM
  #undef ACCL
  #undef VCOH
  #undef VCOL
  #undef VCCH
  #undef VCCL
  #undef VCE
#endif
