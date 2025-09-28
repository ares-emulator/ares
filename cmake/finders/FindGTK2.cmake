#[=======================================================================[.rst:
FindGTK2
-------

Finds the GTK2 library.

Imported Targets
^^^^^^^^^^^^^^^^

This module provides the following imported targets, if found:

``GTK2::GTK2``

Result Variables
^^^^^^^^^^^^^^^^

This will define the following variables:

``GTK2_FOUND``
  True if the system has GTK 2.
``GTK2_VERSION``
  The version of GTK3 which was found.
``GTK2_INCLUDE_DIRS``
  Include directories needed to use GTK2.
``GTK2_LIBRARIES``
  Libraries needed to link to GTK2.

Cache Variables
^^^^^^^^^^^^^^^

The following cache variables may also be set:
.
``GTK2_LIBRARY``
  The path to the GTK2 library.

#]=======================================================================]

include(FindPackageHandleStandardArgs)

find_package(PkgConfig QUIET)
if(PKG_CONFIG_FOUND)
  pkg_check_modules(PC_GTK2 QUIET gtk+-2.0)
endif()

find_path(
  GTK2_INCLUDE_DIRS
  NAMES gtk.h gtk/gtk.h
  HINTS ${PC_GTK2_INCLUDE_DIRS}
  PATHS /usr/include /usr/local/include /usr/include/gtk-2.0
  DOC "GTK2 include directory"
)

find_library(GTK2_LIBRARY NAMES gtk-x11-2.0 HINTS ${PC_GTK2_LIBRARY_DIRS} PATHS /usr/lib /usr/local/lib DOC "GTK2 location")

set(GTK2_VERSION ${PC_GTK2_VERSION})

set(GTK2_ERROR_REASON "Ensure GTK2 libraries are available in local library paths.")

find_package_handle_standard_args(
  GTK2
  REQUIRED_VARS GTK2_LIBRARY GTK2_INCLUDE_DIRS
  VERSION_VAR GTK2_VERSION
  REASON_FAILURE_MESSAGE "${GTK2_ERROR_REASON}"
)
mark_as_advanced(GTK2_INCLUDE_DIRS GTK2_LIBRARY)
unset(GTK2_ERROR_REASON)

if(GTK2_FOUND)
  if(NOT TARGET GTK2::GTK2)
    if(IS_ABSOLUTE "${GTK2_LIBRARY}")
      add_library(GTK2::GTK2 UNKNOWN IMPORTED)
      set_property(TARGET GTK2::GTK2 PROPERTY IMPORTED_LOCATION "${GTK2_LIBRARY}")
    else()
      add_library(GTK2::GTK2 SHARED IMPORTED)
      set_property(TARGET GTK2::GTK2 PROPERTY IMPORTED_LIBNAME "${GTK2_LIBRARY}")
    endif()

    set_target_properties(
      GTK2::GTK2
      PROPERTIES
        INTERFACE_COMPILE_OPTIONS "${PC_GTK2_CFLAGS_OTHER}"
        INTERFACE_INCLUDE_DIRECTORIES "${PC_GTK2_INCLUDE_DIRS}"
        INTERFACE_LINK_LIBRARIES "${PC_GTK2_LINK_LIBRARIES}"
        VERSION ${GTK2_VERSION}
    )
  endif()
endif()

include(FeatureSummary)
set_package_properties(
  GTK2
  PROPERTIES
    URL "https://www.gtk.org"
    DESCRIPTION "GTK is a free and open-source cross-platform widget toolkit for creating graphical user interfaces."
)
