ares_components(z80 tms9918 ay38910)

ares_add_sources(
  CORE #
    myvision
  UNITY #
    myvision.cpp
  PRIMARY #
    cpu/cpu.cpp
    vdp/vdp.cpp
    psg/psg.cpp
    system/system.cpp
    cartridge/cartridge.cpp
)

ares_add_sources(
  CORE #
    myvision
  INCLUDED #
    myvision.hpp
    CMakeLists.txt
)

ares_add_sources(
  CORE #
    myvision
  INCLUDED #
    cartridge/cartridge.hpp
    cartridge/serialization.cpp
    cartridge/slot.cpp
    cartridge/slot.hpp
)

ares_add_sources(
  CORE #
    myvision
  INCLUDED #
    cpu/cpu.hpp
    cpu/debugger.cpp
    cpu/memory.cpp
    cpu/serialization.cpp
)

ares_add_sources(
  CORE #
    myvision
  INCLUDED #
    psg/psg.hpp
    psg/serialization.cpp
)

ares_add_sources(
  CORE #
    myvision
  INCLUDED #
    system/controls.cpp
    system/serialization.cpp
    system/system.hpp
)

ares_add_sources(
  CORE #
    myvision
  INCLUDED #
    vdp/color.cpp
    vdp/serialization.cpp
    vdp/vdp.hpp
)
