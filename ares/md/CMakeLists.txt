ares_components(
  m68000
  z80
  sh2
  ssp1601
  sn76489
  ym2612
  m24c
)

target_link_libraries(ares PRIVATE qon)

ares_add_sources(
  CORE #
    md
  UNITY #
    md.cpp
  PRIMARY #
    bus/bus.cpp
    cpu/cpu.cpp
    apu/apu.cpp
    vdp/vdp.cpp
    opn2/opn2.cpp
    m32x/m32x.cpp
    mcd/mcd.cpp
    system/system.cpp
    cartridge/cartridge.cpp
    controller/controller.cpp
)

ares_add_sources(
  CORE #
    md
  INCLUDED #
    md.hpp
    CMakeLists.txt
)

ares_add_sources(
  CORE #
    md
  INCLUDED #
    apu/apu.hpp
    apu/bus.cpp
    apu/debugger.cpp
    apu/serialization.cpp
)

ares_add_sources(
  CORE #
    md
  INCLUDED #
    bus/bus.hpp
    bus/inline.hpp
    bus/serialization.cpp
)

ares_add_sources(
  CORE #
    md
  INCLUDED #
    cartridge/cartridge.hpp
    cartridge/serialization.cpp
    cartridge/slot.cpp
    cartridge/slot.hpp
)

ares_add_sources(
  CORE #
    md
  INCLUDED #
    cartridge/board/banked.cpp
    cartridge/board/board.cpp
    cartridge/board/board.hpp
    cartridge/board/debugger.cpp
    cartridge/board/game-genie.cpp
    cartridge/board/j-cart.cpp
    cartridge/board/linear.cpp
    cartridge/board/mega-32x.cpp
    cartridge/board/realtec.cpp
    cartridge/board/standard.cpp
    cartridge/board/svp.cpp
)

ares_add_sources(
  CORE #
    md
  INCLUDED #
    controller/controller.hpp
    controller/port.cpp
    controller/port.hpp
)

ares_add_sources(
  CORE #
    md
  INCLUDED #
    controller/control-pad/control-pad.cpp
    controller/control-pad/control-pad.hpp
)

ares_add_sources(
  CORE #
    md
  INCLUDED #
    controller/fighting-pad/fighting-pad.cpp
    controller/fighting-pad/fighting-pad.hpp
)

ares_add_sources(
  CORE #
    md
  INCLUDED #
    cpu/bus.cpp
    cpu/cpu.hpp
    cpu/debugger.cpp
    cpu/io.cpp
    cpu/serialization.cpp
)

ares_add_sources(
  CORE #
    md
  INCLUDED #
    m32x/bus-external.cpp
    m32x/bus-internal.cpp
    m32x/debugger.cpp
    m32x/io-external.cpp
    m32x/io-internal.cpp
    m32x/m32x.hpp
    m32x/pwm.cpp
    m32x/serialization.cpp
    m32x/sh7604.cpp
    m32x/vdp.cpp
)

ares_add_sources(
  CORE #
    md
  INCLUDED #
    mcd/bus-external.cpp
    mcd/bus-internal.cpp
    mcd/cdc-transfer.cpp
    mcd/cdc.cpp
    mcd/cdd-dac.cpp
    mcd/cdd.cpp
    mcd/megald.cpp
    mcd/debugger.cpp
    mcd/gpu.cpp
    mcd/io-external.cpp
    mcd/io-internal.cpp
    mcd/irq.cpp
    mcd/mcd.hpp
    mcd/pcm.cpp
    mcd/serialization.cpp
    mcd/timer.cpp
)

ares_add_sources(
  CORE #
    md
  INCLUDED #
    opn2/opn2.hpp
    opn2/serialization.cpp
)

ares_add_sources(
  CORE #
    md
  INCLUDED #
    system/controls.cpp
    system/serialization.cpp
    system/system.hpp
)

ares_add_sources(
  CORE #
    md
  INCLUDED #
    vdp/color.cpp
    vdp/dac.cpp
    vdp/debugger.cpp
    vdp/dma.cpp
    vdp/fifo.cpp
    vdp/io.cpp
    vdp/irq.cpp
    vdp/layer.cpp
    vdp/layers.cpp
    vdp/main.cpp
    vdp/memory.cpp
    vdp/prefetch.cpp
    vdp/psg.cpp
    vdp/serialization.cpp
    vdp/sprite.cpp
    vdp/vdp.hpp
    vdp/window.cpp
)

ares_add_sources(
  CORE #
    md
  INCLUDED #
    vdp-performance/background.cpp
    vdp-performance/color.cpp
    vdp-performance/debugger.cpp
    vdp-performance/dma.cpp
    vdp-performance/io.cpp
    vdp-performance/irq.cpp
    vdp-performance/main.cpp
    vdp-performance/memory.cpp
    vdp-performance/object.cpp
    vdp-performance/psg.cpp
    vdp-performance/render.cpp
    vdp-performance/serialization.cpp
    vdp-performance/sprite.cpp
    vdp-performance/vdp.cpp
    vdp-performance/vdp.hpp
)
