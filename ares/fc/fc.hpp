#pragma once
//started: 2011-09-05

#include <ares/ares.hpp>

#include <component/processor/mos6502/mos6502.hpp>
#include <component/audio/ym2149/ym2149.hpp>
#include <component/audio/ym2413/ym2413.hpp>
#include <component/eeprom/x24c01/x24c01.hpp>

namespace ares::Famicom {
  #include <ares/inline.hpp>

  struct Region {
    static inline auto NTSCJ() -> bool;
    static inline auto NTSCU() -> bool;
    static inline auto PAL() -> bool;
  };

  #include <fc/controller/controller.hpp>
  #include <fc/system/system.hpp>
  #include <fc/cartridge/cartridge.hpp>
  #include <fc/cpu/cpu.hpp>
  #include <fc/apu/apu.hpp>
  #include <fc/ppu/ppu.hpp>
  #include <fc/fds/fds.hpp>
}

#include <fc/interface/interface.hpp>
