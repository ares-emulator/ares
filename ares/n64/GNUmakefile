ares.components += sm5k

ares.objects += ares-n64-memory
ares.objects += ares-n64-system
ares.objects += ares-n64-cartridge
ares.objects += ares-n64-controller
ares.objects += ares-n64-dd
ares.objects += ares-n64-mi
ares.objects += ares-n64-vi
ares.objects += ares-n64-ai
ares.objects += ares-n64-pi
ares.objects += ares-n64-ri
ares.objects += ares-n64-si
ares.objects += ares-n64-rdram
ares.objects += ares-n64-cpu
ares.objects += ares-n64-rdp
ares.objects += ares-n64-rsp

$(object.path)/ares-n64-memory.o:     $(ares.path)/n64/memory/memory.cpp
$(object.path)/ares-n64-system.o:     $(ares.path)/n64/system/system.cpp
$(object.path)/ares-n64-cartridge.o:  $(ares.path)/n64/cartridge/cartridge.cpp
$(object.path)/ares-n64-controller.o: $(ares.path)/n64/controller/controller.cpp
$(object.path)/ares-n64-dd.o:         $(ares.path)/n64/dd/dd.cpp
$(object.path)/ares-n64-sp.o:         $(ares.path)/n64/sp/sp.cpp
$(object.path)/ares-n64-dp.o:         $(ares.path)/n64/dp/dp.cpp
$(object.path)/ares-n64-mi.o:         $(ares.path)/n64/mi/mi.cpp
$(object.path)/ares-n64-vi.o:         $(ares.path)/n64/vi/vi.cpp
$(object.path)/ares-n64-ai.o:         $(ares.path)/n64/ai/ai.cpp
$(object.path)/ares-n64-pi.o:         $(ares.path)/n64/pi/pi.cpp
$(object.path)/ares-n64-ri.o:         $(ares.path)/n64/ri/ri.cpp
$(object.path)/ares-n64-si.o:         $(ares.path)/n64/si/si.cpp
$(object.path)/ares-n64-rdram.o:      $(ares.path)/n64/rdram/rdram.cpp
$(object.path)/ares-n64-cpu.o:        $(ares.path)/n64/cpu/cpu.cpp
$(object.path)/ares-n64-rdp.o:        $(ares.path)/n64/rdp/rdp.cpp
$(object.path)/ares-n64-rsp.o:        $(ares.path)/n64/rsp/rsp.cpp

flags   += -msse4.2
options += -msse4.2

ifeq ($(vulkan),true)
  ares.objects += ares-n64-vulkan
  $(object.path)/ares-n64-vulkan.o: $(ares.path)/n64/vulkan/vulkan.cpp
  PARALLEL_RDP_IMPLEMENTATION := $(ares.path)/n64/vulkan/parallel-rdp
  include $(PARALLEL_RDP_IMPLEMENTATION)/config.mk
  ares.objects += $(PARALLEL_RDP_SOURCES_CXX:$(PARALLEL_RDP_IMPLEMENTATION)/%.cpp=ares-n64-parallel-rdp/%)
  ares.objects += $(PARALLEL_RDP_SOURCES_C:$(PARALLEL_RDP_IMPLEMENTATION)/%.c=ares-n64-parallel-rdp/%)
  flags += $(PARALLEL_RDP_INCLUDE_DIRS) $(PARALLEL_RDP_CXXFLAGS) $(PARALLEL_RDP_CFLAGS)
  $(object.path)/ares-n64-parallel-rdp/%.o: $(PARALLEL_RDP_IMPLEMENTATION)/%.cpp
	$(info Compiling $(subst ../,,$<) ...)
	@mkdir -p $(dir $@)
	@$(call compile)
  $(object.path)/ares-n64-parallel-rdp/%.o: $(PARALLEL_RDP_IMPLEMENTATION)/%.c
	$(info Compiling $(subst ../,,$<) ...)
	@mkdir -p $(dir $@)
	@$(call compile)
endif
