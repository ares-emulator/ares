ares.components += wdc65816
ares.components += spc700
ares.components += arm7tdmi
ares.components += gsu
ares.components += hg51b
ares.components += upd96050

ares.objects += ares-sfc-system
ares.objects += ares-sfc-controller
ares.objects += ares-sfc-cartridge
ares.objects += ares-sfc-memory
ares.objects += ares-sfc-cpu
ares.objects += ares-sfc-smp
ares.objects += ares-sfc-dsp
ares.objects += ares-sfc-ppu
ares.objects += ares-sfc-coprocessor
ares.objects += ares-sfc-expansion
ares.objects += ares-sfc-slot

$(object.path)/ares-sfc-system.o:      $(ares.path)/sfc/system/system.cpp
$(object.path)/ares-sfc-controller.o:  $(ares.path)/sfc/controller/controller.cpp
$(object.path)/ares-sfc-cartridge.o:   $(ares.path)/sfc/cartridge/cartridge.cpp
$(object.path)/ares-sfc-memory.o:      $(ares.path)/sfc/memory/memory.cpp

$(object.path)/ares-sfc-cpu.o:         $(ares.path)/sfc/cpu/cpu.cpp
$(object.path)/ares-sfc-smp.o:         $(ares.path)/sfc/smp/smp.cpp
$(object.path)/ares-sfc-dsp.o:         $(ares.path)/sfc/dsp/dsp.cpp
$(object.path)/ares-sfc-ppu.o:         $(ares.path)/sfc/ppu/ppu.cpp

$(object.path)/ares-sfc-coprocessor.o: $(ares.path)/sfc/coprocessor/coprocessor.cpp
$(object.path)/ares-sfc-expansion.o:   $(ares.path)/sfc/expansion/expansion.cpp
$(object.path)/ares-sfc-slot.o:        $(ares.path)/sfc/slot/slot.cpp
