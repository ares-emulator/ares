#if !defined(CPU_REGISTERS_HPP)
  #define CPU_REGISTERS_HPP

  //CPU
  #define GPR core.r
  #define LO  core.lo
  #define HI  core.hi
  #define PC  core.pc

  #define RA GPR[Core::Register::RA]
#else
  #undef CPU_REGISTERS_HPP

  //CPU
  #undef GPR
  #undef LO
  #undef HI
  #undef PC
  #undef RA
#endif
