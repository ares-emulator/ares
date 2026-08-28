ares_components(arm7tdmi mos6502)

ares_add_sources(
  CORE #
    a26
  UNITY #
    a26.cpp
  PRIMARY #
    system/system.cpp
    controller/controller.cpp
    cartridge/cartridge.cpp
    cpu/cpu.cpp
    harmony/harmony.cpp
    video/video.cpp
    tia/tia.cpp
    riot/riot.cpp
)

ares_add_sources(
  CORE #
    a26
  INCLUDED #
    a26.hpp
    CMakeLists.txt
)

ares_add_sources(
  CORE #
    a26
  INCLUDED #
    cartridge/cartridge.hpp
    cartridge/serialization.cpp
    cartridge/slot.cpp
    cartridge/slot.hpp
    cartridge/board/atari16k.cpp
    cartridge/board/atari32k.cpp
    cartridge/board/atari8k.cpp
    cartridge/board/board.cpp
    cartridge/board/board.hpp
    cartridge/board/commavid.cpp
    cartridge/board/cdf.cpp
    cartridge/board/dpc-plus.cpp
    cartridge/board/linear.cpp
    cartridge/board/parker-bros.cpp
    cartridge/board/tigervision.cpp
)

ares_add_sources(
  CORE #
    a26
  INCLUDED #
    harmony/harmony.hpp
    harmony/memory.cpp
    harmony/serialization.cpp
    harmony/timing.cpp
)

ares_add_sources(
  CORE #
    a26
  INCLUDED #
    controller/controller.hpp
    controller/port.cpp
    controller/port.hpp
    controller/gamepad/gamepad.cpp
    controller/gamepad/gamepad.hpp
)

ares_add_sources(
  CORE #
    a26
  INCLUDED #
    cpu/cpu.hpp
    cpu/debugger.cpp
    cpu/memory.cpp
    cpu/serialization.cpp
    cpu/timing.cpp
)

ares_add_sources(
  CORE #
    a26
  INCLUDED #
    riot/io.cpp
    riot/riot.hpp
    riot/serialization.cpp
)

ares_add_sources(
  CORE #
    a26
  INCLUDED #
    system/controls.cpp
    system/serialization.cpp
    system/system.hpp
)

ares_add_sources(
  CORE #
    a26
  INCLUDED #
    video/color.cpp
    video/receiver.cpp
    video/serialization.cpp
    video/video.hpp
)

ares_add_sources(
  CORE #
    a26
  INCLUDED #
    tia/audio.cpp
    tia/analog.cpp
    tia/ball.cpp
    tia/collision.cpp
    tia/io.cpp
    tia/missile.cpp
    tia/objects.cpp
    tia/player.cpp
    tia/playfield.cpp
    tia/priority.cpp
    tia/serialization.cpp
    tia/timing.cpp
    tia/tia.hpp
    tia/trigger.cpp
    tia/write-queue.cpp
)
