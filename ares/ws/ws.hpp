#pragma once
//started: 2016-01-26

#include <ares/ares.hpp>

#include <component/processor/v30mz/v30mz.hpp>
#include <component/eeprom/m93lcx6/m93lcx6.hpp>

namespace ares::WonderSwan {
  #include <ares/inline.hpp>

  enum : uint { Byte = 1, Word = 2, Long = 4 };

  struct SoC {
    inline static auto ASWAN() -> bool;    //Grayscale
    inline static auto SPHINX() -> bool;   //Color + HyperVoice
    inline static auto SPHINX2() -> bool;  //Color + HyperVoice
  };

  struct Model {
    inline static auto WonderSwan() -> bool;         //Bandai  (ASWAN)   [SW-001]
    inline static auto WonderSwanColor() -> bool;    //Bandai  (SPHINX)  [WSC-001]
    inline static auto SwanCrystal() -> bool;        //Bandai  (SPHINX2) [SCT-001]
    inline static auto PocketChallengeV2() -> bool;  //Benesse (ASWAN)   [3AA902]
  };

  #include <ws/memory/memory.hpp>
  #include <ws/eeprom/eeprom.hpp>
  #include <ws/system/system.hpp>
  #include <ws/cartridge/cartridge.hpp>
  #include <ws/cpu/cpu.hpp>
  #include <ws/ppu/ppu.hpp>
  #include <ws/apu/apu.hpp>
}

#include <ws/interface/interface.hpp>
