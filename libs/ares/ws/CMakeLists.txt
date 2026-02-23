ares_components(v30mz m93lcx6)

ares_add_sources(
  CORE #
    ws
  UNITY #
    ws.cpp
  PRIMARY #
    system/system.cpp
    memory/memory.cpp
    eeprom/eeprom.cpp
    cartridge/cartridge.cpp
    cpu/cpu.cpp
    ppu/ppu.cpp
    apu/apu.cpp
    serial/serial.cpp
)

ares_add_sources(
  CORE #
    ws
  INCLUDED #
    ws.hpp
    CMakeLists.txt
)

ares_add_sources(
  CORE #
    ws
  INCLUDED #
    apu/apu.hpp
    apu/channel1.cpp
    apu/channel2.cpp
    apu/channel3.cpp
    apu/channel4.cpp
    apu/channel5.cpp
    apu/debugger.cpp
    apu/dma.cpp
    apu/io.cpp
    apu/serialization.cpp
)

ares_add_sources(
  CORE #
    ws
  INCLUDED #
    cartridge/cartridge.hpp
    cartridge/debugger.cpp
    cartridge/flash.cpp
    cartridge/io.cpp
    cartridge/karnak.cpp
    cartridge/memory.cpp
    cartridge/rtc.cpp
    cartridge/serialization.cpp
    cartridge/slot.cpp
    cartridge/slot.hpp
)

ares_add_sources(
  CORE #
    ws
  INCLUDED #
    cpu/cpu.hpp
    cpu/debugger.cpp
    cpu/dma.cpp
    cpu/interrupt.cpp
    cpu/io.cpp
    cpu/keypad.cpp
    cpu/serialization.cpp
)

ares_add_sources(
  CORE #
    ws
  INCLUDED #
    eeprom/eeprom.hpp
    eeprom/serialization.cpp
)

ares_add_sources(
  CORE #
    ws
  INCLUDED #
    memory/memory.hpp
)

ares_add_sources(
  CORE #
    ws
  INCLUDED #
    ppu/color.cpp
    ppu/dac.cpp
    ppu/debugger.cpp
    ppu/io.cpp
    ppu/memory.cpp
    ppu/ppu.hpp
    ppu/screen.cpp
    ppu/serialization.cpp
    ppu/sprite.cpp
    ppu/timer.cpp
    ppu/window.cpp
)

ares_add_sources(
  CORE #
    ws
  INCLUDED #
    serial/debugger.cpp
    serial/serial.hpp
    serial/serialization.cpp
)

ares_add_sources(
  CORE #
    ws
  INCLUDED #
    system/controls.cpp
    system/debugger.cpp
    system/io.cpp
    system/serialization.cpp
    system/system.hpp
)
