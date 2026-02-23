ares_components(z80 sn76489 ym2413)

ares_add_sources(
  CORE #
    ms
  UNITY #
    ms.cpp
  PRIMARY #
    cpu/cpu.cpp
    vdp/vdp.cpp
    psg/psg.cpp
    opll/opll.cpp
    system/system.cpp
    cartridge/cartridge.cpp
    controller/controller.cpp
    expansion/expansion.cpp
)

ares_add_sources(
  CORE #
    ms
  INCLUDED #
    ms.hpp
    CMakeLists.txt
)

ares_add_sources(
  CORE #
    ms
  INCLUDED #
    cartridge/cartridge.hpp
    cartridge/serialization.cpp
    cartridge/slot.cpp
    cartridge/slot.hpp
)

ares_add_sources(
  CORE #
    ms
  INCLUDED #
    cartridge/board/board.cpp
    cartridge/board/board.hpp
    cartridge/board/codemasters.cpp
    cartridge/board/hap2000.cpp
    cartridge/board/hicom.cpp
    cartridge/board/janggun.cpp
    cartridge/board/k188in1.cpp
    cartridge/board/korea.cpp
    cartridge/board/pak4.cpp
    cartridge/board/sega.cpp
    cartridge/board/zemina.cpp
)

ares_add_sources(
  CORE #
    ms
  INCLUDED #
    controller/controller.hpp
    controller/port.cpp
    controller/port.hpp
)

ares_add_sources(
  CORE #
    ms
  INCLUDED #
    controller/gamepad/gamepad.cpp
    controller/gamepad/gamepad.hpp
)

ares_add_sources(
  CORE #
    ms
  INCLUDED #
    controller/light-phaser/light-phaser.cpp
    controller/light-phaser/light-phaser.hpp
)

ares_add_sources(
  CORE #
    ms
  INCLUDED #
    controller/md-control-pad/md-control-pad.cpp
    controller/md-control-pad/md-control-pad.hpp
)

ares_add_sources(
  CORE #
    ms
  INCLUDED #
    controller/md-fighting-pad/md-fighting-pad.cpp
    controller/md-fighting-pad/md-fighting-pad.hpp
)

ares_add_sources(
  CORE #
    ms
  INCLUDED #
    controller/mega-mouse/mega-mouse.cpp
    controller/mega-mouse/mega-mouse.hpp
)

ares_add_sources(
  CORE #
    ms
  INCLUDED #
    controller/paddle/paddle.cpp
    controller/paddle/paddle.hpp
)

ares_add_sources(
  CORE #
    ms
  INCLUDED #
    controller/sports-pad/sports-pad.cpp
    controller/sports-pad/sports-pad.hpp
)

ares_add_sources(
  CORE #
    ms
  INCLUDED #
    cpu/cpu.hpp
    cpu/debugger.cpp
    cpu/memory.cpp
    cpu/serialization.cpp
)

ares_add_sources(
  CORE #
    ms
  INCLUDED #
    expansion/expansion.hpp
    expansion/port.cpp
    expansion/port.hpp
)

ares_add_sources(
  CORE #
    ms
  INCLUDED #
    expansion/fmsound/fmsound.cpp
    expansion/fmsound/fmsound.hpp
)

ares_add_sources(
  CORE #
    ms
  INCLUDED #
    opll/opll.hpp
    opll/serialization.cpp
)

ares_add_sources(
  CORE #
    ms
  INCLUDED #
    psg/psg.hpp
    psg/serialization.cpp
)

ares_add_sources(
  CORE #
    ms
  INCLUDED #
    system/bios.cpp
    system/controls.cpp
    system/serialization.cpp
    system/system.hpp
)

ares_add_sources(
  CORE #
    ms
  INCLUDED #
    vdp/background.cpp
    vdp/color.cpp
    vdp/dac.cpp
    vdp/debugger.cpp
    vdp/io.cpp
    vdp/irq.cpp
    vdp/serialization.cpp
    vdp/sprite.cpp
    vdp/vdp.hpp
)
