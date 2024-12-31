ares_components(tlcs900h z80 t6w28)

ares_add_sources(
  CORE #
    ngp
  UNITY #
    ngp.cpp
  PRIMARY #
    system/system.cpp
    cartridge/cartridge.cpp
    cpu/cpu.cpp
    apu/apu.cpp
    kge/kge.cpp
    psg/psg.cpp
)

ares_add_sources(
  CORE #
    ngp
  INCLUDED #
    ngp.hpp
    CMakeLists.txt
)

ares_add_sources(
  CORE #
    ngp
  INCLUDED #
    apu/apu.hpp
    apu/debugger.cpp
    apu/memory.cpp
    apu/serialization.cpp
)

ares_add_sources(
  CORE #
    ngp
  INCLUDED #
    cartridge/cartridge.hpp
    cartridge/debugger.cpp
    cartridge/flash.cpp
    cartridge/flash.hpp
    cartridge/serialization.cpp
    cartridge/slot.cpp
    cartridge/slot.hpp
)

ares_add_sources(
  CORE #
    ngp
  INCLUDED #
    cpu/adc.cpp
    cpu/cpu.hpp
    cpu/debugger.cpp
    cpu/interrupts.cpp
    cpu/io.cpp
    cpu/memory.cpp
    cpu/ports.cpp
    cpu/rtc.cpp
    cpu/serial.cpp
    cpu/serialization.cpp
    cpu/timers.cpp
    cpu/watchdog.cpp
)

ares_add_sources(
  CORE #
    ngp
  INCLUDED #
    kge/color.cpp
    kge/dac.cpp
    kge/kge.hpp
    kge/memory.cpp
    kge/plane.cpp
    kge/serialization.cpp
    kge/sprite.cpp
    kge/window.cpp
)

ares_add_sources(
  CORE #
    ngp
  INCLUDED #
    psg/psg.hpp
    psg/serialization.cpp
)

ares_add_sources(
  CORE #
    ngp
  INCLUDED #
    system/controls.cpp
    system/debugger.cpp
    system/serialization.cpp
    system/system.hpp
)
