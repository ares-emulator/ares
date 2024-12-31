ares_components(z80 tms9918 sn76489)

ares_add_sources(
  CORE #
    cv
  UNITY #
    cv.cpp
  PRIMARY #
    cpu/cpu.cpp
    vdp/vdp.cpp
    psg/psg.cpp
    system/system.cpp
    cartridge/cartridge.cpp
    controller/controller.cpp
)

ares_add_sources(
  CORE #
    cv
  INCLUDED #
    cv.hpp
    CMakeLists.txt
)

ares_add_sources(
  CORE #
    cv
  INCLUDED #
    cartridge/cartridge.hpp
    cartridge/serialization.cpp
    cartridge/slot.cpp
    cartridge/slot.hpp
)

ares_add_sources(
  CORE #
    cv
  INCLUDED #
    cartridge/board/board.cpp
    cartridge/board/board.hpp
    cartridge/board/coleco.cpp
    cartridge/board/megacart.cpp
    cartridge/board/xin1.cpp
)

ares_add_sources(
  CORE #
    cv
  INCLUDED #
    controller/controller.hpp
    controller/port.cpp
    controller/port.hpp
)

ares_add_sources(
  CORE #
    cv
  INCLUDED #
    controller/gamepad/gamepad.cpp
    controller/gamepad/gamepad.hpp
)

ares_add_sources(
  CORE #
    cv
  INCLUDED #
    cpu/cpu.hpp
    cpu/debugger.cpp
    cpu/memory.cpp
    cpu/serialization.cpp
)

ares_add_sources(
  CORE #
    cv
  INCLUDED #
    psg/psg.hpp
    psg/serialization.cpp
)

ares_add_sources(
  CORE #
    cv
  INCLUDED #
    system/controls.cpp
    system/serialization.cpp
    system/system.hpp
)

ares_add_sources(
  CORE #
    cv
  INCLUDED #
    vdp/color.cpp
    vdp/serialization.cpp
    vdp/vdp.hpp
)
