ares_components(sm5k)

ares_add_sources(
  CORE #
    n64
  UNITY #
    n64.cpp
  PRIMARY #
    memory/memory.cpp
    system/system.cpp
    cartridge/cartridge.cpp
    cic/cic.cpp
    controller/controller.cpp
    dd/dd.cpp
    mi/mi.cpp
    vi/vi.cpp
    ai/ai.cpp
    pi/pi.cpp
    pif/pif.cpp
    ri/ri.cpp
    si/si.cpp
    rdram/rdram.cpp
    cpu/cpu.cpp
    rsp/rsp.cpp
    rdp/rdp.cpp
    aleck64/aleck64.cpp
)

target_enable_feature(ares "N64 Vulkan rendering with paraLLEl-RDP" VULKAN)
target_compile_definitions(ares PUBLIC VULKAN)

if(OS_WINDOWS)
  include(cmake/os-windows.cmake)
elseif(OS_MACOS)
  include(cmake/os-macos.cmake)
elseif(OS_LINUX OR OS_FREEBSD OR OS_OPENBSD)
  include(cmake/os-linux.cmake)
endif()

ares_add_sources(
  CORE #
    n64
  INCLUDED #
    n64.hpp
    CMakeLists.txt
)

ares_add_sources(
  CORE #
    n64
  INCLUDED #
    ai/ai.hpp
    ai/debugger.cpp
    ai/io.cpp
    ai/serialization.cpp
)

ares_add_sources(
  CORE #
    n64
  INCLUDED #
    aleck64/aleck64.hpp
    aleck64/controls.cpp
    aleck64/debugger.cpp
    aleck64/serialization.cpp
    aleck64/io.cpp
    aleck64/vdp.cpp
    aleck64/game-config/11beat.hpp
    aleck64/game-config/doncdoon.hpp
    aleck64/game-config/kurufev.hpp
    aleck64/game-config/mayjin3.hpp
    aleck64/game-config/starsldr.hpp
    aleck64/game-config/twrshaft.hpp
    aleck64/game-config/vivdolls.hpp
    aleck64/game-config/hipai.hpp
    aleck64/game-config/hipai2.hpp
    aleck64/game-config/srmvs.hpp
    aleck64/game-config/mtetrisc.hpp
)

ares_add_sources(
  CORE #
    n64
  INCLUDED #
    cartridge/cartridge.hpp
    cartridge/serialization.cpp
    cartridge/slot.cpp
    cartridge/slot.hpp
    cartridge/board/board.cpp
    cartridge/board/board.hpp
    cartridge/board/flash.cpp
    cartridge/board/flash.hpp
    cartridge/board/isviewer.cpp
    cartridge/board/isviewer.hpp
    cartridge/board/rtc.cpp
    cartridge/board/rtc.hpp
    cartridge/board/gs-eeprom.cpp
    cartridge/board/gs-eeprom.hpp
    cartridge/board/generic.cpp
    cartridge/board/nus-01a.cpp
    cartridge/board/nus-07a.cpp
    cartridge/board/datel-ref1256.cpp
    cartridge/board/datel-ref1329.cpp
)

ares_add_sources(
  CORE #
    n64
  INCLUDED #
    cic/cic.hpp
    cic/commands.cpp
    cic/io.cpp
    cic/serialization.cpp
)

ares_add_sources(
  CORE #
    n64
  INCLUDED #
    controller/controller.hpp
    controller/port.cpp
    controller/port.hpp
)

ares_add_sources(
  CORE #
    n64
  INCLUDED #
    controller/aleck64/aleck64.cpp
    controller/aleck64/aleck64.hpp
)

ares_add_sources(
  CORE #
    n64
  INCLUDED #
    controller/gamepad/gamepad.cpp
    controller/gamepad/gamepad.hpp
    controller/gamepad/transfer-pak.cpp
)

ares_add_sources(
  CORE #
    n64
  INCLUDED #
    controller/mouse/mouse.cpp
    controller/mouse/mouse.hpp
)

ares_add_sources(
  CORE #
    n64
  INCLUDED #
    cpu/algorithms.cpp
    cpu/context.cpp
    cpu/cpu.hpp
    cpu/dcache.cpp
    cpu/debugger.cpp
    cpu/disassembler.cpp
    cpu/exceptions.cpp
    cpu/interpreter-cop2.cpp
    cpu/interpreter-fpu.cpp
    cpu/interpreter-ipu.cpp
    cpu/interpreter-scc.cpp
    cpu/interpreter.cpp
    cpu/memory.cpp
    cpu/recompiler.cpp
    cpu/serialization.cpp
    cpu/tlb.cpp
)

ares_add_sources(
  CORE #
    n64
  INCLUDED #
    dd/controller.cpp
    dd/dd.hpp
    dd/debugger.cpp
    dd/drive.cpp
    dd/io.cpp
    dd/rtc.cpp
    dd/serialization.cpp
)

ares_add_sources(
  CORE #
    n64
  INCLUDED #
    memory/bus.hpp
    memory/io.hpp
    memory/memory.hpp
    memory/lsb/readable.hpp
    memory/lsb/writable.hpp
    memory/msb/readable.hpp
    memory/msb/writable.hpp
)

ares_add_sources(
  CORE #
    n64
  INCLUDED #
    mi/debugger.cpp
    mi/io.cpp
    mi/mi.hpp
    mi/serialization.cpp
)

ares_add_sources(
  CORE #
    n64
  INCLUDED #
    pi/bus.hpp
    pi/debugger.cpp
    pi/dma.cpp
    pi/io.cpp
    pi/pi.hpp
    pi/serialization.cpp
)

ares_add_sources(
  CORE #
    n64
  INCLUDED #
    pif/debugger.cpp
    pif/hle.cpp
    pif/io.cpp
    pif/pif.hpp
    pif/serialization.cpp
)

ares_add_sources(
  CORE #
    n64
  INCLUDED #
    rdp/debugger.cpp
    rdp/io.cpp
    rdp/rdp.hpp
    rdp/render.cpp
    rdp/serialization.cpp
)

ares_add_sources(
  CORE #
    n64
  INCLUDED #
    rdram/debugger.cpp
    rdram/io.cpp
    rdram/rdram.hpp
    rdram/serialization.cpp
)

ares_add_sources(
  CORE #
    n64
  INCLUDED #
    ri/debugger.cpp
    ri/io.cpp
    ri/ri.hpp
    ri/serialization.cpp
)

ares_add_sources(
  CORE #
    n64
  INCLUDED #
    rsp/debugger.cpp
    rsp/decoder.cpp
    rsp/disassembler.cpp
    rsp/dma.cpp
    rsp/interpreter-ipu.cpp
    rsp/interpreter-scc.cpp
    rsp/interpreter-vpu.cpp
    rsp/interpreter.cpp
    rsp/io.cpp
    rsp/recompiler.cpp
    rsp/rsp.hpp
    rsp/serialization.cpp
)

ares_add_sources(
  CORE #
    n64
  INCLUDED #
    si/debugger.cpp
    si/dma.cpp
    si/io.cpp
    si/serialization.cpp
    si/si.hpp
)

ares_add_sources(
  CORE #
    n64
  INCLUDED #
    system/serialization.cpp
    system/system.hpp
)

ares_add_sources(
  CORE #
    n64
  INCLUDED #
    vi/debugger.cpp
    vi/io.cpp
    vi/serialization.cpp
    vi/vi.hpp
)

target_compile_definitions(ares PRIVATE VULKAN)

set(
  _parallel_rdp_sources
  vulkan/vulkan.cpp
  vulkan/vulkan.hpp
  vulkan/parallel-rdp/parallel-rdp/command_ring.cpp
  vulkan/parallel-rdp/parallel-rdp/rdp_device.cpp
  vulkan/parallel-rdp/parallel-rdp/rdp_dump_write.cpp
  vulkan/parallel-rdp/parallel-rdp/rdp_renderer.cpp
  vulkan/parallel-rdp/parallel-rdp/video_interface.cpp
  vulkan/parallel-rdp/vulkan/buffer.cpp
  vulkan/parallel-rdp/vulkan/buffer_pool.cpp
  vulkan/parallel-rdp/vulkan/command_buffer.cpp
  vulkan/parallel-rdp/vulkan/command_pool.cpp
  vulkan/parallel-rdp/vulkan/context.cpp
  vulkan/parallel-rdp/vulkan/cookie.cpp
  vulkan/parallel-rdp/vulkan/descriptor_set.cpp
  vulkan/parallel-rdp/vulkan/device.cpp
  vulkan/parallel-rdp/vulkan/event_manager.cpp
  vulkan/parallel-rdp/vulkan/fence.cpp
  vulkan/parallel-rdp/vulkan/fence_manager.cpp
  vulkan/parallel-rdp/vulkan/image.cpp
  vulkan/parallel-rdp/vulkan/indirect_layout.cpp
  vulkan/parallel-rdp/vulkan/memory_allocator.cpp
  vulkan/parallel-rdp/vulkan/pipeline_event.cpp
  vulkan/parallel-rdp/vulkan/query_pool.cpp
  vulkan/parallel-rdp/vulkan/render_pass.cpp
  vulkan/parallel-rdp/vulkan/sampler.cpp
  vulkan/parallel-rdp/vulkan/semaphore.cpp
  vulkan/parallel-rdp/vulkan/semaphore_manager.cpp
  vulkan/parallel-rdp/vulkan/shader.cpp
  vulkan/parallel-rdp/vulkan/texture/texture_format.cpp
  vulkan/parallel-rdp/util/arena_allocator.cpp
  vulkan/parallel-rdp/util/logging.cpp
  vulkan/parallel-rdp/util/thread_id.cpp
  vulkan/parallel-rdp/util/aligned_alloc.cpp
  vulkan/parallel-rdp/util/timer.cpp
  vulkan/parallel-rdp/util/timeline_trace_file.cpp
  vulkan/parallel-rdp/util/thread_name.cpp
  vulkan/parallel-rdp/util/environment.cpp
  vulkan/parallel-rdp/volk/volk.c
)

if(ARES_UNITY_CORES)
  ares_add_sources(
    CORE #
      n64
    UNITY #
      ${_parallel_rdp_sources}
  )
else()
  ares_add_sources(
    CORE #
      n64
    PRIMARY #
      ${_parallel_rdp_sources}
  )
endif()

ares_add_sources(
  CORE #
    n64
  PRIMARY #
    vulkan/parallel-rdp/parallel-rdp/command_ring.hpp
    vulkan/parallel-rdp/parallel-rdp/rdp_device.hpp
    vulkan/parallel-rdp/parallel-rdp/rdp_dump_write.hpp
    vulkan/parallel-rdp/parallel-rdp/rdp_renderer.hpp
    vulkan/parallel-rdp/parallel-rdp/video_interface.hpp
    vulkan/parallel-rdp/vulkan/buffer.hpp
    vulkan/parallel-rdp/vulkan/buffer_pool.hpp
    vulkan/parallel-rdp/vulkan/command_buffer.hpp
    vulkan/parallel-rdp/vulkan/command_pool.hpp
    vulkan/parallel-rdp/vulkan/context.hpp
    vulkan/parallel-rdp/vulkan/cookie.hpp
    vulkan/parallel-rdp/vulkan/descriptor_set.hpp
    vulkan/parallel-rdp/vulkan/device.hpp
    vulkan/parallel-rdp/vulkan/event_manager.hpp
    vulkan/parallel-rdp/vulkan/fence.hpp
    vulkan/parallel-rdp/vulkan/fence_manager.hpp
    vulkan/parallel-rdp/vulkan/image.hpp
    vulkan/parallel-rdp/vulkan/indirect_layout.hpp
    vulkan/parallel-rdp/vulkan/memory_allocator.hpp
    vulkan/parallel-rdp/vulkan/pipeline_event.hpp
    vulkan/parallel-rdp/vulkan/query_pool.hpp
    vulkan/parallel-rdp/vulkan/render_pass.hpp
    vulkan/parallel-rdp/vulkan/sampler.hpp
    vulkan/parallel-rdp/vulkan/semaphore.hpp
    vulkan/parallel-rdp/vulkan/semaphore_manager.hpp
    vulkan/parallel-rdp/vulkan/shader.hpp
    vulkan/parallel-rdp/vulkan/texture/texture_format.hpp
    vulkan/parallel-rdp/util/arena_allocator.hpp
    vulkan/parallel-rdp/util/logging.hpp
    vulkan/parallel-rdp/util/thread_id.hpp
    vulkan/parallel-rdp/util/aligned_alloc.hpp
    vulkan/parallel-rdp/util/timer.hpp
    vulkan/parallel-rdp/util/timeline_trace_file.hpp
    vulkan/parallel-rdp/util/thread_name.hpp
    vulkan/parallel-rdp/util/environment.hpp
)

target_include_directories(
  ares
  PRIVATE
    vulkan/parallel-rdp/parallel-rdp
    vulkan/parallel-rdp/volk
    vulkan/parallel-rdp/vulkan
    vulkan/parallel-rdp/vulkan-headers/include
    vulkan/parallel-rdp/util
)

target_compile_options(ares PRIVATE $<$<CXX_COMPILER_ID:Clang,AppleClang>:-Wno-newline-eof>)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
target_link_libraries(ares PUBLIC Threads::Threads)

if(OS_WINDOWS)
  target_compile_definitions(ares PRIVATE VK_USE_PLATFORM_WIN32_KHR)
  target_link_libraries(ares PRIVATE winmm)
else()
  target_link_libraries(ares PRIVATE dl)
endif()
