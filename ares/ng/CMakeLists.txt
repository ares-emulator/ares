ares_components(m68000 z80)

target_link_libraries(ares PRIVATE ymfm)

ares_add_sources(
  CORE #
    ng
  UNITY #
    ng.cpp
  PRIMARY #
    system/system.cpp
    cpu/cpu.cpp
    apu/apu.cpp
    lspc/lspc.cpp
    opnb/opnb.cpp
    cartridge/cartridge.cpp
    controller/controller.cpp
    card/card.cpp
)

ares_add_sources(
  CORE #
    ng
  INCLUDED #
    ng.hpp
    CMakeLists.txt
)

ares_add_sources(
  CORE #
    ng
  INCLUDED #
    apu/apu.hpp
    apu/debugger.cpp
    apu/memory.cpp
    apu/serialization.cpp
)

ares_add_sources(
  CORE #
    ng
  INCLUDED #
    card/card.hpp
    card/debugger.cpp
    card/serialization.cpp
    card/slot.cpp
    card/slot.hpp
)

ares_add_sources(
  CORE #
    ng
  INCLUDED #
    cartridge/cartridge.hpp
    cartridge/serialization.cpp
    cartridge/slot.cpp
    cartridge/slot.hpp
    cartridge/board/board.cpp
    cartridge/board/board.hpp
    cartridge/board/jockey-gp.cpp
    cartridge/board/mslugx.cpp
    cartridge/board/rom.cpp
)

ares_add_sources(
  CORE #
    ng
  INCLUDED #
    controller/controller.hpp
    controller/port.cpp
    controller/port.hpp
)

ares_add_sources(
  CORE #
    ng
  INCLUDED #
    controller/arcade-stick/arcade-stick.cpp
    controller/arcade-stick/arcade-stick.hpp
)

ares_add_sources(
  CORE #
    ng
  INCLUDED #
    cpu/cpu.hpp
    cpu/debugger.cpp
    cpu/memory.cpp
    cpu/serialization.cpp
)

ares_add_sources(
  CORE #
    ng
  INCLUDED #
    lspc/color.cpp
    lspc/debugger.cpp
    lspc/lspc.hpp
    lspc/render.cpp
    lspc/serialization.cpp
)

ares_add_sources(
  CORE #
    ng
  INCLUDED #
    opnb/opnb.hpp
    opnb/serialization.cpp
)

ares_add_sources(
  CORE #
    ng
  INCLUDED #
    system/debugger.cpp
    system/serialization.cpp
    system/system.hpp
)
