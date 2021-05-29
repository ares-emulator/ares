ares.components += mos6502
ares.components += ym2149
ares.components += ym2413
ares.components += m24c

ares.objects += ares-fc-system
ares.objects += ares-fc-controller
ares.objects += ares-fc-expansion
ares.objects += ares-fc-cartridge
ares.objects += ares-fc-cpu
ares.objects += ares-fc-apu
ares.objects += ares-fc-ppu
ares.objects += ares-fc-fds

$(object.path)/ares-fc-system.o:     $(ares.path)/fc/system/system.cpp
$(object.path)/ares-fc-controller.o: $(ares.path)/fc/controller/controller.cpp
$(object.path)/ares-fc-expansion.o:  $(ares.path)/fc/expansion/expansion.cpp
$(object.path)/ares-fc-cartridge.o:  $(ares.path)/fc/cartridge/cartridge.cpp
$(object.path)/ares-fc-cpu.o:        $(ares.path)/fc/cpu/cpu.cpp
$(object.path)/ares-fc-apu.o:        $(ares.path)/fc/apu/apu.cpp
$(object.path)/ares-fc-ppu.o:        $(ares.path)/fc/ppu/ppu.cpp
$(object.path)/ares-fc-fds.o:        $(ares.path)/fc/fds/fds.cpp
