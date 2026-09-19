#pragma once
//started: 2026-09-06

#include <ares/ares.hpp>
#include <vector>

#include <component/processor/mos6502/mos6502.hpp>
#include <component/audio/ay38910/ay38910.hpp>

namespace ares::Gamate {
  #include <ares/inline.hpp>
  auto enumerate() -> std::vector<string>;
  auto load(Node::System& node, string name) -> bool;

  //4.433MHz, the PAL colour subcarrier, divided down for everything on the board
  static constexpr u32 MasterClock = 4'433'000;
  static constexpr u32 CpuClock    = MasterClock / 2;   //NCR 65CX02
  static constexpr u32 PsgClock    = MasterClock / 64;  //AY clock/4, tone counters clock/16

  #include <gamate/cpu/cpu.hpp>
  #include <gamate/vdp/vdp.hpp>
  #include <gamate/psg/psg.hpp>

  #include <gamate/system/system.hpp>
  #include <gamate/cartridge/cartridge.hpp>
}
