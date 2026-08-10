ares_components(mos6502)

ares_add_sources(
  CORE #
    a52
  UNITY #
    a52.cpp
  PRIMARY #
    system/system.cpp
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
    pokey/pokey.hpp
)
