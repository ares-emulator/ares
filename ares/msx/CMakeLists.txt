ares_components(
  z80
  tms9918
  v9938
  ay38910
  ym2413
)

ares_add_sources(
  CORE #
    msx
  UNITY #
    msx.cpp
  PRIMARY #
    system/system.cpp
    keyboard/keyboard.cpp
    cartridge/cartridge.cpp
    controller/controller.cpp
    cpu/cpu.cpp
    vdp/vdp.cpp
    psg/psg.cpp
    tape/tape.cpp
    rtc/rtc.cpp
)

ares_add_sources(
  CORE #
    msx
  INCLUDED #
    msx.hpp
    CMakeLists.txt
)

ares_add_sources(
  CORE #
    msx
  INCLUDED #
    cartridge/cartridge.hpp
    cartridge/serialization.cpp
    cartridge/slot.cpp
    cartridge/slot.hpp
)

ares_add_sources(
  CORE #
    msx
  INCLUDED #
    cartridge/board/asc16.cpp
    cartridge/board/asc8.cpp
    cartridge/board/board.cpp
    cartridge/board/board.hpp
    cartridge/board/cross-blaim.cpp
    cartridge/board/konami-scc.cpp
    cartridge/board/konami.cpp
    cartridge/board/linear.cpp
    cartridge/board/mirrored.cpp
    cartridge/board/super-lode-runner.cpp
    cartridge/board/super-pierrot.cpp
)

ares_add_sources(
  CORE #
    msx
  INCLUDED #
    controller/controller.hpp
    controller/mux.cpp
    controller/mux.hpp
    controller/port.cpp
    controller/port.hpp
)

ares_add_sources(
  CORE #
    msx
  INCLUDED #
    controller/gamepad/gamepad.cpp
    controller/gamepad/gamepad.hpp
)

ares_add_sources(
  CORE #
    msx
  INCLUDED #
    controller/vaus-paddle/vaus-paddle.cpp
    controller/vaus-paddle/vaus-paddle.hpp
)

ares_add_sources(
  CORE #
    msx
  INCLUDED #
    cpu/cpu.hpp
    cpu/debugger.cpp
    cpu/memory.cpp
    cpu/serialization.cpp
)

ares_add_sources(
  CORE #
    msx
  INCLUDED #
    keyboard/keyboard.hpp
    keyboard/serialization.cpp
)

ares_add_sources(
  CORE #
    msx
  INCLUDED #
    psg/psg.hpp
    psg/serialization.cpp
)

ares_add_sources(
  CORE #
    msx
  INCLUDED #
    rtc/rtc.hpp
    rtc/serialization.cpp
)

ares_add_sources(
  CORE #
    msx
  INCLUDED #
    system/serialization.cpp
    system/system.hpp
)

ares_add_sources(
  CORE #
    msx
  INCLUDED #
    tape/deck.cpp
    tape/deck.hpp
    tape/tape.hpp
    tape/tray.cpp
    tape/tray.hpp
)

ares_add_sources(
  CORE #
    msx
  INCLUDED #
    vdp/color.cpp
    vdp/serialization.cpp
    vdp/vdp.hpp
)
