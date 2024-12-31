include_guard(GLOBAL)

# gersemi: off
set(CMAKE_MAP_IMPORTED_CONFIG_RELWITHDEBINFO RelWithDebInfo Release MinSizeRel None "")
set(CMAKE_MAP_IMPORTED_CONFIG_MINSIZEREL MinSizeRel Release RelWithDebInfo None "")
set(CMAKE_MAP_IMPORTED_CONFIG_RELEASE Release RelWithDebInfo MinSizeRel None "")
# gersemi: on

# Prohibit in-source builds
if("${CMAKE_CURRENT_BINARY_DIR}" STREQUAL "${CMAKE_CURRENT_SOURCE_DIR}")
  message(
    FATAL_ERROR
    "In-source builds of ares are not supported. "
    "Specify a build directory via 'cmake -S <SOURCE DIRECTORY> -B <BUILD_DIRECTORY>' instead."
  )
  file(REMOVE_RECURSE "${CMAKE_CURRENT_SOURCE_DIR}/CMakeCache.txt" "${CMAKE_CURRENT_SOURCE_DIR}/CMakeFiles")
endif()

# Set default global project variables
set(ARES_NAME "ares")
set(ARES_WEBSITE "https://ares-emu.net")
set(
  ARES_COMMENTS
  "ares is a cross-platform, open source, multi-system emulator, focusing on accuracy and preservation."
)
set(ARES_LEGAL_COPYRIGHT "Copyright (c) 2004-${CURRENT_YEAR} ares team, Near et. al.")

# Add common module directories to default search path
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/common" "${CMAKE_CURRENT_SOURCE_DIR}/cmake/finders")

include(versionconfig)
include(osconfig)

# Allow selection of common build types via UI
if(NOT CMAKE_GENERATOR MATCHES "(Xcode|Ninja Multi-Config|Visual Studio .+)")
  if(NOT CMAKE_BUILD_TYPE)
    set(
      CMAKE_BUILD_TYPE
      "RelWithDebInfo"
      CACHE STRING
      "ares build type [Release, RelWithDebInfo, Debug, MinSizeRel]"
      FORCE
    )
    set_property(
      CACHE CMAKE_BUILD_TYPE
      PROPERTY STRINGS Release RelWithDebInfo Debug MinSizeRel
    )
  endif()
endif()

# Enable default inclusion of targets' source and binary directory
set(CMAKE_INCLUDE_CURRENT_DIR TRUE)
