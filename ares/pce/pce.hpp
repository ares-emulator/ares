#pragma once

//license: GPLv3
//started: 2017-01-11

#include <emulator/emulator.hpp>

#include <component/processor/huc6280/huc6280.hpp>

namespace ares::PCEngine {
  #include <emulator/inline.hpp>

  struct Model {
    inline static auto PCEngine() -> bool;
    inline static auto SuperGrafx() -> bool;
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
