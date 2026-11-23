#include <ares/ares.hpp>
#include "mbm29f016a.hpp"

namespace ares {

#include "serialization.cpp"

auto MBM29F016A::read(u32 address) -> u8 {
  address &= 0x1fffff;
  if(state == 3) {
    switch(address & 3) {
    case 0: return 0x04;
    case 1: return 0xad;
    }
    return 0;
  }
  return data[address];
}

auto MBM29F016A::write(u32 address, u8 value) -> void {
  address &= 0x1fffff;
  u32 low = address & 0xfff;
  bool first = low == 0x555 || low == 0xaaa;
  bool second = low == 0x2aa || low == 0x555;
  switch(state) {
  case 0:
  case 3:
    if(value == 0xf0) state = 0;
    else if(value == 0xaa && first) state = 1;
    return;
  case 1:
    state = value == 0x55 && second ? 2 : 0;
    return;
  case 2:
    state = 0;
    if(!first) return;
    if(value == 0x90) state = 3;
    if(value == 0xa0) state = 4;
    if(value == 0x80) state = 5;
    return;
  case 4:
    data[address] &= value;
    state = 0;
    return;
  case 5:
    state = value == 0xaa ? 6 : 0;
    return;
  case 6:
    state = value == 0x55 ? 7 : 0;
    return;
  case 7:
    if(value == 0x10) for(auto& byte : data) byte = 0xff;
    if(value == 0x30) for(u32 index : range(0x10000)) data[(address & ~0xffff) + index] = 0xff;
    state = 0;
    return;
  }
  state = 0;
}

}
