ares_components(
  wdc65816
  spc700
  arm7tdmi
  gsu
  hg51b
  upd96050
)

ares_add_sources(
  CORE #
    sfc
  UNITY #
    sfc.cpp
  PRIMARY #
    system/system.cpp
    controller/controller.cpp
    cartridge/cartridge.cpp
    memory/memory.cpp
    cpu/cpu.cpp
    smp/smp.cpp
    dsp/dsp.cpp
    ppu/ppu.cpp
    ppu-performance/ppu.cpp
    coprocessor/coprocessor.cpp
    expansion/expansion.cpp
    slot/slot.cpp
)

ares_add_sources(
  CORE #
    sfc
  INCLUDED #
    sfc.hpp
    CMakeLists.txt
)

ares_add_sources(
  CORE #
    sfc
  INCLUDED #
    cartridge/cartridge.hpp
    cartridge/debugger.cpp
    cartridge/load.cpp
    cartridge/save.cpp
    cartridge/serialization.cpp
    cartridge/slot.cpp
    cartridge/slot.hpp
)

ares_add_sources(
  CORE #
    sfc
  INCLUDED #
    controller/controller.hpp
    controller/port.cpp
    controller/port.hpp
    controller/gamepad/gamepad.cpp
    controller/gamepad/gamepad.hpp
    controller/justifier/justifier.cpp
    controller/justifier/justifier.hpp
    controller/justifiers/justifiers.cpp
    controller/justifiers/justifiers.hpp
    controller/mouse/mouse.cpp
    controller/mouse/mouse.hpp
    controller/rumble-gamepad/rumble-gamepad.cpp
    controller/rumble-gamepad/rumble-gamepad.hpp
    controller/ntt-data-keypad/ntt-data-keypad.cpp
    controller/ntt-data-keypad/ntt-data-keypad.hpp
    controller/super-multitap/super-multitap.cpp
    controller/super-multitap/super-multitap.hpp
    controller/super-scope/super-scope.cpp
    controller/super-scope/super-scope.hpp
    controller/twin-tap/twin-tap.cpp
    controller/twin-tap/twin-tap.hpp
)

ares_add_sources(
  CORE #
    sfc
  INCLUDED #
    coprocessor/coprocessor.hpp
    coprocessor/armdsp/armdsp.cpp
    coprocessor/armdsp/armdsp.hpp
    coprocessor/armdsp/debugger.cpp
    coprocessor/armdsp/io.cpp
    coprocessor/armdsp/memory.cpp
    coprocessor/armdsp/serialization.cpp
    coprocessor/competition/competition.cpp
    coprocessor/competition/competition.hpp
    coprocessor/competition/serialization.cpp
    coprocessor/dip/dip.cpp
    coprocessor/dip/dip.hpp
    coprocessor/dip/serialization.cpp
    coprocessor/epsonrtc/epsonrtc.cpp
    coprocessor/epsonrtc/epsonrtc.hpp
    coprocessor/epsonrtc/memory.cpp
    coprocessor/epsonrtc/serialization.cpp
    coprocessor/epsonrtc/time.cpp
    coprocessor/hitachidsp/debugger.cpp
    coprocessor/hitachidsp/hitachidsp.cpp
    coprocessor/hitachidsp/hitachidsp.hpp
    coprocessor/hitachidsp/memory.cpp
    coprocessor/hitachidsp/serialization.cpp
    coprocessor/icd/icd.cpp
    coprocessor/icd/icd.hpp
    coprocessor/icd/interface.cpp
    coprocessor/icd/io.cpp
    coprocessor/icd/serialization.cpp
    coprocessor/mcc/mcc.cpp
    coprocessor/mcc/mcc.hpp
    coprocessor/mcc/serialization.cpp
    coprocessor/msu1/msu1.cpp
    coprocessor/msu1/msu1.hpp
    coprocessor/msu1/serialization.cpp
    coprocessor/necdsp/debugger.cpp
    coprocessor/necdsp/memory.cpp
    coprocessor/necdsp/necdsp.cpp
    coprocessor/necdsp/necdsp.hpp
    coprocessor/necdsp/serialization.cpp
    coprocessor/obc1/obc1.cpp
    coprocessor/obc1/obc1.hpp
    coprocessor/obc1/serialization.cpp
    coprocessor/sa1/bwram.cpp
    coprocessor/sa1/debugger.cpp
    coprocessor/sa1/dma.cpp
    coprocessor/sa1/io.cpp
    coprocessor/sa1/iram.cpp
    coprocessor/sa1/memory.cpp
    coprocessor/sa1/rom.cpp
    coprocessor/sa1/sa1.cpp
    coprocessor/sa1/sa1.hpp
    coprocessor/sa1/serialization.cpp
    coprocessor/sdd1/decompressor.cpp
    coprocessor/sdd1/decompressor.hpp
    coprocessor/sdd1/sdd1.cpp
    coprocessor/sdd1/sdd1.hpp
    coprocessor/sdd1/serialization.cpp
    coprocessor/sharprtc/memory.cpp
    coprocessor/sharprtc/serialization.cpp
    coprocessor/sharprtc/sharprtc.cpp
    coprocessor/sharprtc/sharprtc.hpp
    coprocessor/sharprtc/time.cpp
    coprocessor/spc7110/alu.cpp
    coprocessor/spc7110/data.cpp
    coprocessor/spc7110/dcu.cpp
    coprocessor/spc7110/decompressor.cpp
    coprocessor/spc7110/serialization.cpp
    coprocessor/spc7110/spc7110.cpp
    coprocessor/spc7110/spc7110.hpp
    coprocessor/superfx/bus.cpp
    coprocessor/superfx/core.cpp
    coprocessor/superfx/debugger.cpp
    coprocessor/superfx/io.cpp
    coprocessor/superfx/memory.cpp
    coprocessor/superfx/serialization.cpp
    coprocessor/superfx/superfx.cpp
    coprocessor/superfx/superfx.hpp
    coprocessor/superfx/timing.cpp
)

ares_add_sources(
  CORE #
    sfc
  INCLUDED #
    cpu/cpu.hpp
    cpu/debugger.cpp
    cpu/dma.cpp
    cpu/io.cpp
    cpu/irq.cpp
    cpu/memory.cpp
    cpu/serialization.cpp
    cpu/timing.cpp
)

ares_add_sources(
  CORE #
    sfc
  INCLUDED #
    dsp/brr.cpp
    dsp/counter.cpp
    dsp/debugger.cpp
    dsp/dsp.hpp
    dsp/echo.cpp
    dsp/envelope.cpp
    dsp/gaussian.cpp
    dsp/memory.cpp
    dsp/misc.cpp
    dsp/serialization.cpp
    dsp/voice.cpp
)

ares_add_sources(
  CORE #
    sfc
  INCLUDED #
    expansion/expansion.hpp
    expansion/port.cpp
    expansion/port.hpp
    expansion/21fx/21fx.cpp
    expansion/21fx/21fx.hpp
    expansion/satellaview/satellaview.cpp
    expansion/satellaview/satellaview.hpp
)

ares_add_sources(
  CORE #
    sfc
  INCLUDED #
    memory/inline.hpp
    memory/memory.hpp
    memory/protectable.hpp
    memory/readable.hpp
    memory/writable.hpp
)

ares_add_sources(
  CORE #
    sfc
  INCLUDED #
    ppu/background.cpp
    ppu/color.cpp
    ppu/dac.cpp
    ppu/debugger.cpp
    ppu/io.cpp
    ppu/main.cpp
    ppu/mode7.cpp
    ppu/mosaic.cpp
    ppu/oam.cpp
    ppu/object.cpp
    ppu/ppu.hpp
    ppu/serialization.cpp
    ppu/window.cpp
    ppu/counter/counter.hpp
    ppu/counter/inline.hpp
    ppu/counter/serialization.cpp
)

ares_add_sources(
  CORE #
    sfc
  INCLUDED #
    ppu-performance/background.cpp
    ppu-performance/color.cpp
    ppu-performance/dac.cpp
    ppu-performance/debugger.cpp
    ppu-performance/io.cpp
    ppu-performance/mode7.cpp
    ppu-performance/mosaic.cpp
    ppu-performance/oam.cpp
    ppu-performance/object.cpp
    ppu-performance/ppu.hpp
    ppu-performance/serialization.cpp
    ppu-performance/window.cpp
)

ares_add_sources(
  CORE #
    sfc
  INCLUDED #
    slot/slot.hpp
    slot/bsmemory/bsmemory.cpp
    slot/bsmemory/bsmemory.hpp
    slot/bsmemory/serialization.cpp
    slot/bsmemory/slot.cpp
    slot/bsmemory/slot.hpp
    slot/sufamiturbo/memory.cpp
    slot/sufamiturbo/serialization.cpp
    slot/sufamiturbo/slot.cpp
    slot/sufamiturbo/slot.hpp
    slot/sufamiturbo/sufamiturbo.cpp
    slot/sufamiturbo/sufamiturbo.hpp
)

ares_add_sources(
  CORE #
    sfc
  INCLUDED #
    smp/debugger.cpp
    smp/io.cpp
    smp/memory.cpp
    smp/serialization.cpp
    smp/smp.hpp
    smp/timing.cpp
)

ares_add_sources(
  CORE #
    sfc
  INCLUDED #
    system/controls.cpp
    system/serialization.cpp
    system/system.hpp
)
