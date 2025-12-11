cmake_minimum_required(VERSION 3.28...4.1)

include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/common/bootstrap.cmake" NO_POLICY_SCOPE)

project(ares VERSION ${ARES_VERSION_CANONICAL})

include(compilerconfig)
include(defaults)
include(helpers)

add_subdirectory(thirdparty)

add_subdirectory(nall/nall)
add_subdirectory(libco)
add_subdirectory(ruby)
add_subdirectory(hiro)

# gersemi: off
set(
  ARES_CORES
  a26 fc sfc sg ms md ps1 pce ng msx cv myvision gb gba ws ngp spec n64
  CACHE STRING "List of systems to be built by the project"
)
# gersemi: on

add_subdirectory(ares)
add_subdirectory(mia)
add_subdirectory(desktop-ui)

option(ARES_BUILD_OPTIONAL_TARGETS "Include supplemental tools and tests" OFF)

if(ARES_BUILD_OPTIONAL_TARGETS)
  add_subdirectory(tests/arm7tdmi)
  add_subdirectory(tests/i8080)
  add_subdirectory(tests/m68000)
  if(HIRO_BACKEND STREQUAL "GTK3")
    add_subdirectory(tools/genius)
  else()
    target_disable_subproject(genius "genius (database editor)")
  endif()
  add_subdirectory(tools/mame2bml)
else()
  target_disable_subproject(arm7tdmi "arm7tdmi processor test harness")
  target_disable_subproject(i8080 "i8080 processor test harness")
  target_disable_subproject(m68000 "m68000 processor test harness")
  target_disable_subproject(mame2bml "mame2bml (MAME manifest converter)")
  target_disable_subproject(genius "genius (database editor)")
endif()

add_subdirectory(tools/sourcery)

message_configuration()

add_subdirectory(.github)
add_subdirectory(cmake)
return()
