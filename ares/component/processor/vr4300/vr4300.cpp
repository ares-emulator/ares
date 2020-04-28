#include <ares/ares.hpp>
#include "vr4300.hpp"

namespace ares {

#define SHIFT (opcode >>  6 & 31)
#define RDn   (opcode >> 11 & 31)
#define RTn   (opcode >> 16 & 31)
#define RSn   (opcode >> 21 & 31)

#define GPR context.gpr
#define FPR context.fpr
#define PC  context.pc

#define LO  context.lo
#define HI  context.hi

#define LO32u uint32_t(LO)
#define HI32u uint32_t(HI)

#define LO64u uint64_t(LO)
#define HI64u uint64_t(HI)

#define RD GPR[RDn]
#define RT GPR[RTn]
#define RS GPR[RSn]

#define RDw(value) if(RDn) RD = value

#define RD32i int32_t(RD)
#define RT32i int32_t(RT)
#define RS32i int32_t(RS)

#define RD64i int64_t(RD)
#define RT64i int64_t(RT)
#define RS64i int64_t(RS)

#define RD128i int128_t(RD)
#define RT128i int128_t(RT)
#define RS128i int128_t(RS)

#define RD32u uint32_t(RD)
#define RT32u uint32_t(RT)
#define RS32u uint32_t(RS)

#define RD64u uint64_t(RD)
#define RT64u uint64_t(RT)
#define RS64u uint64_t(RS)

#define RD128u uint128_t(RD)
#define RT128u uint128_t(RT)
#define RS128u uint128_t(RS)

#include "instruction.cpp"
#include "instructions-special.cpp"
#include "serialization.cpp"
#include "disassembler.cpp"

auto VR4300::power() -> void {
  context = {};
  PC = 0xbfc00000;
}

}
