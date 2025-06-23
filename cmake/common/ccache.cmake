# ares CMake Ccache module

include_guard(GLOBAL)

option(ENABLE_CCACHE "Enable compiler acceleration with Ccache" ON)

if(ENABLE_CCACHE)
  if(NOT DEFINED CCACHE_PROGRAM)
    message(STATUS "Trying to find Ccache on build host")
    find_program(CCACHE_PROGRAM "ccache")
    mark_as_advanced(CCACHE_PROGRAM)
    if(CCACHE_PROGRAM STREQUAL CCACHE_PROGRAM-NOTFOUND)
      message(STATUS "Trying to find Ccache on build host - not found")
    endif()
  endif()
else()
  message(STATUS "Skipping Ccache - set ENABLE_CCACHE to ON to enable Ccache")
endif()

if(ENABLE_CCACHE AND CCACHE_PROGRAM)
  message(STATUS "Trying to find Ccache on build host - success")
  set(CMAKE_C_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
  set(CMAKE_CXX_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
  set(CMAKE_OBJC_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
  set(CMAKE_OBJCXX_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
  message(STATUS "Ccache enabled - located at ${CCACHE_PROGRAM}")
endif()
