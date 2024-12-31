# ares CMake macOS compiler configuration module

include_guard(GLOBAL)

option(ENABLE_COMPILER_TRACE "Enable clang time-trace" OFF)
mark_as_advanced(ENABLE_COMPILER_TRACE)

include(ccache)
include(compiler_common)

# Enable selection between arm64 and x86_64 targets
if(NOT CMAKE_OSX_ARCHITECTURES)
  set(CMAKE_OSX_ARCHITECTURES arm64 CACHE STRING "Build architectures for macOS" FORCE)
endif()
set_property(CACHE CMAKE_OSX_ARCHITECTURES PROPERTY STRINGS arm64 x86_64)

# Ensure recent enough Xcode and platform SDK
set(_ares_macos_minimum_sdk 11.1) # Minimum tested SDK
set(_ares_macos_minimum_xcode 12.4) # Sync with SDK
message(DEBUG "macOS SDK Path: ${CMAKE_OSX_SYSROOT}")
string(REGEX MATCH ".+/MacOSX.platform/Developer/SDKs/MacOSX([0-9]+\\.[0-9])+\\.sdk$" _ ${CMAKE_OSX_SYSROOT})
set(_ares_macos_current_sdk ${CMAKE_MATCH_1})
message(DEBUG "macOS SDK version: ${_ares_macos_current_sdk}")
if(_ares_macos_current_sdk VERSION_LESS _ares_macos_minimum_sdk)
  message(
    FATAL_ERROR
    "Your macOS SDK version (${_ares_macos_current_sdk}) is too low. "
    "The macOS ${_ares_macos_minimum_sdk} SDK (Xcode ${_ares_macos_minimum_xcode}) is required to build ares."
  )
endif()
unset(_ares_macos_current_sdk)
unset(_ares_macos_minimum_sdk)
unset(_ares_macos_minimum_xcode)

# Enable dSYM generator for release builds
string(APPEND CMAKE_C_FLAGS_RELEASE " -g")
string(APPEND CMAKE_CXX_FLAGS_RELEASE " -g")
string(APPEND CMAKE_OBJC_FLAGS_RELEASE " -g")
string(APPEND CMAKE_OBJCXX_FLAGS_RELEASE " -g")

if(ENABLE_COMPILER_TRACE)
  add_compile_options(-ftime-trace)
  add_link_options(LINKER:-print_statistics)
endif()

if(NOT XCODE)
  add_compile_options(
    "$<$<COMPILE_LANGUAGE:C>:${_ares_clang_c_options}>"
    "$<$<COMPILE_LANGUAGE:CXX>:${_ares_clang_cxx_options}>"
    -mmacos-version-min=10.13
  )
endif()
