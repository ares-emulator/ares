ares.components += m68k
ares.components += z80
ares.components += ym2610

ares.objects += ares-ng-system
ares.objects += ares-ng-cpu
ares.objects += ares-ng-apu
ares.objects += ares-ng-gpu
ares.objects += ares-ng-opnb
ares.objects += ares-ng-cartridge
ares.objects += ares-ng-controller

$(object.path)/ares-ng-system.o:     $(ares.path)/ng/system/system.cpp
$(object.path)/ares-ng-cpu.o:        $(ares.path)/ng/cpu/cpu.cpp
$(object.path)/ares-ng-apu.o:        $(ares.path)/ng/apu/apu.cpp
$(object.path)/ares-ng-gpu.o:        $(ares.path)/ng/gpu/gpu.cpp
$(object.path)/ares-ng-opnb.o:       $(ares.path)/ng/opnb/opnb.cpp
$(object.path)/ares-ng-cartridge.o:  $(ares.path)/ng/cartridge/cartridge.cpp
$(object.path)/ares-ng-controller.o: $(ares.path)/ng/controller/controller.cpp
