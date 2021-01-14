ares.components += z80
ares.components += tms9918
ares.components += sn76489

ares.objects += ares-sg-cpu
ares.objects += ares-sg-vdp
ares.objects += ares-sg-psg
ares.objects += ares-sg-system
ares.objects += ares-sg-cartridge
ares.objects += ares-sg-controller

$(object.path)/ares-sg-cpu.o:        $(ares.path)/sg/cpu/cpu.cpp
$(object.path)/ares-sg-vdp.o:        $(ares.path)/sg/vdp/vdp.cpp
$(object.path)/ares-sg-psg.o:        $(ares.path)/sg/psg/psg.cpp
$(object.path)/ares-sg-system.o:     $(ares.path)/sg/system/system.cpp
$(object.path)/ares-sg-cartridge.o:  $(ares.path)/sg/cartridge/cartridge.cpp
$(object.path)/ares-sg-controller.o: $(ares.path)/sg/controller/controller.cpp
