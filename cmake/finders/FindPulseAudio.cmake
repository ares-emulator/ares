#[=======================================================================[.rst:
FindPulseAudio
-------

Finds the Pulse Audio library.

Imported Targets
^^^^^^^^^^^^^^^^

This module provides the following imported targets, if found:

``PulseAudio::PulseAudio``
``PulseAudio::PulseAudioSimple``

Result Variables
^^^^^^^^^^^^^^^^

This will define the following variables:

``PulseAudio_FOUND``
  True if the system has PulseAudio.
``PulseAudio_VERSION``
  The version of PulseAudio which was found.
``PulseAudio_INCLUDE_DIR``
  Include directories needed to use PulseAudio.
``PulseAudio_LIBRARY``
  The location of the PulseAudio library.
``PulseAudioSimple_LIBRARY``
  The location of the simple version of the PulseAudio library.
  

#]=======================================================================]

include(FindPackageHandleStandardArgs)

find_package(PkgConfig QUIET)
if(PKG_CONFIG_FOUND)
  pkg_search_module(PC_PulseAudio QUIET libpulse)
endif()

find_path(
  PulseAudio_INCLUDE_DIR
  NAMES pulse/pulseaudio.h
  HINTS ${PC_PulseAudio_INCLUDE_DIRS}
  PATHS /usr/include/ /usr/local/include
  DOC "PulseAudio include directory"
)

find_library(
  PulseAudio_LIBRARY
  NAMES pulse
  HINTS ${PC_PulseAudio_LIBRARY_DIRS}
  PATHS /usr/lib /usr/local/lib
  DOC "PulseAudio location"
)

find_library(
  PulseAudioSimple_LIBRARY
  NAMES pulse-simple
  HINTS ${PC_PulseAudio_LIBRARY_DIRS}
  PATHS /usr/lib /usr/local/lib
  DOC "PulseAudioSimple location"
)

set(PulseAudio_VERSION ${PC_PulseAudio_VERSION})

find_package_handle_standard_args(
  PulseAudio
  REQUIRED_VARS PulseAudio_INCLUDE_DIR PulseAudio_LIBRARY PulseAudioSimple_LIBRARY
  VERSION_VAR PulseAudio_VERSION
  REASON_FAILURE_MESSAGE "Ensure that PulseAudio and PulseAudioSimple are available in local library paths."
)
mark_as_advanced(PulseAudio_INCLUDE_DIR PulseAudio_LIBRARY)

if(PulseAudio_FOUND)
  if(NOT TARGET PulseAudio::PulseAudio)
    if(IS_ABSOLUTE "${PulseAudio_LIBRARY}")
      add_library(PulseAudio::PulseAudio UNKNOWN IMPORTED)
      set_property(TARGET PulseAudio::PulseAudio PROPERTY IMPORTED_LOCATION "${PulseAudio_LIBRARY}")
    else()
      add_library(PulseAudio::PulseAudio INTERFACE IMPORTED)
      set_property(TARGET PulseAudio::PulseAudio PROPERTY IMPORTED_LIBNAME "${PulseAudio_LIBRARY}")
    endif()
    if(IS_ABSOLUTE "${PulseAudioSimple_LIBRARY}")
      add_library(PulseAudio::PulseAudioSimple UNKNOWN IMPORTED)
      set_property(TARGET PulseAudio::PulseAudioSimple PROPERTY IMPORTED_LOCATION "${PulseAudioSimple_LIBRARY}")
    else()
      add_library(PulseAudio::PulseAudioSimple INTERFACE IMPORTED)
      set_property(TARGET PulseAudio::PulseAudioSimple PROPERTY IMPORTED_LIBNAME "${PulseAudioSimple_LIBRARY}")
    endif()

    set_target_properties(
      PulseAudio::PulseAudio
      PROPERTIES
        INTERFACE_COMPILE_OPTIONS "${PC_PulseAudio_CFLAGS_OTHER}"
        INTERFACE_INCLUDE_DIRECTORIES "${PulseAudio_INCLUDE_DIR}"
        VERSION ${PulseAudio_VERSION}
    )
    set_target_properties(
      PulseAudio::PulseAudioSimple
      PROPERTIES
        INTERFACE_COMPILE_OPTIONS "${PC_PulseAudio_CFLAGS_OTHER}"
        INTERFACE_INCLUDE_DIRECTORIES "${PulseAudio_INCLUDE_DIR}"
        VERSION ${PulseAudio_VERSION}
    )
  endif()
endif()

include(FeatureSummary)
set_package_properties(
  PulseAudio
  PROPERTIES
    URL "https://www.freedesktop.org/wiki/Software/PulseAudio/"
    DESCRIPTION
      "PulseAudio is a sound server system for POSIX OSes, meaning that it is a proxy for your sound applications."
)
