#if !defined(VR4300_REGISTERS_HPP)
  #define VR4300_REGISTERS_HPP

  //ionstruction decoding
  #define SA  (pipeline.instruction >>  6 & 31)
  #define RDn (pipeline.instruction >> 11 & 31)
  #define RTn (pipeline.instruction >> 16 & 31)
  #define RSn (pipeline.instruction >> 21 & 31)

  #define IMM16i i16(pipeline.instruction)
  #define IMM16u u16(pipeline.instruction)
  #define IMM26u (pipeline.instruction & 0x03ffffff)

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

  #define  RT32i  i32(RT)
  #define  RT64i  i64(RT)
  #define RT128i i128(RT)

  #define  RS32i  i32(RS)
  #define  RS64i  i64(RS)
  #define RS128i i128(RS)

  #define  RT32u  u32(RT)
  #define  RT64u  u64(RT)
  #define RT128u u128(RT)

  #define  RS32u  u32(RS)
  #define  RS64u  u64(RS)
  #define RS128u u128(RS)

  #define wRD32i(value) if(RDn) RD = i32(value)
  #define wRD64u(value) if(RDn) RD = u64(value)

  #define wRT32i(value) if(RTn) RT = i32(value)
  #define wRT64u(value) if(RTn) RT = u64(value)

  #define wRA32i(value) RA = i32(value)
  #define wRA64u(value) RA = u64(value)

  #define wLO32i(value) LO = i32(value)
  #define wLO64u(value) LO = u64(value)

  #define wHI32i(value) HI = i32(value)
  #define wHI64u(value) HI = u64(value)

  #define LLBIT core.llbit

  //COP0
  #define LL     cop0.r[COP0::Register::LLAddr]
  #define CAUSE  cop0.r[COP0::Register::Cause]
  #define STATUS cop0.r[COP0::Register::Status]
  #define STATUS_FR   (STATUS & 1 << 26)
  #define STATUS_COP0 (STATUS & 1 << 28)
  #define STATUS_COP1 (STATUS & 1 << 29)
  #define STATUS_COP2 (STATUS & 1 << 30)
  #define STATUS_COP3 (STATUS & 1 << 31)
#else
  #undef VR4300_REGISTERS_HPP

  //instruction decoding
  #undef SA
  #undef RDn
  #undef RTn
  #undef RSn

  #undef IMM16i
  #undef IMM16u
  #undef IMM26u

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

  #undef  RT32i
  #undef  RT64i
  #undef RT128i

  #undef  RS32i
  #undef  RS64i
  #undef RS128i

  #undef  RT32u
  #undef  RT64u
  #undef RT128u

  #undef  RS32u
  #undef  RS64u
  #undef RS128u

  #undef wRD32i
  #undef wRD64u

  #undef wRT32i
  #undef wRT64u

  #undef wLR32i
  #undef wLR64u

  #undef wLO32i
  #undef wLO64u

  #undef wHI32i
  #undef wHI64u

  #undef LL
  #undef LLBIT

  //COP0
  #undef CAUSE
  #undef STATUS
  #undef STATUS_FR
  #undef STATUS_COP0
  #undef STATUS_COP1
  #undef STATUS_COP2
  #undef STATUS_COP3
#endif
