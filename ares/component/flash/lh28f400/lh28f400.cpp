#include <ares/ares.hpp>
#include "lh28f400.hpp"

namespace ares {

#include "serialization.cpp"

auto LH28F400::read(u32 word) -> u32 {
  word &= 0x3ffff;
  if(state == 1) {
    if(word == 0) return 0x00b0;
    if(word == 1) return 0x00ed;
    return 0;
  }
  if(state == 2) return 0x0080;
  return data[word * 2] << 8 | data[word * 2 + 1];
}

auto LH28F400::write(u32 word, u32 value) -> void {
  word &= 0x3ffff;
  u32 command = value & 0xff;
  if(state == 3) {
    data[word * 2 + 0] &= value >> 8;
    data[word * 2 + 1] &= value;
    state = 2;
    return;
  }
  if(state == 4) {
    if(command == 0xd0) {
      u32 address = word * 2;
      u32 start = address & ~0xffff, end = start + 0x10000;
      for(u32 index = start; index < end; index++) data[index] = 0xff;
    }
    state = 2;
    return;
  }
  if(state == 5) {
    state = 2;
    return;
  }
  if(command == 0xff || command == 0xf0) state = 0;
  else if(command == 0x50) state = 2;
  else if(command == 0x60) state = 5;
  else if(command == 0x90) state = 1;
  else if(command == 0x70) state = 2;
  else if(command == 0x40 || command == 0x10) state = 3;
  else if(command == 0x20) state = 4;
}

}
