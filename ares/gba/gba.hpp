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

  enum : u32 {
    Load          = 1 << 0,  //load operation
    Store         = 1 << 1,  //store operation
    Prefetch      = 1 << 2,  //instruction fetch
    Byte          = 1 << 3,  // 8-bit access
    Half          = 1 << 4,  //16-bit access
    Word          = 1 << 5,  //32-bit access
    Signed        = 1 << 6,  //sign-extend
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
