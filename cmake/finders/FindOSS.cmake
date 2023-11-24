#[=======================================================================[.rst:
FindOSS
-------

Finds the OSS library.

Imported Targets
^^^^^^^^^^^^^^^^

This module provides the following imported targets, if found:

``GTK::GTK``

Result Variables
^^^^^^^^^^^^^^^^

This will define the following variables:

``OSS_FOUND``
  True if the system has OSS.
``OSS_VERSION``
  The version of OSS which was found.
``OSS_INCLUDE_DIR``
  Include directories needed to use OSS.

#]=======================================================================]

include(FindPackageHandleStandardArgs)

find_path(OSS_INCLUDE_DIR NAMES sys/soundcard.h PATHS /usr/include /usr/local/include DOC "OSS include directory")

set(OSS_VERSION ${CMAKE_HOST_SYSTEM_VERSION})

find_package_handle_standard_args(
  OSS
  REQUIRED_VARS OSS_INCLUDE_DIR
  VERSION_VAR OSS_VERSION
  REASON_FAILURE_MESSAGE "Ensure that OSS is installed on the system."
)
mark_as_advanced(OSS_INCLUDE_DIR OSS_LIBRARY)

if(OSS_FOUND)
  if(NOT TARGET OSS::OSS)
    add_library(OSS::OSS INTERFACE IMPORTED)

    set_target_properties(OSS::OSS PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${OSS_INCLUDE_DIR}" VERSION ${OSS_VERSION})
  endif()
endif()
