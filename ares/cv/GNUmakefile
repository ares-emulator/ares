ares.components += z80
ares.components += tms9918
ares.components += sn76489

ares.objects += ares-cv-cpu
ares.objects += ares-cv-vdp
ares.objects += ares-cv-psg
ares.objects += ares-cv-system
ares.objects += ares-cv-cartridge
ares.objects += ares-cv-controller

$(object.path)/ares-cv-cpu.o:        $(ares.path)/cv/cpu/cpu.cpp
$(object.path)/ares-cv-vdp.o:        $(ares.path)/cv/vdp/vdp.cpp
$(object.path)/ares-cv-psg.o:        $(ares.path)/cv/psg/psg.cpp
$(object.path)/ares-cv-system.o:     $(ares.path)/cv/system/system.cpp
$(object.path)/ares-cv-cartridge.o:  $(ares.path)/cv/cartridge/cartridge.cpp
$(object.path)/ares-cv-controller.o: $(ares.path)/cv/controller/controller.cpp
