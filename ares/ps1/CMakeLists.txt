ares_components(m68hc05)

ares_add_sources(
  CORE #
    ps1
  UNITY #
    ps1.cpp
  PRIMARY #
    memory/memory.cpp
    system/system.cpp
    disc/disc.cpp
    cpu/cpu.cpp
    gpu/gpu.cpp
    spu/spu.cpp
    mdec/mdec.cpp
    interrupt/interrupt.cpp
    peripheral/peripheral.cpp
    dma/dma.cpp
    timer/timer.cpp
)

ares_add_sources(
  CORE #
    ps1
  INCLUDED #
    ps1.hpp
    CMakeLists.txt
)

ares_add_sources(
  CORE #
    ps1
  INCLUDED #
    accuracy.hpp
)

ares_add_sources(
  CORE #
    ps1
  INCLUDED #
    cpu/breakpoints.cpp
    cpu/cpu.hpp
    cpu/debugger.cpp
    cpu/delay-slots.cpp
    cpu/disassembler.cpp
    cpu/exceptions.cpp
    cpu/icache.cpp
    cpu/interpreter-gte.cpp
    cpu/interpreter-ipu.cpp
    cpu/interpreter-scc.cpp
    cpu/interpreter.cpp
    cpu/memory.cpp
    cpu/serialization.cpp
)

ares_add_sources(
  CORE #
    ps1
  INCLUDED #
    disc/cdda.cpp
    disc/cdxa.cpp
    disc/command.cpp
    disc/debugger.cpp
    disc/disc.hpp
    disc/drive.cpp
    disc/io.cpp
    disc/irq.cpp
    disc/serialization.cpp
)

ares_add_sources(
  CORE #
    ps1
  INCLUDED #
    dma/channel.cpp
    dma/debugger.cpp
    dma/dma.hpp
    dma/io.cpp
    dma/irq.cpp
    dma/serialization.cpp
)

ares_add_sources(
  CORE #
    ps1
  INCLUDED #
    gpu/blitter.cpp
    gpu/debugger.cpp
    gpu/gp0.cpp
    gpu/gp1.cpp
    gpu/gpu.hpp
    gpu/io.cpp
    gpu/renderer.cpp
    gpu/serialization.cpp
)

ares_add_sources(
  CORE #
    ps1
  INCLUDED #
    interrupt/interrupt.hpp
    interrupt/io.cpp
    interrupt/serialization.cpp
)

ares_add_sources(
  CORE #
    ps1
  INCLUDED #
    mdec/decoder.cpp
    mdec/io.cpp
    mdec/mdec.hpp
    mdec/serialization.cpp
    mdec/tables.cpp
)

ares_add_sources(
  CORE #
    ps1
  INCLUDED #
    memory/bus.hpp
    memory/interface.hpp
    memory/io.cpp
    memory/memory.hpp
    memory/readable.hpp
    memory/serialization.cpp
    memory/unmapped.hpp
    memory/writable.hpp
)

ares_add_sources(
  CORE #
    ps1
  INCLUDED #
    peripheral/io.cpp
    peripheral/peripheral.hpp
    peripheral/port.cpp
    peripheral/port.hpp
    peripheral/serialization.cpp
    peripheral/digital-gamepad/digital-gamepad.cpp
    peripheral/digital-gamepad/digital-gamepad.hpp
    peripheral/dualshock/dualshock.cpp
    peripheral/dualshock/dualshock.hpp
    peripheral/memory-card/memory-card.cpp
    peripheral/memory-card/memory-card.hpp
)

ares_add_sources(
  CORE #
    ps1
  INCLUDED #
    spu/adpcm.cpp
    spu/adsr.cpp
    spu/capture.cpp
    spu/debugger.cpp
    spu/envelope.cpp
    spu/fifo.cpp
    spu/gaussian.cpp
    spu/io.cpp
    spu/noise.cpp
    spu/reverb.cpp
    spu/serialization.cpp
    spu/spu.hpp
    spu/voice.cpp
)

ares_add_sources(
  CORE #
    ps1
  INCLUDED #
    system/serialization.cpp
    system/system.hpp
)

ares_add_sources(
  CORE #
    ps1
  INCLUDED #
    timer/io.cpp
    timer/serialization.cpp
    timer/timer.hpp
)
