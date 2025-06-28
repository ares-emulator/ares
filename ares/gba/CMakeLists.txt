ares_components(arm7tdmi)

ares_add_sources(
  CORE #
    gba
  UNITY #
    gba.cpp
  PRIMARY #
    memory/memory.cpp
    system/system.cpp
    cartridge/cartridge.cpp
    player/player.cpp
    cpu/cpu.cpp
    display/display.cpp
    ppu/ppu.cpp
    apu/apu.cpp
)

ares_add_sources(
  CORE #
    gba
  INCLUDED #
    gba.hpp
    CMakeLists.txt
)

ares_add_sources(
  CORE #
    gba
  INCLUDED #
    apu/apu.hpp
    apu/fifo.cpp
    apu/io.cpp
    apu/noise.cpp
    apu/sequencer.cpp
    apu/serialization.cpp
    apu/square.cpp
    apu/square1.cpp
    apu/square2.cpp
    apu/wave.cpp
)

ares_add_sources(
  CORE #
    gba
  INCLUDED #
    cartridge/cartridge.hpp
    cartridge/eeprom.cpp
    cartridge/flash.cpp
    cartridge/gpio.cpp
    cartridge/memory.hpp
    cartridge/mrom.cpp
    cartridge/rtc.cpp
    cartridge/serialization.cpp
    cartridge/slot.cpp
    cartridge/slot.hpp
    cartridge/sram.cpp
)

ares_add_sources(
  CORE #
    gba
  INCLUDED #
    cpu/bus.cpp
    cpu/coprocessor.cpp
    cpu/cpu.hpp
    cpu/debugger.cpp
    cpu/dma.cpp
    cpu/io.cpp
    cpu/keypad.cpp
    cpu/memory.cpp
    cpu/prefetch.cpp
    cpu/serialization.cpp
    cpu/timer.cpp
)

ares_add_sources(
  CORE #
    gba
  INCLUDED #
    display/display.hpp
    display/io.cpp
    display/serialization.cpp
)

ares_add_sources(
  CORE #
    gba
  INCLUDED #
    memory/memory.hpp
)

ares_add_sources(
  CORE #
    gba
  INCLUDED #
    player/player.hpp
    player/serialization.cpp
)

ares_add_sources(
  CORE #
    gba
  INCLUDED #
    ppu/background.cpp
    ppu/color.cpp
    ppu/dac.cpp
    ppu/debugger.cpp
    ppu/io.cpp
    ppu/memory.cpp
    ppu/object.cpp
    ppu/ppu.hpp
    ppu/serialization.cpp
    ppu/window.cpp
)

ares_add_sources(
  CORE #
    gba
  INCLUDED #
    system/bios.cpp
    system/controls.cpp
    system/serialization.cpp
    system/system.hpp
)
