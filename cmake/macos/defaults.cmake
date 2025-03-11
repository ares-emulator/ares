# ares CMake macOS defaults module

include_guard(GLOBAL)

# Required to avoid us finding a system SDL2.framework before our provided SDL2.dylib
set(CMAKE_FIND_FRAMEWORK LAST)

# Set empty codesigning team if not specified as cache variable
if(NOT ARES_CODESIGN_TEAM)
  set(ARES_CODESIGN_TEAM "" CACHE STRING "ares code signing team for macOS" FORCE)

  # Set ad-hoc codesigning identity if not specified as cache variable
  if(NOT ARES_CODESIGN_IDENTITY)
    set(ARES_CODESIGN_IDENTITY "-" CACHE STRING "ares code signing identity for macOS" FORCE)
  endif()
endif()

find_program(ACTOOL_PROGRAM "actool")

include(xcode)

include(dependencies)

# Enable find_package targets to become globally available targets
set(CMAKE_FIND_PACKAGE_TARGETS_GLOBAL TRUE)
# Use RPATHs from build tree _in_ the build tree
set(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)
# Use common bundle-relative RPATH for installed targets
set(CMAKE_INSTALL_RPATH "@executable_path/../Frameworks")
# Use build tree as the install prefix
set(CMAKE_INSTALL_PREFIX ${CMAKE_BINARY_DIR})
