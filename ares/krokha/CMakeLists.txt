ares_components(i8080)

ares_add_sources(
  CORE #
    krokha
  UNITY #
    krokha.cpp
  PRIMARY #
    cpu/cpu.cpp
    vdp/vdp.cpp
    psg/psg.cpp
    system/system.cpp
    cartridge/cartridge.cpp
)

ares_add_sources(
  CORE #
    krokha
  INCLUDED #
    krokha.hpp
    CMakeLists.txt
)

ares_add_sources(
  CORE #
    krokha
  INCLUDED #
    cartridge/cartridge.hpp
    cartridge/serialization.cpp
    cartridge/slot.cpp
    cartridge/slot.hpp
)

ares_add_sources(
  CORE #
    krokha
  INCLUDED #
    cpu/cpu.hpp
    cpu/debugger.cpp
    cpu/memory.cpp
    cpu/serialization.cpp
)

ares_add_sources(
  CORE #
    krokha
  INCLUDED #
    psg/psg.hpp
    psg/serialization.cpp
)

ares_add_sources(
  CORE #
    krokha
  INCLUDED #
    system/controls.cpp
    system/serialization.cpp
    system/system.hpp
)

ares_add_sources(
  CORE #
    krokha
  INCLUDED #
    vdp/serialization.cpp
    vdp/vdp.hpp
)
