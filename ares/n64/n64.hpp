#pragma once
//started: 2020-04-28

#include <ares/ares.hpp>
#include <nmmintrin.h>
using s128 = __m128i;

namespace ares::Nintendo64 {
  struct Accuracy {
    //enable all accuracy flags
    static constexpr bool Reference = 0;

    struct CPU {
      //exceptions when the CPU accesses unaligned memory addresses
      static constexpr bool AddressErrors = 0 | Reference;
    };
  };

  struct Region {
    static inline auto NTSC() -> bool;
    static inline auto PAL() -> bool;
  };

  struct Thread {
    auto reset() -> void {
      clock = 0;
    }

    auto serialize(serializer& s) -> void {
      s.integer(clock);
    }

    i64 clock;
  };

  #include <n64/memory/memory.hpp>
  #include <n64/system/system.hpp>
  #include <n64/cartridge/cartridge.hpp>
  #include <n64/controller/controller.hpp>
  #include <n64/mi/mi.hpp>
  #include <n64/vi/vi.hpp>
  #include <n64/ai/ai.hpp>
  #include <n64/pi/pi.hpp>
  #include <n64/ri/ri.hpp>
  #include <n64/si/si.hpp>
  #include <n64/rdram/rdram.hpp>
  #include <n64/cpu/cpu.hpp>
  #include <n64/rdp/rdp.hpp>
  #include <n64/rsp/rsp.hpp>
  #include <n64/memory/bus.hpp>
}

#include <n64/interface/interface.hpp>
