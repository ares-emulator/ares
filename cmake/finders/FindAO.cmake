#[=======================================================================[.rst:
FindSDL
-------

Finds the SDL library.

Imported Targets
^^^^^^^^^^^^^^^^

This module provides the following imported targets, if found:

``GTK::GTK``

Result Variables
^^^^^^^^^^^^^^^^

This will define the following variables:

``GTK_FOUND``
  True if the system has GTK 3.
``GTK_VERSION``
  The version of GTK3 which was found.
``GTK_INCLUDE_DIRS``
  Include directories needed to use GTK3.
``GTK_LIBRARIES``
  Libraries needed to link to GTK3.

#]=======================================================================]

include(FindPackageHandleStandardArgs)

find_package(PkgConfig QUIET)
if(PKG_CONFIG_FOUND)
  pkg_check_modules(PC_AO QUIET ao)
endif()

message(AUTHOR_WARNING "pc ao include dirs ${PC_AO_INCLUDE_DIR}")

find_path(
  AO_INCLUDE_DIR
  NAMES ao/ao.h
  HINTS ${PC_AO_INCLUDE_DIR}
  PATHS /usr/include /usr/local/include /usr/include/ao /usr/local/include/ao
  DOC "AO include directory"
)

find_library(AO_LIBRARIES NAMES ao HINTS ${PC_AO_LIBRARY_DIRS} PATHS /usr/lib /usr/local/lib DOC "SDL location")

set(GTK_ERROR_REASON "Ensure AO libraries are available in local library paths.")

find_package_handle_standard_args(
  AO
  REQUIRED_VARS AO_LIBRARIES AO_INCLUDE_DIR
  REASON_FAILURE_MESSAGE "${AO_ERROR_REASON}"
)
mark_as_advanced(GTK_INCLUDE_DIRS GTK_LIBRARY)
unset(GTK_ERROR_REASON)

if(AO_FOUND)
  if(NOT TARGET AO::AO)
    add_library(AO::AO UNKNOWN IMPORTED)
    set_property(TARGET AO::AO PROPERTY IMPORTED_LOCATION "${AO_LIBRARIES}")

    set_target_properties(
      AO::AO
      PROPERTIES
        INTERFACE_COMPILE_OPTIONS "${PC_AO_CFLAGS_OTHER}"
        INTERFACE_INCLUDE_DIRECTORIES "${PC_AO_INCLUDE_DIR}"
        INTERFACE_LINK_LIBRARIES "${PC_AO_LINK_LIBRARIES}"
    )
  endif()
endif()

include(FeatureSummary)
set_package_properties(
  AO
  PROPERTIES
    URL "https://xiph.org/ao/"
    DESCRIPTION
      "Libao is a cross-platform audio library that allows programs to output audio using a simple API on a wide variety of platforms."
)
