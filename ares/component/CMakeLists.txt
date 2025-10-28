list(REMOVE_DUPLICATES ares.components)

set(ARES_COMPONENTS_LIST "${ares.components}" CACHE STRING "Enabled shared hardware components" FORCE)

mark_as_advanced(ARES_COMPONENTS_LIST)

ares_add_sources(INCLUDED CMakeLists.txt)

macro(component_sources)
  set(options "")
  set(oneValueArgs COMPONENT PRIMARY)
  set(multiValueArgs INCLUDED)

  cmake_parse_arguments(CPS "" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

  if(${CPS_COMPONENT} IN_LIST ares.components)
    target_sources(ares PRIVATE ${CPS_PRIMARY})
    foreach(arg IN ITEMS ${CPS_INCLUDED})
      list(APPEND ARES_HEADER_SOURCES "component/${arg}")
      target_sources(ares PRIVATE ${arg})
    endforeach()
  endif()
  set(ARES_HEADER_SOURCES ${ARES_HEADER_SOURCES} PARENT_SCOPE)
endmacro()

component_sources(
  COMPONENT
    ay38910
  PRIMARY
    audio/ay38910/ay38910.cpp
  INCLUDED
    audio/ay38910/ay38910.hpp
    audio/ay38910/serialization.cpp
)

component_sources(
  COMPONENT
    msm5205
  PRIMARY
    audio/msm5205/msm5205.cpp
  INCLUDED
    audio/msm5205/msm5205.hpp
    audio/msm5205/serialization.cpp
)

component_sources(
  COMPONENT
    sn76489
  PRIMARY
    audio/sn76489/sn76489.cpp
  INCLUDED
    audio/sn76489/sn76489.hpp
    audio/sn76489/serialization.cpp
)

component_sources(
  COMPONENT
    t6w28
  PRIMARY
    audio/t6w28/t6w28.cpp
  INCLUDED
    audio/t6w28/t6w28.hpp
    audio/t6w28/serialization.cpp
)

component_sources(
  COMPONENT
    ym2149
  PRIMARY
    audio/ym2149/ym2149.cpp
  INCLUDED
    audio/ym2149/ym2149.hpp
    audio/ym2149/serialization.cpp
)

component_sources(
  COMPONENT
    ym2413
  PRIMARY
    audio/ym2413/ym2413.cpp
  INCLUDED
    audio/ym2413/constants.cpp
    audio/ym2413/io.cpp
    audio/ym2413/operator.cpp
    audio/ym2413/serialization.cpp
    audio/ym2413/voice.cpp
    audio/ym2413/ym2413.hpp
)

component_sources(
  COMPONENT
    ym2612
  PRIMARY
    audio/ym2612/ym2612.cpp
  INCLUDED
    audio/ym2612/channel.cpp
    audio/ym2612/constants.cpp
    audio/ym2612/io.cpp
    audio/ym2612/serialization.cpp
    audio/ym2612/timer.cpp
    audio/ym2612/ym2612.hpp
)

component_sources(
  COMPONENT
    m24c
  PRIMARY
    eeprom/m24c/m24c.cpp
  INCLUDED
    eeprom/m24c/m24c.hpp
    eeprom/m24c/serialization.cpp
)

component_sources(
  COMPONENT
    m93lcx6
  PRIMARY
    eeprom/m93lcx6/m93lcx6.cpp
  INCLUDED
    eeprom/m93lcx6/m93lcx6.hpp
    eeprom/m93lcx6/serialization.cpp
)

component_sources(
  COMPONENT
    i8255
  PRIMARY
    io/i8255/i8255.cpp
  INCLUDED
    io/i8255/i8255.hpp
)

component_sources(
  COMPONENT
    arm7tdmi
  PRIMARY
    processor/arm7tdmi/arm7tdmi.cpp
  INCLUDED
    processor/arm7tdmi/algorithms.cpp
    processor/arm7tdmi/arm7tdmi.hpp
    processor/arm7tdmi/coprocessor.cpp
    processor/arm7tdmi/disassembler.cpp
    processor/arm7tdmi/instruction.cpp
    processor/arm7tdmi/instructions-arm.cpp
    processor/arm7tdmi/instructions-thumb.cpp
    processor/arm7tdmi/memory.cpp
    processor/arm7tdmi/registers.cpp
    processor/arm7tdmi/serialization.cpp
)

component_sources(
  COMPONENT
    gsu
  PRIMARY
    processor/gsu/gsu.cpp
  INCLUDED
    processor/gsu/disassembler.cpp
    processor/gsu/gsu.hpp
    processor/gsu/instruction.cpp
    processor/gsu/instructions.cpp
    processor/gsu/registers.hpp
    processor/gsu/serialization.cpp
)

component_sources(
  COMPONENT
    hg51b
  PRIMARY
    processor/hg51b/hg51b.cpp
  INCLUDED
    processor/hg51b/disassembler.cpp
    processor/hg51b/hg51b.hpp
    processor/hg51b/instruction.cpp
    processor/hg51b/instructions.cpp
    processor/hg51b/registers.cpp
    processor/hg51b/serialization.cpp
)

component_sources(
  COMPONENT
    huc6280
  PRIMARY
    processor/huc6280/huc6280.cpp
  INCLUDED
    processor/huc6280/algorithms.cpp
    processor/huc6280/disassembler.cpp
    processor/huc6280/huc6280.hpp
    processor/huc6280/instruction.cpp
    processor/huc6280/instructions.cpp
    processor/huc6280/memory.cpp
    processor/huc6280/serialization.cpp
)

component_sources(
  COMPONENT
    i8080
  PRIMARY
    processor/i8080/i8080.cpp
  INCLUDED
    processor/i8080/algorithms.cpp
    processor/i8080/disassembler.cpp
    processor/i8080/i8080.hpp
    processor/i8080/instruction.cpp
    processor/i8080/instructions.cpp
    processor/i8080/memory.cpp
    processor/i8080/registers.hpp
    processor/i8080/serialization.cpp
)
component_sources(
  COMPONENT
    m68hc05
  PRIMARY
    processor/m68hc05/m68hc05.cpp
  INCLUDED
    processor/m68hc05/algorithms.cpp
    processor/m68hc05/disassembler.cpp
    processor/m68hc05/instruction.cpp
    processor/m68hc05/instructions.cpp
    processor/m68hc05/m68hc05.hpp
    processor/m68hc05/memory.cpp
    processor/m68hc05/serialization.cpp
)

component_sources(
  COMPONENT
    m68000
  PRIMARY
    processor/m68000/m68000.cpp
  INCLUDED
    processor/m68000/algorithms.cpp
    processor/m68000/conditions.cpp
    processor/m68000/disassembler.cpp
    processor/m68000/effective-address.cpp
    processor/m68000/instruction.cpp
    processor/m68000/instructions.cpp
    processor/m68000/m68000.hpp
    processor/m68000/memory.cpp
    processor/m68000/registers.cpp
    processor/m68000/serialization.cpp
    processor/m68000/traits.cpp
)

component_sources(
  COMPONENT
    mos6502
  PRIMARY
    processor/mos6502/mos6502.cpp
  INCLUDED
    processor/mos6502/addresses.cpp
    processor/mos6502/algorithms.cpp
    processor/mos6502/disassembler.cpp
    processor/mos6502/instruction.cpp
    processor/mos6502/instructions.cpp
    processor/mos6502/memory.cpp
    processor/mos6502/mos6502.hpp
    processor/mos6502/serialization.cpp
)
component_sources(
  COMPONENT
    sh2
  PRIMARY
    processor/sh2/sh2.cpp
  INCLUDED
    processor/sh2/accuracy.hpp
    processor/sh2/decoder.hpp
    processor/sh2/disassembler.cpp
    processor/sh2/exceptions.cpp
    processor/sh2/instruction.cpp
    processor/sh2/instructions.cpp
    processor/sh2/recompiler.cpp
    processor/sh2/serialization.cpp
    processor/sh2/sh2.hpp
    processor/sh2/sh7604/bus.cpp
    processor/sh2/sh7604/cache.cpp
    processor/sh2/sh7604/dma.cpp
    processor/sh2/sh7604/interrupts.cpp
    processor/sh2/sh7604/io.cpp
    processor/sh2/sh7604/serial.cpp
    processor/sh2/sh7604/sh7604.cpp
    processor/sh2/sh7604/sh7604.hpp
    processor/sh2/sh7604/timer.cpp
)

component_sources(
  COMPONENT
    sm5k
  PRIMARY
    processor/sm5k/sm5k.cpp
  INCLUDED
    processor/sm5k/disassembler.cpp
    processor/sm5k/instruction.cpp
    processor/sm5k/instructions.cpp
    processor/sm5k/memory.cpp
    processor/sm5k/serialization.cpp
    processor/sm5k/sm5k.hpp
    processor/sm5k/timer.cpp
)

component_sources(
  COMPONENT
    sm83
  PRIMARY
    processor/sm83/sm83.cpp
  INCLUDED
    processor/sm83/algorithms.cpp
    processor/sm83/disassembler.cpp
    processor/sm83/instruction.cpp
    processor/sm83/instructions.cpp
    processor/sm83/memory.cpp
    processor/sm83/registers.hpp
    processor/sm83/serialization.cpp
    processor/sm83/sm83.hpp
)

component_sources(
  COMPONENT
    spc700
  PRIMARY
    processor/spc700/spc700.cpp
  INCLUDED
    processor/spc700/algorithms.cpp
    processor/spc700/disassembler.cpp
    processor/spc700/instruction.cpp
    processor/spc700/instructions.cpp
    processor/spc700/memory.cpp
    processor/spc700/serialization.cpp
    processor/spc700/spc700.hpp
)

component_sources(
  COMPONENT
    ssp1601
  PRIMARY
    processor/ssp1601/ssp1601.cpp
  INCLUDED
    processor/ssp1601/algorithms.cpp
    processor/ssp1601/disassembler.cpp
    processor/ssp1601/instructions.cpp
    processor/ssp1601/memory.cpp
    processor/ssp1601/registers.cpp
    processor/ssp1601/serialization.cpp
    processor/ssp1601/ssp1601.hpp
)

component_sources(
  COMPONENT
    tlcs900h
  PRIMARY
    processor/tlcs900h/tlcs900h.cpp
  INCLUDED
    processor/tlcs900h/algorithms.cpp
    processor/tlcs900h/conditions.cpp
    processor/tlcs900h/control-registers.cpp
    processor/tlcs900h/disassembler.cpp
    processor/tlcs900h/dma.cpp
    processor/tlcs900h/instruction.cpp
    processor/tlcs900h/instructions.cpp
    processor/tlcs900h/memory.cpp
    processor/tlcs900h/prefetch.cpp
    processor/tlcs900h/registers.cpp
    processor/tlcs900h/serialization.cpp
    processor/tlcs900h/tlcs900h.hpp
)

component_sources(
  COMPONENT
    upd96050
  PRIMARY
    processor/upd96050/upd96050.cpp
  INCLUDED
    processor/upd96050/disassembler.cpp
    processor/upd96050/instructions.cpp
    processor/upd96050/memory.cpp
    processor/upd96050/serialization.cpp
    processor/upd96050/upd96050.hpp
)

component_sources(
  COMPONENT
    v30mz
  PRIMARY
    processor/v30mz/v30mz.cpp
  INCLUDED
    processor/v30mz/algorithms.cpp
    processor/v30mz/disassembler.cpp
    processor/v30mz/instruction.cpp
    processor/v30mz/instructions-adjust.cpp
    processor/v30mz/instructions-alu.cpp
    processor/v30mz/instructions-exec.cpp
    processor/v30mz/instructions-flag.cpp
    processor/v30mz/instructions-group.cpp
    processor/v30mz/instructions-misc.cpp
    processor/v30mz/instructions-move.cpp
    processor/v30mz/instructions-string.cpp
    processor/v30mz/memory.cpp
    processor/v30mz/modrm.cpp
    processor/v30mz/prefetch.cpp
    processor/v30mz/registers.cpp
    processor/v30mz/serialization.cpp
    processor/v30mz/v30mz.hpp
)

component_sources(
  COMPONENT
    wdc65816
  PRIMARY
    processor/wdc65816/wdc65816.cpp
  INCLUDED
    processor/wdc65816/algorithms.cpp
    processor/wdc65816/disassembler.cpp
    processor/wdc65816/instruction.cpp
    processor/wdc65816/instruction.hpp
    processor/wdc65816/instructions-modify.cpp
    processor/wdc65816/instructions-other.cpp
    processor/wdc65816/instructions-pc.cpp
    processor/wdc65816/instructions-read.cpp
    processor/wdc65816/instructions-write.cpp
    processor/wdc65816/memory.cpp
    processor/wdc65816/registers.hpp
    processor/wdc65816/serialization.cpp
    processor/wdc65816/wdc65816.hpp
)

component_sources(
  COMPONENT
    z80
  PRIMARY
    processor/z80/z80.cpp
  INCLUDED
    processor/z80/algorithms.cpp
    processor/z80/disassembler.cpp
    processor/z80/instruction.cpp
    processor/z80/instructions.cpp
    processor/z80/memory.cpp
    processor/z80/registers.hpp
    processor/z80/serialization.cpp
    processor/z80/z80.hpp
)

component_sources(
  COMPONENT
    s3511a
  PRIMARY
    rtc/s3511a/s3511a.cpp
  INCLUDED
    rtc/s3511a/s3511a.hpp
    rtc/s3511a/serialization.cpp
)

component_sources(
  COMPONENT
    sst39sf0x0
  PRIMARY
    flash/sst39sf0x0/sst39sf0x0.cpp
  INCLUDED
    flash/sst39sf0x0/sst39sf0x0.hpp
)

component_sources(
  COMPONENT
    tms9918
  PRIMARY
    video/tms9918/tms9918.cpp
  INCLUDED
    video/tms9918/background.cpp
    video/tms9918/dac.cpp
    video/tms9918/io.cpp
    video/tms9918/serialization.cpp
    video/tms9918/sprite.cpp
    video/tms9918/tms9918.hpp
)

component_sources(
  COMPONENT
    v9938
  PRIMARY
    video/v9938/v9938.cpp
  INCLUDED
    video/v9938/background.cpp
    video/v9938/commands.cpp
    video/v9938/dac.cpp
    video/v9938/io.cpp
    video/v9938/serialization.cpp
    video/v9938/sprite.cpp
    video/v9938/v9938.hpp
)
