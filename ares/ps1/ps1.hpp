#pragma once
//started: 2020-06-17

#include <ares/ares.hpp>
#include <nall/hashset.hpp>
#include <nall/recompiler/amd64/amd64.hpp>

namespace ares::PlayStation {
  struct Thread {
    auto reset() -> void {
      clock = 0;
    }

    auto serialize(serializer& s) -> void {
      s.integer(clock);
    }

    i64 clock;
  };

  #include <ps1/memory/memory.hpp>
  #include <ps1/system/system.hpp>
  #include <ps1/disc/disc.hpp>
  #include <ps1/cpu/cpu.hpp>
  #include <ps1/gpu/gpu.hpp>
  #include <ps1/spu/spu.hpp>
  #include <ps1/memory/bus.hpp>
}

#include <ps1/interface/interface.hpp>
