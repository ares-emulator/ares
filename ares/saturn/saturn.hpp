#pragma once
//started: 2021-03-31

#include <ares/ares.hpp>

#include <component/processor/sh2/sh2.hpp>
#include <component/processor/m68k/m68k.hpp>

namespace ares::Saturn {
  auto enumerate() -> vector<string>;
  auto load(Node::System& node, string name) -> bool;

  struct Region {
    static inline auto NTSCJ() -> bool;
    static inline auto NTSCU() -> bool;
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

  #include <saturn/system/system.hpp>
}
