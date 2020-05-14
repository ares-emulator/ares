#if !defined(CPU_REGISTERS_HPP)
  #define CPU_REGISTERS_HPP

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
#else
  #undef CPU_REGISTERS_HPP

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
#endif
