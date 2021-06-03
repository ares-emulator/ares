#pragma once
//started: 2016-07-08

#include <ares/ares.hpp>

#include <component/processor/m68000/m68000.hpp>
#include <component/processor/z80/z80.hpp>
#include <component/processor/sh2/sh2.hpp>
#include <component/processor/ssp1601/ssp1601.hpp>
#include <component/audio/sn76489/sn76489.hpp>
#include <component/audio/ym2612/ym2612.hpp>
#include <component/eeprom/m24c/m24c.hpp>

namespace ares::MegaDrive {
  #include <ares/inline.hpp>
  auto enumerate() -> vector<string>;
  auto load(Node::System& node, string name) -> bool;

  enum : u32 {
    Byte = 0,
    Word = 1,
  };

  struct Region {
    inline static auto NTSC() -> bool;
    inline static auto NTSCJ() -> bool;
    inline static auto NTSCU() -> bool;
    inline static auto PAL() -> bool;
  };

  inline static auto Mega32X() -> bool;
  inline static auto MegaCD() -> bool;

  #include <md/controller/controller.hpp>
  #include <md/bus/bus.hpp>
  #include <md/cpu/cpu.hpp>
  #include <md/apu/apu.hpp>
  #include <md/vdp/vdp.hpp>
  #include <md/opn2/opn2.hpp>
  #include <md/m32x/m32x.hpp>
  #include <md/mcd/mcd.hpp>
  #include <md/system/system.hpp>
  #include <md/cartridge/cartridge.hpp>
  #include <md/bus/inline.hpp>
}
