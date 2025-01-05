ares_components(huc6280 msm5205)

ares_add_sources(
  CORE #
    pce
  UNITY #
    pce.cpp
  PRIMARY #
    cpu/cpu.cpp
    psg/psg.cpp
    pcd/pcd.cpp
    system/system.cpp
    cartridge/cartridge.cpp
    controller/controller.cpp
)

if(ARES_UNITY_CORES)
  ares_add_sources(
    CORE #
      pce
    UNITY #
      vdp/vdp.cpp
      vdp-performance/vdp.cpp
  )
else()
  ares_add_sources(
    CORE #
      pce
    PRIMARY #
      vdp/vdp.cpp
      vdp-performance/vdp.cpp
  )
endif()

ares_add_sources(
  CORE #
    pce
  INCLUDED #
    pce.hpp
    CMakeLists.txt
)

ares_add_sources(
  CORE #
    pce
  INCLUDED #
    cartridge/cartridge.hpp
    cartridge/serialization.cpp
    cartridge/slot.cpp
    cartridge/slot.hpp
    cartridge/board/arcade-card-duo.cpp
    cartridge/board/arcade-card-pro.cpp
    cartridge/board/banked.cpp
    cartridge/board/board.cpp
    cartridge/board/board.hpp
    cartridge/board/debugger.cpp
    cartridge/board/games-express.cpp
    cartridge/board/linear.cpp
    cartridge/board/ram.cpp
    cartridge/board/split.cpp
    cartridge/board/super-system-card.cpp
    cartridge/board/system-card.cpp
)

ares_add_sources(
  CORE #
    pce
  INCLUDED #
    controller/controller.hpp
    controller/port.cpp
    controller/port.hpp
    controller/avenuepad/avenuepad.cpp
    controller/avenuepad/avenuepad.hpp
    controller/gamepad/gamepad.cpp
    controller/gamepad/gamepad.hpp
    controller/multitap/multitap.cpp
    controller/multitap/multitap.hpp
)

ares_add_sources(
  CORE #
    pce
  INCLUDED #
    cpu/cpu.hpp
    cpu/debugger.cpp
    cpu/io.cpp
    cpu/irq.cpp
    cpu/serialization.cpp
)

ares_add_sources(
  CORE #
    pce
  INCLUDED #
    pcd/adpcm.cpp
    pcd/cdda.cpp
    pcd/debugger.cpp
    pcd/drive.cpp
    pcd/fader.cpp
    pcd/io.cpp
    pcd/pcd.hpp
    pcd/scsi.cpp
    pcd/serialization.cpp
)

ares_add_sources(
  CORE #
    pce
  INCLUDED #
    psg/channel.cpp
    psg/io.cpp
    psg/psg.hpp
    psg/serialization.cpp
)

ares_add_sources(
  CORE #
    pce
  INCLUDED #
    system/serialization.cpp
    system/system.hpp
)

ares_add_sources(
  CORE #
    pce
  INCLUDED #
    vdp/background.cpp
    vdp/color.cpp
    vdp/debugger.cpp
    vdp/dma.cpp
    vdp/irq.cpp
    vdp/serialization.cpp
    vdp/sprite.cpp
    vdp/vce.cpp
    vdp/vce.hpp
    vdp/vdc.cpp
    vdp/vdc.hpp
    vdp/vdp.hpp
    vdp/vpc.cpp
    vdp/vpc.hpp
)

ares_add_sources(
  CORE #
    pce
  INCLUDED #
    vdp-performance/background.cpp
    vdp-performance/color.cpp
    vdp-performance/debugger.cpp
    vdp-performance/dma.cpp
    vdp-performance/irq.cpp
    vdp-performance/serialization.cpp
    vdp-performance/sprite.cpp
    vdp-performance/vce.cpp
    vdp-performance/vce.hpp
    vdp-performance/vdc.cpp
    vdp-performance/vdc.hpp
    vdp-performance/vdp.hpp
    vdp-performance/vpc.cpp
    vdp-performance/vpc.hpp
)
