ares_components(z80 ay38910)

ares_add_sources(
  CORE #
    spec
  UNITY #
    spec.cpp
  PRIMARY #
    keyboard/keyboard.cpp
    expansion/expansion.cpp
    system/system.cpp
    cpu/cpu.cpp
    ula/ula.cpp
    psg/psg.cpp
    tape/tape.cpp
)

ares_add_sources(
  CORE #
    spec
  INCLUDED #
    spec.hpp
    CMakeLists.txt
)

ares_add_sources(
  CORE #
    spec
  INCLUDED #
    cpu/cpu.hpp
    cpu/debugger.cpp
    cpu/memory.cpp
    cpu/serialization.cpp
)

ares_add_sources(
  CORE #
    spec
  INCLUDED #
    expansion/expansion.hpp
    expansion/port.cpp
    expansion/port.hpp
    expansion/kempston/kempston.cpp
    expansion/kempston/kempston.hpp
)

ares_add_sources(
  CORE #
    spec
  INCLUDED #
    keyboard/keyboard.hpp
)

ares_add_sources(
  CORE #
    spec
  INCLUDED #
    psg/psg.hpp
    psg/serialization.cpp
)

ares_add_sources(
  CORE #
    spec
  INCLUDED #
    system/serialization.cpp
    system/system.hpp
)

ares_add_sources(
  CORE #
    spec
  INCLUDED #
    tape/deck.cpp
    tape/deck.hpp
    tape/tape.hpp
    tape/tray.cpp
    tape/tray.hpp
)

ares_add_sources(
  CORE #
    spec
  INCLUDED #
    ula/color.cpp
    ula/serialization.cpp
    ula/ula.hpp
)
