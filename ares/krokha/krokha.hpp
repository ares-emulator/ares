#pragma once
//started: 2026-09-05

#include <ares/ares.hpp>
#include <vector>

#include <component/processor/i8080/i8080.hpp>

namespace ares::Krokha {
  #include <ares/inline.hpp>
  auto enumerate() -> std::vector<string>;
  auto load(Node::System& node, string name) -> bool;

  #include <krokha/cpu/cpu.hpp>
  #include <krokha/vdp/vdp.hpp>
  #include <krokha/psg/psg.hpp>

  #include <krokha/system/system.hpp>
  #include <krokha/cartridge/cartridge.hpp>
}
