#pragma once
//started: 2020-04-28

#define XXH_INLINE_ALL
#include <xxhash.h>
#include <ares/ares.hpp>
#include <nall/float-env.hpp>
#include <nall/hashset.hpp>
#include <nall/recompiler/generic/generic.hpp>
#include <component/processor/sm5k/sm5k.hpp>

#if defined(ARCHITECTURE_AMD64)
#include <nmmintrin.h>
using v128 = __m128i;
#elif defined(ARCHITECTURE_ARM64)
#include <sse2neon.h>
using v128 = __m128i;
#endif

#if defined(VULKAN)
  #include <n64/vulkan/vulkan.hpp>
#endif

#if defined(MAME_RDP)
class n64_state;
#endif

namespace ares::Nintendo64 {
  auto enumerate() -> vector<string>;
  auto load(Node::System& node, string name) -> bool;
  auto option(string name, string value) -> bool;

  enum : u32 { Read, Write };
  enum : u32 { Byte = 1, Half = 2, Word = 4, Dual = 8 };

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

  struct Queue : priority_queue<u32[512]> {
    enum : u32 {
      RSP_DMA,
      PI_DMA_Read,
      PI_DMA_Write,
      PI_BUS_Write,
      SI_DMA_Read,
      SI_DMA_Write,
    };
  };
  extern Queue queue;

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
  #include <n64/pif/pif.hpp>
  #include <n64/ri/ri.hpp>
  #include <n64/si/si.hpp>
  #include <n64/rdram/rdram.hpp>
  #include <n64/cpu/cpu.hpp>
  #include <n64/rsp/rsp.hpp>
  #include <n64/rdp/rdp.hpp>
  #include <n64/memory/bus.hpp>
  #include <n64/pi/bus.hpp>
}
