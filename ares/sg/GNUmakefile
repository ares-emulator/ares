ares.components += z80 tms9918 sn76489

ares.objects += ares-sg-interface
ares.objects += ares-sg-cpu ares-sg-vdp ares-sg-psg
ares.objects += ares-sg-system ares-sg-cartridge
ares.objects += ares-sg-controller

$(object.path)/ares-sg-interface.o:  $(ares.path)/sg/interface/interface.cpp
$(object.path)/ares-sg-cpu.o:        $(ares.path)/sg/cpu/cpu.cpp
$(object.path)/ares-sg-vdp.o:        $(ares.path)/sg/vdp/vdp.cpp
$(object.path)/ares-sg-psg.o:        $(ares.path)/sg/psg/psg.cpp
$(object.path)/ares-sg-system.o:     $(ares.path)/sg/system/system.cpp
$(object.path)/ares-sg-cartridge.o:  $(ares.path)/sg/cartridge/cartridge.cpp
$(object.path)/ares-sg-controller.o: $(ares.path)/sg/controller/controller.cpp
