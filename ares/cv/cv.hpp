#pragma once
//started: 2019-02-19

#include <ares/ares.hpp>

#include <component/processor/z80/z80.hpp>
#include <component/video/tms9918/tms9918.hpp>
#include <component/audio/sn76489/sn76489.hpp>

namespace ares::ColecoVision {
  #include <ares/inline.hpp>

  struct Model {
    inline static auto ColecoVision() -> bool;
    inline static auto ColecoAdam() -> bool;
  };

  struct Region {
    inline static auto NTSC() -> bool;
    inline static auto PAL() -> bool;
  };

  #include <cv/controller/controller.hpp>

  #include <cv/cpu/cpu.hpp>
  #include <cv/vdp/vdp.hpp>
  #include <cv/psg/psg.hpp>

  #include <cv/system/system.hpp>
  #include <cv/cartridge/cartridge.hpp>
}

#include <cv/interface/interface.hpp>
