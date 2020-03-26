#pragma once
//started: 2004-10-14

#include <ares/ares.hpp>

#include <component/processor/arm7tdmi/arm7tdmi.hpp>
#include <component/processor/gsu/gsu.hpp>
#include <component/processor/hg51b/hg51b.hpp>
#include <component/processor/spc700/spc700.hpp>
#include <component/processor/upd96050/upd96050.hpp>
#include <component/processor/wdc65816/wdc65816.hpp>

#if defined(CORE_GB)
  #include <gb/gb.hpp>
#endif

namespace ares::SuperFamicom {
  #include <ares/inline.hpp>

  struct Region {
    static inline auto NTSC() -> bool;
    static inline auto PAL() -> bool;
  };

  #include <sfc/system/system.hpp>
  #include <sfc/memory/memory.hpp>
  #include <sfc/ppu/counter/counter.hpp>

  #include <sfc/cpu/cpu.hpp>
  #include <sfc/smp/smp.hpp>
  #include <sfc/dsp/dsp.hpp>
  #include <sfc/ppu/ppu.hpp>

  #include <sfc/controller/controller.hpp>
  #include <sfc/expansion/expansion.hpp>
  #include <sfc/coprocessor/coprocessor.hpp>
  #include <sfc/slot/slot.hpp>
  #include <sfc/cartridge/cartridge.hpp>

  #include <sfc/memory/memory-inline.hpp>
  #include <sfc/ppu/counter/counter-inline.hpp>
}

#include <sfc/interface/interface.hpp>
