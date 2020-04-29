#include <ares/ares.hpp>
#include "vr4300.hpp"

namespace ares {

#define SA  (opcode >>  6 & 31)
#define RDn (opcode >> 11 & 31)
#define RTn (opcode >> 16 & 31)
#define RSn (opcode >> 21 & 31)

#define IMM16i  int16_t(opcode)
#define IMM16u uint16_t(opcode)
#define IMM26u (opcode & 0x03ffffff)

#define GPR r.gpr
#define LR  r.gpr[31]
#define LO  r.lo
#define HI  r.hi
#define PC  r.pc
#define IP  r.ip

#define RD GPR[RDn]
#define RT GPR[RTn]
#define RS GPR[RSn]

#define  RT32i  int32_t(RT)
#define  RT64i  int64_t(RT)
#define RT128i int128_t(RT)

#define  RS32i  int32_t(RS)
#define  RS64i  int64_t(RS)
#define RS128i int128_t(RS)

#define  RT32u  uint32_t(RT)
#define  RT64u  uint64_t(RT)
#define RT128u uint128_t(RT)

#define  RS32u  uint32_t(RS)
#define  RS64u  uint64_t(RS)
#define RS128u uint128_t(RS)

#define wRD32i(value) if(RDn) RD =  int32_t(value)
#define wRD64u(value) if(RDn) RD = uint64_t(value)

#define wRT32i(value) if(RTn) RT =  int32_t(value)
#define wRT64u(value) if(RTn) RT = uint64_t(value)

#define wLR32i(value) LR =  int32_t(value)
#define wLR64u(value) LR = uint64_t(value)

#define wLO32i(value) LO =  int32_t(value)
#define wLO64u(value) LO = uint64_t(value)

#define wHI32i(value) HI =  int32_t(value)
#define wHI64u(value) HI = uint64_t(value)

#include "instruction.cpp"
#include "instructions-special.cpp"
#include "instructions-regimm.cpp"
#include "instructions.cpp"
#include "serialization.cpp"
#include "disassembler.cpp"

auto VR4300::power() -> void {
  for(uint n : range(32)) GPR[n] = 0;
  LO = 0;
  HI = 0;
  PC = 0xbfc00000;
}

}
