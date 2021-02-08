#pragma once
//started: 2010-12-27

#include <ares/ares.hpp>

#include <component/processor/sm83/sm83.hpp>
#include <component/eeprom/m93lcx6/m93lcx6.hpp>

namespace ares::GameBoy {
  #include <ares/inline.hpp>
  auto enumerate() -> vector<string>;
  auto load(Node::System& node, string name) -> bool;

  struct Model {
    inline static auto GameBoy() -> bool;
    inline static auto GameBoyColor() -> bool;
    inline static auto SuperGameBoy() -> bool;
  };

  struct SuperGameBoyInterface {
    virtual auto ppuHreset() -> void = 0;
    virtual auto ppuVreset() -> void = 0;
    virtual auto ppuWrite(n2 color) -> void = 0;
    virtual auto joypWrite(n1 p14, n1 p15) -> void = 0;
  };

  #include <gb/system/system.hpp>
  #include <gb/bus/bus.hpp>
  #include <gb/cartridge/cartridge.hpp>
  #include <gb/cpu/cpu.hpp>
  #include <gb/ppu/ppu.hpp>
  #include <gb/apu/apu.hpp>
}
