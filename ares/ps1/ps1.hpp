#pragma once
//started: 2020-06-17

#include <ares/ares.hpp>
#include <span>
#include <vector>
#include <nall/hashset.hpp>
#include <component/processor/m68hc05/m68hc05.hpp>
#include <component/eeprom/parallel-flash/parallel-flash.hpp>
#include <component/eeprom/m93lcx6/m93lcx6.hpp>
#include <component/scsi/scsi.hpp>
#include <component/scsi/ncr53cf96/ncr53cf96.hpp>
#include <component/scsi/cdrom/cdrom.hpp>
#include <component/flash/mbm29f016a/mbm29f016a.hpp>
#include <component/flash/lh28f400/lh28f400.hpp>
#include <component/io/upd4701/upd4701.hpp>

namespace ares::PlayStation {
  #include <ares/inline.hpp>
  auto enumerate() -> std::vector<string>;
  auto load(Node::System& node, string name) -> bool;
  auto option(string name, string value) -> bool;

  enum : bool { Read = 0, Write = 1 };
  enum : u32  { Byte = 1, Half = 2, Word = 4 };

  struct Region {
    inline static auto NTSCJ() -> bool;
    inline static auto NTSCU() -> bool;
    inline static auto PAL() -> bool;
  };

  #include <ps1/accuracy.hpp>
  #include <ps1/memory/memory.hpp>
  #include <ps1/system/system.hpp>
  #include <ps1/expansion/expansion.hpp>
  #include <ps1/disc/disc.hpp>
  #include <ps1/cpu/cpu.hpp>
  #include <ps1/gpu/gpu.hpp>
  #include <ps1/spu/spu.hpp>
  #include <ps1/mdec/mdec.hpp>
  #include <ps1/interrupt/interrupt.hpp>
  #include <ps1/peripheral/peripheral.hpp>
  #include <ps1/dma/dma.hpp>
  #include <ps1/timer/timer.hpp>
  #include <ps1/memory/bus.hpp>
}
