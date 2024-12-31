#[=======================================================================[.rst:
FindGTK
-------

Finds the GTK library.

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

Cache Variables
^^^^^^^^^^^^^^^

The following cache variables may also be set:
.
``GTK_LIBRARY``
  The path to the GTK library.

#]=======================================================================]

include(FindPackageHandleStandardArgs)

find_package(PkgConfig QUIET)
if(PKG_CONFIG_FOUND)
  pkg_check_modules(PC_GTK QUIET gtk+-3.0)
endif()

find_path(
  GTK_INCLUDE_DIRS
  NAMES gtk.h gtk/gtk.h
  HINTS ${PC_GTK_INCLUDE_DIRS}
  PATHS /usr/include /usr/local/include /usr/include/gtk-3.0
  DOC "GTK include directory"
)

find_library(GTK_LIBRARY NAMES gtk-3 HINTS ${PC_GTK_LIBRARY_DIRS} PATHS /usr/lib /usr/local/lib DOC "GTK location")

set(GTK_VERSION ${PC_GTK_VERSION})

set(GTK_ERROR_REASON "Ensure GTK libraries are available in local library paths.")

find_package_handle_standard_args(
  GTK
  REQUIRED_VARS GTK_LIBRARY GTK_INCLUDE_DIRS
  VERSION_VAR GTK_VERSION
  REASON_FAILURE_MESSAGE "${GTK_ERROR_REASON}"
)
mark_as_advanced(GTK_INCLUDE_DIRS GTK_LIBRARY)
unset(GTK_ERROR_REASON)

if(GTK_FOUND)
  if(NOT TARGET GTK::GTK)
    if(IS_ABSOLUTE "${GTK_LIBRARY}")
      add_library(GTK::GTK UNKNOWN IMPORTED)
      set_property(TARGET GTK::GTK PROPERTY IMPORTED_LOCATION "${GTK_LIBRARY}")
    else()
      add_library(GTK::GTK SHARED IMPORTED)
      set_property(TARGET GTK::GTK PROPERTY IMPORTED_LIBNAME "${GTK_LIBRARY}")
    endif()

    set_target_properties(
      GTK::GTK
      PROPERTIES
        INTERFACE_COMPILE_OPTIONS "${PC_GTK_CFLAGS_OTHER}"
        INTERFACE_INCLUDE_DIRECTORIES "${PC_GTK_INCLUDE_DIRS}"
        INTERFACE_LINK_LIBRARIES "${PC_GTK_LINK_LIBRARIES}"
        VERSION ${GTK_VERSION}
    )
  endif()
endif()

include(FeatureSummary)
set_package_properties(
  GTK
  PROPERTIES
    URL "https://www.gtk.org"
    DESCRIPTION "GTK is a free and open-source cross-platform widget toolkit for creating graphical user interfaces."
)
