#pragma once
//started: 2012-03-19

#include <ares/ares.hpp>

#include <component/processor/arm7tdmi/arm7tdmi.hpp>
#include <component/rtc/s3511a/s3511a.hpp>

namespace ares::GameBoyAdvance {
  #include <ares/inline.hpp>
  auto enumerate() -> vector<string>;
  auto load(Node::System& node, string name) -> bool;
  auto option(string name, string value) -> bool;

  enum : u32 {           //mode flags for bus read, write:
    Load          =   1,  //load operation
    Store         =   2,  //store operation
    Prefetch      =   4,  //instruction fetch (eligible for prefetch)
    Byte          =   8,  //8-bit access
    Half          =  16,  //16-bit access
    Word          =  32,  //32-bit access
    Signed        =  64,  //sign extended
    Nonsequential = 128,  //N cycle
    Sequential    = 256,  //S cycle
    DMA           = 512,  //DMA transaction
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
  #include <gba/display/display.hpp>
}
