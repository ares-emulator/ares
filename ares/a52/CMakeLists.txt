ares_components(mos6502)

ares_add_sources(
  CORE #
    a52
  UNITY #
    a52.cpp
  PRIMARY #
    system/system.cpp
    controller/controller.cpp
    cartridge/cartridge.cpp
    cpu/cpu.cpp
    antic/antic.cpp
    gtia/gtia.cpp
    pokey/pokey.cpp
)

ares_add_sources(
  CORE #
    a52
  INCLUDED #
    a52.hpp
    CMakeLists.txt
    system/system.hpp
    controller/controller.hpp
    controller/port.cpp
    controller/port.hpp
    controller/standard/standard.cpp
    controller/standard/standard.hpp
    cartridge/cartridge.hpp
    cartridge/slot.cpp
    cartridge/slot.hpp
    cartridge/board/board.cpp
    cartridge/board/board.hpp
    cartridge/board/dual-window.cpp
    cartridge/board/linear.cpp
    cpu/cpu.hpp
    cpu/debugger.cpp
    cpu/memory.cpp
    cpu/timing.cpp
    antic/antic.hpp
    antic/display-list.cpp
    antic/dma.cpp
    antic/interrupt.cpp
    antic/io.cpp
    antic/playfield.cpp
    antic/timing.cpp
    gtia/gtia.hpp
    gtia/collision.cpp
    gtia/color.cpp
    gtia/console.cpp
    gtia/io.cpp
    gtia/player-missile.cpp
    gtia/playfield.cpp
    gtia/priority.cpp
    gtia/timing.cpp
    gtia/video.cpp
    pokey/audio.cpp
    pokey/clock.cpp
    pokey/dac.cpp
    pokey/io.cpp
    pokey/irq.cpp
    pokey/keyboard.cpp
    pokey/output.cpp
    pokey/pokey.hpp
    pokey/pots.cpp
    pokey/serial.cpp
    pokey/status.cpp
)
