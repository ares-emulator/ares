#include <ares/ares.hpp>
#include "sst39sf0x0.hpp"

namespace ares {

auto SST39SF0x0::reset() -> void {
  mode = Mode::Data;
  unlockStep = UnlockStep::Unlock0;
}

auto SST39SF0x0::read(n32 address) -> n8 {
  if(mode == Mode::ID) {
    if(address == 0x00) return 0xBF; /* Manufacturer ID */
    if(address == 0x01) { /* Device ID */
      if(flash.size() <= 128 * 1024) return 0xB5;
      if(flash.size() <= 256 * 1024) return 0xB6;
      return 0xB7;
    }
    return 0xFF;
  }
  return flash.read(address);
}

auto SST39SF0x0::write(n32 address, n8 data) -> void {
  if(mode == Mode::Program) {
    flash.write(address, data);
    mode = Mode::Data;
  } else if (mode == Mode::EraseBlock) {
    address &= ~0xFFF;
    for(u32 i = 0; i < 4096; i++) {
      flash.write(address + i, 0xFF);
    }
    mode = Mode::Data;
  } else if(mode == Mode::ID && data == 0xF0) {
    unlockStep = UnlockStep::Unlock0;
    mode = Mode::Data;
  } else {
    if(unlockStep == UnlockStep::Unlock0) {
      if((address & 0x7FFF) == 0x5555 && (data & 0xFF) == 0xAA) unlockStep = UnlockStep::Unlock1;
    } else if(unlockStep == UnlockStep::Unlock1) {
      unlockStep = UnlockStep::Unlock0;
      if((address & 0x7FFF) == 0x2AAA && (data & 0xFF) == 0x55) unlockStep = UnlockStep::Unlock2;
    } else {
      unlockStep = UnlockStep::Unlock0;
      if((address & 0x7FFF) == 0x5555) {
        switch(data) {
          case 0xA0:
            mode = Mode::Program;
            break;
          case 0x80:
            mode = Mode::Erase;
            break;
          case 0x10:
            if(mode != Mode::Erase) return;
            for(u32 i = 0; i < flash.size(); i++) {
              flash.write(i, 0xFF);
            }
            break;
          case 0x30:
            if(mode != Mode::Erase) return;
            mode = Mode::EraseBlock;
            break;
          case 0x90:
            mode = Mode::ID;
            break;
          case 0xF0:
          default:
            mode = Mode::Data;
            break;
        }
      }
    }
  }
}

};
