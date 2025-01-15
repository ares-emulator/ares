#include <ares/ares.hpp>
#include "m68000.hpp"

namespace ares {

enum : u32  { Byte, Word, Long };
enum : bool { Reverse = 1 };

#include "registers.cpp"
#include "memory.cpp"
#include "effective-address.cpp"
#include "traits.cpp"
#include "conditions.cpp"
#include "algorithms.cpp"
#include "instructions.cpp"
#include "disassembler.cpp"
#include "instruction.cpp"
#include "serialization.cpp"

auto M68000::power() -> void {
  for(auto& dr : r.d) dr = 0;
  for(auto& ar : r.a) ar = 0;
  r.sp = 0;
  r.pc = 0;

  r.c = 0;
  r.v = 0;
  r.z = 0;
  r.n = 0;
  r.x = 0;
  r.i = 7;
  r.s = 1;
  r.t = 0;

  r.irc = 0x4e71;  //nop
  r.ir  = 0x4e71;  //nop
  r.ird = 0x4e71;  //nop

  r.stop  = false;
  r.reset = false;
}

auto M68000::supervisor() -> bool {
  if(r.s) return true;

  exception(Exception::Unprivileged, Vector::Unprivileged);
  return false;
}

auto M68000::exception(u32 exception, u32 vector, u32 priority) -> void {
  r.stop  = false;

  // register setup (+6 cyc)
  idle(6);
  n32 pc = r.pc - 4;
  n32 sr = readSR();
  if(!r.s) swap(r.a[7], r.sp);
  r.s = 1;
  r.t = 0;

  // push pc low (+4 cyc)
  push<Word>(pc & 0x0000ffff);

  // external interrupt handling
  if(exception == Exception::Interrupt) {
    // IACK vector number acquisition (+4 cyc normal or +10 to +18 cyc autovectored)
    // & justify vector number (+4 cyc)
    idle(14+4); // assuming autovector cycles (approximated for Megadrive)
    r.i = priority;
  }

  // push pc high & sr (+8 cyc)
  push<Long>(sr << 16 | pc >> 16);

  // read vector address (+8 cyc)
  r.pc = read<Long>(vector << 2);
  // todo: if the vector address read causes an address error, this exception should be escalated

  // prefetch (+8 cyc)
  prefetch();
  if(exception == Exception::Interrupt) idle(2); // dead cycles (+2 cyc)
  prefetch();
}

auto M68000::interrupt(u32 vector, u32 priority) -> void {
  return exception(Exception::Interrupt, vector, priority);
}

}
