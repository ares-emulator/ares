#pragma once
//started: 2020-06-17

#include <ares/ares.hpp>
#include <nall/hashset.hpp>
#include <nall/recompiler/amd64/amd64.hpp>

namespace ares::PlayStation {
  struct Accuracy {
    //enable all accuracy flags
    static constexpr bool Reference = 1;

    struct CPU {
      //0 = dynamic recompiler; 1 = interpreter
      static constexpr bool Interpreter = 1 | Reference;

      //exceptions when the CPU accesses unaligned memory addresses
      static constexpr bool AlignmentErrors = 0 | Reference;
    };
  };

  struct Region {
    inline static auto NTSCJ() -> bool;
    inline static auto NTSCU() -> bool;
    inline static auto PAL() -> bool;
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

  #include <ps1/memory/memory.hpp>
  #include <ps1/system/system.hpp>
  #include <ps1/disc/disc.hpp>
  #include <ps1/controller/controller.hpp>
  #include <ps1/cpu/cpu.hpp>
  #include <ps1/gpu/gpu.hpp>
  #include <ps1/spu/spu.hpp>
  #include <ps1/interrupt/interrupt.hpp>
  #include <ps1/peripheral/peripheral.hpp>
  #include <ps1/dma/dma.hpp>
  #include <ps1/timer/timer.hpp>
  #include <ps1/memory/bus.hpp>
}

#include <ps1/interface/interface.hpp>
