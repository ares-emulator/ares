ares.components += z80 tms9918 sn76489

ares.objects += ares-cv-interface
ares.objects += ares-cv-cpu ares-cv-vdp ares-cv-psg
ares.objects += ares-cv-system ares-cv-cartridge
ares.objects += ares-cv-controller

$(object.path)/ares-cv-interface.o:  $(ares.path)/cv/interface/interface.cpp
$(object.path)/ares-cv-cpu.o:        $(ares.path)/cv/cpu/cpu.cpp
$(object.path)/ares-cv-vdp.o:        $(ares.path)/cv/vdp/vdp.cpp
$(object.path)/ares-cv-psg.o:        $(ares.path)/cv/psg/psg.cpp
$(object.path)/ares-cv-system.o:     $(ares.path)/cv/system/system.cpp
$(object.path)/ares-cv-cartridge.o:  $(ares.path)/cv/cartridge/cartridge.cpp
$(object.path)/ares-cv-controller.o: $(ares.path)/cv/controller/controller.cpp
