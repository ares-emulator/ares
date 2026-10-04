#pragma once

#include <ares/ares.hpp>
#include <vector>

#include <component/processor/mos6502/mos6502.hpp>

namespace ares::Atari5200 {
  #include <ares/inline.hpp>

  auto enumerate() -> std::vector<string>;
  auto load(Node::System& node, string name) -> bool;

  struct Timing {
    static constexpr u32 MachineCyclesPerScanline = 114;
    static constexpr u32 ColorClocksPerMachineCycle = 2;
    static constexpr u32 ColorClocksPerScanline = MachineCyclesPerScanline * ColorClocksPerMachineCycle;
    static constexpr u32 SamplesPerColorClock = 2;
    static constexpr u32 SamplesPerMachineCycle = ColorClocksPerMachineCycle * SamplesPerColorClock;
    static constexpr u32 SamplesPerScanline = ColorClocksPerScanline * SamplesPerColorClock;
    static constexpr u32 ScanlinesPerFrame = 262;
    static constexpr u32 DisplayListFirstScanline = 8;
    static constexpr u32 DisplayListLastScanline = 247;
    static constexpr u32 VisibleFirstColorClock = 0x22;
    static constexpr u32 VisibleLastColorClock = 0xdd;
    static constexpr u32 WSYNCReleaseCycle = 105;
  };

  #include <a52/system/system.hpp>
  #include <a52/cartridge/cartridge.hpp>
  #include <a52/controller/controller.hpp>
  #include <a52/antic/antic.hpp>
  #include <a52/gtia/gtia.hpp>
  #include <a52/pokey/pokey.hpp>
  #include <a52/cpu/cpu.hpp>
}
