#pragma once
//started: 2010-12-27

#include <ares/ares.hpp>

#include <component/processor/sm83/sm83.hpp>
#include <component/eeprom/m93lcx6/m93lcx6.hpp>

namespace ares::GameBoy {
  #include <ares/inline.hpp>

  struct Model {
    inline static auto GameBoy() -> bool;
    inline static auto GameBoyColor() -> bool;
    inline static auto SuperGameBoy() -> bool;
  };

  #include <gb/system/system.hpp>
  #include <gb/bus/bus.hpp>
  #include <gb/cartridge/cartridge.hpp>
  #include <gb/cpu/cpu.hpp>
  #include <gb/ppu/ppu.hpp>
  #include <gb/apu/apu.hpp>
}

#include <gb/interface/interface.hpp>
