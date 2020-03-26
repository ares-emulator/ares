#pragma once
//started: 2018-12-28

#include <ares/ares.hpp>

#include <component/processor/z80/z80.hpp>
#include <component/video/tms9918/tms9918.hpp>
#include <component/video/v9938/v9938.hpp>
#include <component/audio/ay38910/ay38910.hpp>
#include <component/audio/ym2413/ym2413.hpp>

namespace ares::MSX {
  #include <ares/inline.hpp>

  struct Model {
    inline static auto MSX() -> bool;
    inline static auto MSX2() -> bool;
  };

  struct Region {
    inline static auto NTSC() -> bool;
    inline static auto PAL() -> bool;
  };

  #include <msx/system/system.hpp>
  #include <msx/keyboard/keyboard.hpp>
  #include <msx/cartridge/cartridge.hpp>
  #include <msx/controller/controller.hpp>

  #include <msx/cpu/cpu.hpp>
  #include <msx/vdp/vdp.hpp>
  #include <msx/psg/psg.hpp>
}

#include <msx/interface/interface.hpp>
