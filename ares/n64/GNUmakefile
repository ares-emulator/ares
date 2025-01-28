ares.components += sm5k

ares.objects += ares-n64-memory
ares.objects += ares-n64-system
ares.objects += ares-n64-cartridge
ares.objects += ares-n64-cic
ares.objects += ares-n64-controller
ares.objects += ares-n64-dd
ares.objects += ares-n64-mi
ares.objects += ares-n64-vi
ares.objects += ares-n64-ai
ares.objects += ares-n64-pi
ares.objects += ares-n64-pif
ares.objects += ares-n64-ri
ares.objects += ares-n64-si
ares.objects += ares-n64-rdram
ares.objects += ares-n64-cpu
ares.objects += ares-n64-rsp
ares.objects += ares-n64-rdp
ares.objects += ares-n64-aleck64

$(object.path)/ares-n64-memory.o:     $(ares.path)/n64/memory/memory.cpp
$(object.path)/ares-n64-system.o:     $(ares.path)/n64/system/system.cpp
$(object.path)/ares-n64-cartridge.o:  $(ares.path)/n64/cartridge/cartridge.cpp
$(object.path)/ares-n64-cic.o:        $(ares.path)/n64/cic/cic.cpp
$(object.path)/ares-n64-controller.o: $(ares.path)/n64/controller/controller.cpp
$(object.path)/ares-n64-dd.o:         $(ares.path)/n64/dd/dd.cpp
$(object.path)/ares-n64-sp.o:         $(ares.path)/n64/sp/sp.cpp
$(object.path)/ares-n64-dp.o:         $(ares.path)/n64/dp/dp.cpp
$(object.path)/ares-n64-mi.o:         $(ares.path)/n64/mi/mi.cpp
$(object.path)/ares-n64-vi.o:         $(ares.path)/n64/vi/vi.cpp
$(object.path)/ares-n64-ai.o:         $(ares.path)/n64/ai/ai.cpp
$(object.path)/ares-n64-pi.o:         $(ares.path)/n64/pi/pi.cpp
$(object.path)/ares-n64-pif.o:        $(ares.path)/n64/pif/pif.cpp
$(object.path)/ares-n64-ri.o:         $(ares.path)/n64/ri/ri.cpp
$(object.path)/ares-n64-si.o:         $(ares.path)/n64/si/si.cpp
$(object.path)/ares-n64-rdram.o:      $(ares.path)/n64/rdram/rdram.cpp
$(object.path)/ares-n64-cpu.o:        $(ares.path)/n64/cpu/cpu.cpp
$(object.path)/ares-n64-rsp.o:        $(ares.path)/n64/rsp/rsp.cpp
$(object.path)/ares-n64-rdp.o:        $(ares.path)/n64/rdp/rdp.cpp
$(object.path)/ares-n64-aleck64.o:    $(ares.path)/n64/aleck64/aleck64.cpp

ifeq ($(vulkan),true)
  ifeq ($(platform),macos)
    molten = $(ares.path)/../thirdparty/MoltenVK/libMoltenVK.dylib
    ifeq ($(wildcard $(molten)),)  
      molten = $(shell brew --prefix molten-vk)/lib/libMoltenVK.dylib
      ifeq ($(wildcard $(molten)),)
        $(error Compiling Ares N64 Vulkan backend requires MoltenVK. Install it via Homebrew, compile it using thirdparty/MoltenVK/build-moltenvk.sh, or disable with vulkan=false")
      endif
    endif
    ares.dylibs += $(molten)
  endif
  ares.objects += ares-n64-vulkan
  $(object.path)/ares-n64-vulkan.o: $(ares.path)/n64/vulkan/vulkan.cpp
  PARALLEL_RDP_IMPLEMENTATION := $(ares.path)/n64/vulkan/parallel-rdp
  include $(PARALLEL_RDP_IMPLEMENTATION)/config.mk
  ares.objects += $(PARALLEL_RDP_SOURCES_CXX:$(PARALLEL_RDP_IMPLEMENTATION)/%.cpp=ares-n64-parallel-rdp/%)
  ares.objects += $(PARALLEL_RDP_SOURCES_C:$(PARALLEL_RDP_IMPLEMENTATION)/%.c=ares-n64-parallel-rdp/%)
  flags += $(PARALLEL_RDP_INCLUDE_DIRS) $(PARALLEL_RDP_CXXFLAGS) $(PARALLEL_RDP_CFLAGS)
  $(object.path)/ares-n64-parallel-rdp/volk/volk.o:                     $(PARALLEL_RDP_IMPLEMENTATION)/volk/volk.c
  $(object.path)/ares-n64-parallel-rdp/parallel-rdp/video_interface.o:  $(PARALLEL_RDP_IMPLEMENTATION)/parallel-rdp/video_interface.cpp
  $(object.path)/ares-n64-parallel-rdp/parallel-rdp/command_ring.o:     $(PARALLEL_RDP_IMPLEMENTATION)/parallel-rdp/command_ring.cpp
  $(object.path)/ares-n64-parallel-rdp/parallel-rdp/rdp_device.o:       $(PARALLEL_RDP_IMPLEMENTATION)/parallel-rdp/rdp_device.cpp
  $(object.path)/ares-n64-parallel-rdp/parallel-rdp/rdp_dump_write.o:   $(PARALLEL_RDP_IMPLEMENTATION)/parallel-rdp/rdp_dump_write.cpp
  $(object.path)/ares-n64-parallel-rdp/parallel-rdp/rdp_renderer.o:     $(PARALLEL_RDP_IMPLEMENTATION)/parallel-rdp/rdp_renderer.cpp
  $(object.path)/ares-n64-parallel-rdp/vulkan/buffer.o:                 $(PARALLEL_RDP_IMPLEMENTATION)/vulkan/buffer.cpp
  $(object.path)/ares-n64-parallel-rdp/vulkan/buffer_pool.o:            $(PARALLEL_RDP_IMPLEMENTATION)/vulkan/buffer_pool.cpp
  $(object.path)/ares-n64-parallel-rdp/vulkan/command_buffer.o:         $(PARALLEL_RDP_IMPLEMENTATION)/vulkan/command_buffer.cpp
  $(object.path)/ares-n64-parallel-rdp/vulkan/command_pool.o:           $(PARALLEL_RDP_IMPLEMENTATION)/vulkan/command_pool.cpp
  $(object.path)/ares-n64-parallel-rdp/vulkan/context.o:                $(PARALLEL_RDP_IMPLEMENTATION)/vulkan/context.cpp
  $(object.path)/ares-n64-parallel-rdp/vulkan/cookie.o:                 $(PARALLEL_RDP_IMPLEMENTATION)/vulkan/cookie.cpp
  $(object.path)/ares-n64-parallel-rdp/vulkan/descriptor_set.o:         $(PARALLEL_RDP_IMPLEMENTATION)/vulkan/descriptor_set.cpp
  $(object.path)/ares-n64-parallel-rdp/vulkan/device.o:                 $(PARALLEL_RDP_IMPLEMENTATION)/vulkan/device.cpp
  $(object.path)/ares-n64-parallel-rdp/vulkan/event_manager.o:          $(PARALLEL_RDP_IMPLEMENTATION)/vulkan/event_manager.cpp
  $(object.path)/ares-n64-parallel-rdp/vulkan/fence.o:                  $(PARALLEL_RDP_IMPLEMENTATION)/vulkan/fence.cpp
  $(object.path)/ares-n64-parallel-rdp/vulkan/fence_manager.o:          $(PARALLEL_RDP_IMPLEMENTATION)/vulkan/fence_manager.cpp
  $(object.path)/ares-n64-parallel-rdp/vulkan/image.o:                  $(PARALLEL_RDP_IMPLEMENTATION)/vulkan/image.cpp
  $(object.path)/ares-n64-parallel-rdp/vulkan/indirect_layout.o:        $(PARALLEL_RDP_IMPLEMENTATION)/vulkan/indirect_layout.cpp
  $(object.path)/ares-n64-parallel-rdp/vulkan/memory_allocator.o:       $(PARALLEL_RDP_IMPLEMENTATION)/vulkan/memory_allocator.cpp
  $(object.path)/ares-n64-parallel-rdp/vulkan/pipeline_event.o:         $(PARALLEL_RDP_IMPLEMENTATION)/vulkan/pipeline_event.cpp
  $(object.path)/ares-n64-parallel-rdp/vulkan/query_pool.o:             $(PARALLEL_RDP_IMPLEMENTATION)/vulkan/query_pool.cpp
  $(object.path)/ares-n64-parallel-rdp/vulkan/render_pass.o:            $(PARALLEL_RDP_IMPLEMENTATION)/vulkan/render_pass.cpp
  $(object.path)/ares-n64-parallel-rdp/vulkan/sampler.o:                $(PARALLEL_RDP_IMPLEMENTATION)/vulkan/sampler.cpp
  $(object.path)/ares-n64-parallel-rdp/vulkan/semaphore.o:              $(PARALLEL_RDP_IMPLEMENTATION)/vulkan/semaphore.cpp
  $(object.path)/ares-n64-parallel-rdp/vulkan/semaphore_manager.o:      $(PARALLEL_RDP_IMPLEMENTATION)/vulkan/semaphore_manager.cpp
  $(object.path)/ares-n64-parallel-rdp/vulkan/shader.o:                 $(PARALLEL_RDP_IMPLEMENTATION)/vulkan/shader.cpp
  $(object.path)/ares-n64-parallel-rdp/vulkan/texture/texture_format.o: $(PARALLEL_RDP_IMPLEMENTATION)/vulkan/texture/texture_format.cpp
  $(object.path)/ares-n64-parallel-rdp/util/arena_allocator.o:          $(PARALLEL_RDP_IMPLEMENTATION)/util/arena_allocator.cpp
  $(object.path)/ares-n64-parallel-rdp/util/logging.o:                  $(PARALLEL_RDP_IMPLEMENTATION)/util/logging.cpp
  $(object.path)/ares-n64-parallel-rdp/util/thread_id.o:                $(PARALLEL_RDP_IMPLEMENTATION)/util/thread_id.cpp
  $(object.path)/ares-n64-parallel-rdp/util/aligned_alloc.o:            $(PARALLEL_RDP_IMPLEMENTATION)/util/aligned_alloc.cpp
  $(object.path)/ares-n64-parallel-rdp/util/timer.o:                    $(PARALLEL_RDP_IMPLEMENTATION)/util/timer.cpp
  $(object.path)/ares-n64-parallel-rdp/util/timeline_trace_file.o:      $(PARALLEL_RDP_IMPLEMENTATION)/util/timeline_trace_file.cpp
  $(object.path)/ares-n64-parallel-rdp/util/environment.o:              $(PARALLEL_RDP_IMPLEMENTATION)/util/environment.cpp
  $(object.path)/ares-n64-parallel-rdp/util/thread_name.o:              $(PARALLEL_RDP_IMPLEMENTATION)/util/thread_name.cpp
else
  $(warning Ares n64 core requires Vulkan for RDP emulation, only titles using Software Rendering will function)
endif
