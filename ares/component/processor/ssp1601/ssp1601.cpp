#include <ares/ares.hpp>
#include "ssp1601.hpp"

namespace ares {

#define AL   A.bit( 0,15)
#define AH   A.bit(16,31)

#define RPL  ST.bit( 0,2)
#define RB   ST.bit( 3,4)
#define USR0 ST.bit( 5)
#define USR1 ST.bit( 6)
#define IE   ST.bit( 7)
#define OP   ST.bit( 8)
#define MACS ST.bit( 9)
#define GPI0 ST.bit(10)
#define GPI1 ST.bit(11)
#define C    ST.bit(12)
#define Z    ST.bit(13)
#define V    ST.bit(14)
#define N    ST.bit(15)

#include "algorithms.cpp"
#include "registers.cpp"
#include "memory.cpp"
#include "instructions.cpp"
#include "serialization.cpp"
#include "disassembler.cpp"

auto SSP1601::power() -> void {
  for(auto& word : RAM) word = 0;
  for(auto& word : FRAME) word = 0;
  X = 0;
  Y = 0;
  A = 0;
  ST = 0;
  STACK = 0;
  PC = 0;
  P = 0;
  for(auto& byte : R) byte = 0;
  IE = 1;
  IRQ = 1;
}

}
