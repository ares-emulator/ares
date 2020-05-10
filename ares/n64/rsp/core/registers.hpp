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

  //CPU
  #define GPR core.r
  #define LO  core.lo
  #define HI  core.hi
  #define PC  core.pc
  #define IP  core.ip

  #define RD GPR[RDn]
  #define RT GPR[RTn]
  #define RS GPR[RSn]
  #define RA GPR[Core::Register::RA]

  //VU
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

  #define DIVIN  cop2.divin
  #define DIVOUT cop2.divout
  #define DIVDP  cop2.divdp
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

  //CPU
  #undef GPR
  #undef LO
  #undef HI
  #undef PC
  #undef IP

  #undef RD
  #undef RT
  #undef RS
  #undef RA

  //VU
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

  #undef DIVIN
  #undef DIVOUT
  #undef DIVDP
#endif
