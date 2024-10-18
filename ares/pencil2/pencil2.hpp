#pragma once

#include <ares/ares.hpp>

#include <component/processor/z80/z80.hpp>
#include <component/video/tms9918/tms9918.hpp>
#include <component/audio/sn76489/sn76489.hpp>

namespace ares::Pencil2 {
  #include <ares/inline.hpp>
  auto enumerate() -> vector<string>;
  auto load(Node::System& node, string name) -> bool;

  struct Model {
    inline static auto Pencil2() -> bool;
  };

  struct Region {
    inline static auto PAL() -> bool;
  };

  #include <pencil2/controller/controller.hpp>

  #include <pencil2/cpu/cpu.hpp>
  #include <pencil2/vdp/vdp.hpp>
  #include <pencil2/psg/psg.hpp>

  #include <pencil2/system/system.hpp>
  #include <pencil2/cartridge/cartridge.hpp>
}
