#pragma once
//started: 2012-03-19

#include <ares/ares.hpp>

#include <component/processor/arm7tdmi/arm7tdmi.hpp>

namespace ares::GameBoyAdvance {
  #include <ares/inline.hpp>

  enum : uint {           //mode flags for bus read, write:
    Nonsequential =   1,  //N cycle
    Sequential    =   2,  //S cycle
    Prefetch      =   4,  //instruction fetch (eligible for prefetch)
    Byte          =   8,  //8-bit access
    Half          =  16,  //16-bit access
    Word          =  32,  //32-bit access
    Load          =  64,  //load operation
    Store         = 128,  //store operation
    Signed        = 256,  //sign extended
  };

  struct Model {
    inline static auto GameBoyAdvance() -> bool;
    inline static auto GameBoyPlayer() -> bool;
  };

  #include <gba/memory/memory.hpp>
  #include <gba/system/system.hpp>
  #include <gba/cartridge/cartridge.hpp>
  #include <gba/player/player.hpp>
  #include <gba/cpu/cpu.hpp>
  #include <gba/ppu/ppu.hpp>
  #include <gba/apu/apu.hpp>
}

#include <gba/interface/interface.hpp>
