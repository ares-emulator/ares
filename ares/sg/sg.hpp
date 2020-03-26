#pragma once
//started: 2019-02-19

#include <ares/ares.hpp>

#include <component/processor/z80/z80.hpp>
#include <component/video/tms9918/tms9918.hpp>
#include <component/audio/sn76489/sn76489.hpp>

namespace ares::SG1000 {
  #include <ares/inline.hpp>

  struct Model {
    inline static auto SG1000() -> bool;
    inline static auto SC3000() -> bool;
  };

  struct Region {
    inline static auto NTSC() -> bool;
    inline static auto PAL() -> bool;
  };

  #include <sg/controller/controller.hpp>

  #include <sg/cpu/cpu.hpp>
  #include <sg/vdp/vdp.hpp>
  #include <sg/psg/psg.hpp>

  #include <sg/system/system.hpp>
  #include <sg/cartridge/cartridge.hpp>
}

#include <sg/interface/interface.hpp>
