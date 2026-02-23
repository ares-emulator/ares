#pragma once

#include <ares/ares.hpp>
#include <vector>

#include <component/processor/mos6502/mos6502.hpp>

namespace ares::Atari2600 {
  #include <ares/inline.hpp>
  auto enumerate() -> std::vector<string>;
  auto load(Node::System& node, string name) -> bool;

  struct Region {
    static inline auto NTSC() -> bool;
    static inline auto PAL() -> bool;
    static inline auto SECAM() -> bool;
  };

  #include <a26/controller/controller.hpp>
  #include <a26/system/system.hpp>
  #include <a26/cartridge/cartridge.hpp>
  #include <a26/cpu/cpu.hpp>
  #include <a26/tia/tia.hpp>
  #include <a26/riot/riot.hpp>
}

