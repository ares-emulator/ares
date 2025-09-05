#pragma once
//started: 2023-07-19

#include <ares/ares.hpp>
#include <vector>

#include <component/processor/z80/z80.hpp>
#include <component/video/tms9918/tms9918.hpp>
#include <component/audio/ay38910/ay38910.hpp>

namespace ares::MyVision {
  #include <ares/inline.hpp>
  auto enumerate() -> std::vector<string>;
  auto load(Node::System& node, string name) -> bool;

  #include <myvision/cpu/cpu.hpp>
  #include <myvision/vdp/vdp.hpp>
  #include <myvision/psg/psg.hpp>

  #include <myvision/system/system.hpp>
  #include <myvision/cartridge/cartridge.hpp>
}
