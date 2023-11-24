add_library(ares STATIC ares/ares.cpp ares/memory/fixed-allocator.cpp)

add_library(ares::ares ALIAS ares)

option(ARES_PROFILE_ACCURACY "Build to prefer emulation accuracy over performance in certain specific areas" OFF)
option(ARES_PRECOMPILE_HEADERS "Precompile ares/ares.hpp header for performance" OFF)
option(
  ARES_COMPILE_CORES_AS_SINGLE_UNIT
  "\
Build each core as one translation unit. slightly improves performance and cold build times, but reduces \
incremental build performance.\
"
  OFF
)

add_sourcery_command(ares ares/resource)

if(ARES_PRECOMPILE_HEADERS)
  target_precompile_headers(ares PRIVATE "$<$<COMPILE_LANGUAGE:CXX>:ares/ares.hpp>")
endif()

target_link_libraries(ares PUBLIC libco sljit nall)

if(ARES_PROFILE_ACCURACY)
  target_compile_definitions(ares PUBLIC PROFILE_ACCURACY)
else()
  target_compile_definitions(ares PUBLIC PROFILE_PERFORMANCE)
endif()

target_sources(
  ares
  PRIVATE ares/ares.cpp ares/ares.hpp ares/inline.hpp ares/platform.hpp ares/random.hpp ares/types.hpp
)

target_sources(
  ares
  PRIVATE
    ares/debug/debug.cpp
    ares/debug/debug.hpp
    ares/memory/fixed-allocator.cpp
    ares/memory/fixed-allocator.hpp
    ares/memory/memory.hpp
    ares/memory/readable.hpp
    ares/memory/writable.hpp
)

target_sources(
  ares
  PRIVATE ares/scheduler/scheduler.cpp ares/scheduler/scheduler.hpp ares/scheduler/thread.cpp ares/scheduler/thread.hpp
)

target_sources(
  ares
  PRIVATE
    ares/node/attribute.hpp
    ares/node/class.hpp
    ares/node/node.cpp
    ares/node/node.hpp
    ares/node/object.hpp
    ares/node/peripheral.hpp
    ares/node/port.hpp
    ares/node/system.hpp
)

target_sources(ares PRIVATE ares/node/audio/audio.hpp ares/node/audio/stream.cpp ares/node/audio/stream.hpp)

target_sources(ares PRIVATE ares/node/component/component.hpp ares/node/component/real-time-clock.hpp)

target_sources(
  ares
  PRIVATE
    ares/node/input/axis.hpp
    ares/node/input/button.hpp
    ares/node/input/input.hpp
    ares/node/input/rumble.hpp
    ares/node/input/trigger.hpp
)

target_sources(
  ares
  PRIVATE
    ares/node/setting/boolean.hpp
    ares/node/setting/integer.hpp
    ares/node/setting/natural.hpp
    ares/node/setting/real.hpp
    ares/node/setting/setting.hpp
    ares/node/setting/string.hpp
)

target_sources(
  ares
  PRIVATE
    ares/node/video/screen.cpp
    ares/node/video/screen.hpp
    ares/node/video/sprite.cpp
    ares/node/video/sprite.hpp
    ares/node/video/video.hpp
)

if(OS_WINDOWS)
  include(cmake/os-windows.cmake)
elseif(OS_MACOS)
  include(cmake/os-macos.cmake)
elseif(OS_LINUX)
  include(cmake/os-linux.cmake)
elseif(OS_FREEBSD OR OS_OPENBSD)
  include(cmake/os-freebsd.cmake)
endif()

macro(ares_components)
  list(APPEND ares.components ${ARGV})
  set(ares.components ${ares.components} PARENT_SCOPE)
endmacro()

macro(ares_add_header_sources core)
  foreach(arg IN ITEMS ${ARGN})
    list(APPEND ARES_HEADER_SOURCES "${core}/${arg}")
  endforeach()
  target_sources(ares PRIVATE ${ARGV})
  set(ARES_HEADER_SOURCES ${ARES_HEADER_SOURCES} PARENT_SCOPE)
endmacro()

macro(ares_add_sources)
  set(options "")
  set(oneValueArgs CORE)
  set(multiValueArgs PRIMARY INCLUDED UNITY)
  cmake_parse_arguments(AAS "" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

  target_sources(ares PRIVATE ${AAS_INCLUDED} ${AAS_PRIMARY})

  if(ARES_COMPILE_CORES_AS_SINGLE_UNIT)
    list(APPEND AAS_INCLUDED ${AAS_PRIMARY})
    target_sources(ares PRIVATE ${AAS_UNITY})
  endif()

  foreach(arg IN ITEMS ${AAS_INCLUDED})
    list(APPEND ARES_HEADER_SOURCES "${AAS_CORE}/${arg}")
  endforeach()
  set(ARES_HEADER_SOURCES ${ARES_HEADER_SOURCES} PARENT_SCOPE)
endmacro()

list(TRANSFORM ARES_CORES STRIP)

if(a26 IN_LIST ARES_CORES)
  enable_core("Atari 2600")
  add_subdirectory(a26)
else()
  disable_core("Atari 2600")
endif()

if(fc IN_LIST ARES_CORES)
  enable_core("NES / Famicom")
  add_subdirectory(fc)
else()
  disable_core("NES / Famicom")
endif()

if(sfc IN_LIST ARES_CORES)
  enable_core("SNES / Super Famicom")
  add_subdirectory(sfc)
else()
  disable_core("SNES / Super Famicom")
endif()

if(n64 IN_LIST ARES_CORES)
  enable_core("Nintendo 64")
  add_subdirectory(n64)
else()
  disable_core("Nintendo 64")
endif()

if(sg IN_LIST ARES_CORES)
  enable_core("Sega SG-1000")
  add_subdirectory(sg)
else()
  disable_core("Sega SG-1000")
endif()

if(ms IN_LIST ARES_CORES)
  enable_core("Sega Master System / Mark III")
  add_subdirectory(ms)
else()
  disable_core("Sega Master System / Mark III")
endif()

if(md IN_LIST ARES_CORES)
  enable_core("Sega Mega Drive / Genesis")
  add_subdirectory(md)
else()
  disable_core("Sega Mega Drive / Genesis")
endif()

if(saturn IN_LIST ARES_CORES)
  enable_core("Sega Saturn")
  add_subdirectory(saturn)
else()
  disable_core("Sega Saturn")
endif()

if(ps1 IN_LIST ARES_CORES)
  enable_core("PlayStation")
  add_subdirectory(ps1)
else()
  disable_core("Playstation")
endif()

if(pce IN_LIST ARES_CORES)
  enable_core("PC-Engine / TurboGrafx")
  add_subdirectory(pce)
else()
  disable_core("PC-Engine / TurboGrafx")
endif()

if(msx IN_LIST ARES_CORES)
  enable_core("Microsoft MSX")
  add_subdirectory(msx)
else()
  disable_core("Microsoft MSX")
endif()

if(cv IN_LIST ARES_CORES)
  enable_core("ColecoVision")
  add_subdirectory(cv)
else()
  disable_core("ColecoVision")
endif()

if(myvision IN_LIST ARES_CORES)
  enable_core("My Vision")
  add_subdirectory(myvision)
else()
  disable_core("My Vision")
endif()

if(gb IN_LIST ARES_CORES)
  enable_core("Game Boy / Game Boy Color")
  add_subdirectory(gb)
else()
  disable_core("Game Boy / Game Boy Color")
endif()

if(gba IN_LIST ARES_CORES)
  enable_core("Game Boy Advance")
  add_subdirectory(gba)
else()
  disable_core("Game Boy Advance")
endif()

if(ws IN_LIST ARES_CORES)
  enable_core("WonderSwan / WonderSwan Color")
  add_subdirectory(ws)
else()
  disable_core("WonderSwan / WonderSwan Color")
endif()

if(ng IN_LIST ARES_CORES)
  enable_core("Neo Geo (AES/MVS)")
  add_subdirectory(ng)
else()
  disable_core("Neo Geo (AES/MVS)")
endif()

if(ngp IN_LIST ARES_CORES)
  enable_core("Neo Geo Pocket / Neo Geo Pocket Color")
  add_subdirectory(ngp)
else()
  disable_core("Neo Geo Pocket / Neo Geo Pocket Color")
endif()

if(spec IN_LIST ARES_CORES)
  enable_core("ZX Spectrum")
  add_subdirectory(spec)
else()
  disable_core("ZX Spectrum")
endif()

add_subdirectory(component)

foreach(c IN LISTS ARES_CORES)
  # target_enable_feature(ares "${c} core" CORE_$<UPPER_CASE:${c}>)
  target_compile_definitions(ares PUBLIC CORE_$<UPPER_CASE:${c}>)
endforeach()

get_target_property(ares_SOURCES ares SOURCES)

set_source_files_properties(
  ares
  ares/debug/debug.cpp
  ares/node/audio/stream.cpp
  ares/node/node.cpp
  ares/node/video/screen.cpp
  ares/node/video/sprite.cpp
  ares/scheduler/thread.cpp
  ares/scheduler/scheduler.cpp
  PROPERTIES HEADER_FILE_ONLY TRUE
)

set_source_files_properties(
  n64/vulkan/parallel-rdp/vulkan/shader.cpp
  n64/vulkan/parallel-rdp/vulkan/descriptor_set.cpp
  n64/vulkan/parallel-rdp/vulkan/device.cpp
  n64/vulkan/parallel-rdp/vulkan/command_buffer.cpp
  n64/vulkan/parallel-rdp/vulkan/render_pass.cpp
  PROPERTIES SKIP_PRECOMPILE_HEADERS ON
)

target_sources(ares PRIVATE cmake/os-macos.cmake cmake/os-windows.cmake cmake/os-linux.cmake)

set_source_files_properties(ares ${ARES_HEADER_SOURCES} PROPERTIES HEADER_FILE_ONLY TRUE)

set_source_files_properties(ares ares/ares.cpp ares/memory/fixed-allocator.cpp PROPERTIES HEADER_FILE_ONLY FALSE)

target_include_directories(ares INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${ares_SOURCES})
