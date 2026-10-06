ares_components(mos6502 ay38910)

ares_add_sources(
  CORE #
    gamate
  UNITY #
    gamate.cpp
  PRIMARY #
    cpu/cpu.cpp
    vdp/vdp.cpp
    psg/psg.cpp
    system/system.cpp
    cartridge/cartridge.cpp
)

ares_add_sources(
  CORE #
    gamate
  INCLUDED #
    gamate.hpp
    CMakeLists.txt
)

ares_add_sources(
  CORE #
    gamate
  INCLUDED #
    cartridge/cartridge.hpp
    cartridge/serialization.cpp
    cartridge/slot.cpp
    cartridge/slot.hpp
)

ares_add_sources(
  CORE #
    gamate
  INCLUDED #
    cpu/cpu.hpp
    cpu/debugger.cpp
    cpu/memory.cpp
    cpu/serialization.cpp
)

ares_add_sources(
  CORE #
    gamate
  INCLUDED #
    psg/psg.hpp
    psg/serialization.cpp
)

ares_add_sources(
  CORE #
    gamate
  INCLUDED #
    system/controls.cpp
    system/serialization.cpp
    system/system.hpp
)

ares_add_sources(
  CORE #
    gamate
  INCLUDED #
    vdp/color.cpp
    vdp/serialization.cpp
    vdp/vdp.hpp
)
