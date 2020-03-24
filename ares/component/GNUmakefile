components := $(call unique,$(components))

objects += $(if $(findstring ay38910,$(components)),audio-ay38910)
objects += $(if $(findstring sn76489,$(components)),audio-sn76489)
objects += $(if $(findstring t6w28,$(components)),audio-t6w28)
objects += $(if $(findstring ym2149,$(components)),audio-ym2149)
objects += $(if $(findstring ym2413,$(components)),audio-ym2413)

objects += $(if $(findstring m93lcx6,$(components)),eeprom-m93lcx6)
objects += $(if $(findstring x24c01,$(components)),eeprom-x24c01)

objects += $(if $(findstring arm7tdmi,$(components)),processor-arm7tdmi)
objects += $(if $(findstring gsu,$(components)),processor-gsu)
objects += $(if $(findstring hg51b,$(components)),processor-hg51b)
objects += $(if $(findstring huc6280,$(components)),processor-huc6280)
objects += $(if $(findstring m68k,$(components)),processor-m68k)
objects += $(if $(findstring mos6502,$(components)),processor-mos6502)
objects += $(if $(findstring sm83,$(components)),processor-sm83)
objects += $(if $(findstring spc700,$(components)),processor-spc700)
objects += $(if $(findstring tlcs900h,$(components)),processor-tlcs900h)
objects += $(if $(findstring upd96050,$(components)),processor-upd96050)
objects += $(if $(findstring v30mz,$(components)),processor-v30mz)
objects += $(if $(findstring wdc65816,$(components)),processor-wdc65816)
objects += $(if $(findstring z80,$(components)),processor-z80)

objects += $(if $(findstring tms9918,$(components)),video-tms9918)
objects += $(if $(findstring v9938,$(components)),video-v9938)

obj/audio-ay38910.o:      component/audio/ay38910/ay38910.cpp
obj/audio-sn76489.o:      component/audio/sn76489/sn76489.cpp
obj/audio-t6w28.o:        component/audio/t6w28/t6w28.cpp
obj/audio-ym2149.o:       component/audio/ym2149/ym2149.cpp
obj/audio-ym2413.o:       component/audio/ym2413/ym2413.cpp

obj/eeprom-m93lcx6.o:     component/eeprom/m93lcx6/m93lcx6.cpp
obj/eeprom-x24c01.o:      component/eeprom/x24c01/x24c01.cpp

obj/processor-arm7tdmi.o: component/processor/arm7tdmi/arm7tdmi.cpp
obj/processor-gsu.o:      component/processor/gsu/gsu.cpp
obj/processor-hg51b.o:    component/processor/hg51b/hg51b.cpp
obj/processor-huc6280.o:  component/processor/huc6280/huc6280.cpp
obj/processor-m68k.o:     component/processor/m68k/m68k.cpp
obj/processor-mos6502.o:  component/processor/mos6502/mos6502.cpp
obj/processor-sm83.o:     component/processor/sm83/sm83.cpp
obj/processor-spc700.o:   component/processor/spc700/spc700.cpp
obj/processor-tlcs900h.o: component/processor/tlcs900h/tlcs900h.cpp
obj/processor-upd96050.o: component/processor/upd96050/upd96050.cpp
obj/processor-v30mz.o:    component/processor/v30mz/v30mz.cpp
obj/processor-wdc65816.o: component/processor/wdc65816/wdc65816.cpp
obj/processor-z80.o:      component/processor/z80/z80.cpp

obj/video-tms9918.o:      component/video/tms9918/tms9918.cpp
obj/video-v9938.o:        component/video/v9938/v9938.cpp
