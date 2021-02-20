#pragma once
//started: 2020-04-28

#include <ares/ares.hpp>
#include <nall/hashset.hpp>
#include <nall/recompiler/amd64/amd64.hpp>
#include <nmmintrin.h>
using v128 = __m128i;

#if defined(VULKAN)
  #include <n64/vulkan/vulkan.hpp>
#endif

namespace ares::Nintendo64 {
  auto enumerate() -> vector<string>;
  auto load(Node::System& node, string name) -> bool;
  auto option(string name, string value) -> bool;

  struct Region {
    static inline auto NTSC() -> bool;
    static inline auto PAL() -> bool;
  };

  struct Thread {
    auto reset() -> void {
      clock = 0;
    }

    auto serialize(serializer& s) -> void {
      s(clock);
    }

    s64 clock;
  };

  #include <n64/accuracy.hpp>
  #include <n64/memory/memory.hpp>
  #include <n64/system/system.hpp>
  #include <n64/cartridge/cartridge.hpp>
  #include <n64/controller/controller.hpp>
  #include <n64/dd/dd.hpp>
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
