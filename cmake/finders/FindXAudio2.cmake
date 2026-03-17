# FindXAudio2
# -----------
# Detect whether XAudio2 is available and determine the correct library to link.
#
# Result variables:
#   XAudio2_FOUND      - TRUE if a linkable XAudio2 was found
#   XAudio2_LIBRARY    - Library name to pass to target_link_libraries
#
# Imported targets:
#   XAudio2::XAudio2

include(FindPackageHandleStandardArgs)

if(MSVC)
  # On MSVC toolchains the umbrella import library is
  # provided by the Windows SDK and requires no extra detection.
  set(XAudio2_LIBRARY "xaudio2")
else()
  # On other non-MSVC toolchain
  include(CheckCXXSourceCompiles)
  include(CMakePushCheckState)

  # Minimal test program that creates an XAudio2 instance.
  # XAudio2Create is the canonical entry point present in all supported versions.
  set(_xaudio2_test_code "
    #define WIN32_LEAN_AND_MEAN
    #include <windows.h>
    #include <xaudio2.h>
    int main() {
      IXAudio2* p = nullptr;
      XAudio2Create(&p, 0, XAUDIO2_DEFAULT_PROCESSOR);
      return 0;
    }
  ")

  cmake_push_check_state(RESET)
  # Suppress MSVC extension warnings that appear in MinGW XAudio2 headers
  set(CMAKE_REQUIRED_FLAGS "-Wno-language-extension-token")

  # Prefer the versioned import libraries shipped with newer MinGW sysroots.
  # Fall back to the unversioned name if neither is available.
  foreach(_lib IN ITEMS xaudio2_9 xaudio2_8)
    set(CMAKE_REQUIRED_LIBRARIES "-l${_lib}")
    check_cxx_source_compiles("${_xaudio2_test_code}" _HAVE_XAUDIO2_${_lib})
    if(_HAVE_XAUDIO2_${_lib})
      set(XAudio2_LIBRARY "${_lib}")
      break()
    endif()
  endforeach()

  cmake_pop_check_state()
endif()

find_package_handle_standard_args(XAudio2
  REQUIRED_VARS XAudio2_LIBRARY
)

if(XAudio2_FOUND AND NOT TARGET XAudio2::XAudio2)
  add_library(XAudio2::XAudio2 INTERFACE IMPORTED)
  target_link_libraries(XAudio2::XAudio2 INTERFACE ${XAudio2_LIBRARY})
endif()
