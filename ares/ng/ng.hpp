#pragma once
//started: 2021-05-18

#include <ares/ares.hpp>

#include <component/processor/m68000/m68000.hpp>
#include <component/processor/z80/z80.hpp>
#include <component/audio/ym2610/ym2610.hpp>

namespace ares::NeoGeo {
  #include <ares/inline.hpp>
  auto enumerate() -> vector<string>;
  auto load(Node::System& node, string name) -> bool;

  struct Model {
    inline static auto NeoGeoAES() -> bool;
    inline static auto NeoGeoMVS() -> bool;
  };

  #include <ng/system/system.hpp>
  #include <ng/cartridge/cartridge.hpp>
  #include <ng/controller/controller.hpp>
  #include <ng/card/card.hpp>
  #include <ng/cpu/cpu.hpp>
  #include <ng/apu/apu.hpp>
  #include <ng/lspc/lspc.hpp>
  #include <ng/opnb/opnb.hpp>
}
