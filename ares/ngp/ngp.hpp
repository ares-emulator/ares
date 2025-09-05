#pragma once
//started: 2019-01-03

#include <ares/ares.hpp>
#include <vector>

#include <component/processor/tlcs900h/tlcs900h.hpp>
#include <component/processor/z80/z80.hpp>
#include <component/audio/t6w28/t6w28.hpp>

namespace ares::NeoGeoPocket {
  #include <ares/inline.hpp>
  auto enumerate() -> std::vector<string>;
  auto load(Node::System& node, string name) -> bool;

  struct Model {
    inline static auto NeoGeoPocket() -> bool;
    inline static auto NeoGeoPocketColor() -> bool;
  };

  #include <ngp/system/system.hpp>
  #include <ngp/cartridge/cartridge.hpp>
  #include <ngp/cpu/cpu.hpp>
  #include <ngp/apu/apu.hpp>
  #include <ngp/kge/kge.hpp>
  #include <ngp/psg/psg.hpp>
}
