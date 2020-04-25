#pragma once
//started: 2017-01-11

#include <ares/ares.hpp>

#include <component/processor/huc6280/huc6280.hpp>
#include <component/audio/msm5205/msm5205.hpp>

namespace ares::PCEngine {
  #include <ares/inline.hpp>

  struct Model {
    inline static auto PCEngine() -> bool;
    inline static auto PCEngineDuo() -> bool;
    inline static auto SuperGrafx() -> bool;
  };

  struct Region {
    inline static auto NTSCJ() -> bool;
    inline static auto NTSCU() -> bool;
  };

  #include <pce/controller/controller.hpp>

  #include <pce/cpu/cpu.hpp>
  #include <pce/vdp/vdp.hpp>
  #include <pce/psg/psg.hpp>
  #include <pce/pcd/pcd.hpp>

  #include <pce/system/system.hpp>
  #include <pce/cartridge/cartridge.hpp>
}

#include <pce/interface/interface.hpp>
