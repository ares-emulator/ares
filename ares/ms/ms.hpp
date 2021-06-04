#pragma once
//started: 2016-08-17

#include <ares/ares.hpp>

#include <component/processor/z80/z80.hpp>
#include <component/audio/sn76489/sn76489.hpp>
#include <component/audio/ym2413/ym2413.hpp>

namespace ares::MasterSystem {
  #include <ares/inline.hpp>
  auto enumerate() -> vector<string>;
  auto load(Node::System& node, string name) -> bool;

  struct Model {
    inline static auto MarkIII() -> bool;
    inline static auto MasterSystemI() -> bool;
    inline static auto MasterSystemII() -> bool;
    inline static auto GameGear() -> bool;
  };

  struct Device {
    inline static auto MasterSystem() -> bool;
    inline static auto GameGear() -> bool;
  };

  struct Mode {
    inline static auto MasterSystem() -> bool;
    inline static auto GameGear() -> bool;
  };

  struct Display {
    inline static auto CRT() -> bool;
    inline static auto LCD() -> bool;
  };

  struct Region {
    inline static auto NTSCJ() -> bool;
    inline static auto NTSCU() -> bool;
    inline static auto PAL() -> bool;
  };

  #include <ms/controller/controller.hpp>
  #include <ms/expansion/expansion.hpp>

  #include <ms/cpu/cpu.hpp>
  #include <ms/vdp/vdp.hpp>
  #include <ms/psg/psg.hpp>
  #include <ms/opll/opll.hpp>

  #include <ms/system/system.hpp>
  #include <ms/cartridge/cartridge.hpp>
}
