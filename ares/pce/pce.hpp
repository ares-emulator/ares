#pragma once
//started: 2017-01-11

#include <ares/ares.hpp>

#include <component/processor/huc6280/huc6280.hpp>
#include <component/audio/msm5205/msm5205.hpp>

namespace ares::PCEngine {
  #include <ares/inline.hpp>
  auto enumerate() -> vector<string>;
  auto load(Node::System& node, string name) -> bool;
  auto option(string name, string value) -> bool;

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
  #include <pce/vdp-performance/vdp.hpp>
  #include <pce/psg/psg.hpp>
  #include <pce/pcd/pcd.hpp>

  #include <pce/system/system.hpp>
  #include <pce/cartridge/cartridge.hpp>
}
