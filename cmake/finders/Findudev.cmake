#[=======================================================================[.rst:
Findudev
-------

Finds the udev library.

Imported Targets
^^^^^^^^^^^^^^^^

This module provides the following imported targets, if found:

``udev::udev``

Result Variables
^^^^^^^^^^^^^^^^

This will define the following variables:

``udev_FOUND``
  True if the system has udev.
``udev_VERSION``
  The version of udev which was found.
``udev_INCLUDE_DIR``
  Include directories needed to use udev.
``udev_LIBRARIES``
  Libraries needed to link to udev.

Cache Variables
^^^^^^^^^^^^^^^

The following cache variables may also be set:

``udev_LIBRARY``
  The path to the udev library.

#]=======================================================================]

include(FindPackageHandleStandardArgs)

find_package(PkgConfig QUIET)
if(PKG_CONFIG_FOUND)
  pkg_check_modules(PC_udev QUIET udev)
endif()

find_path(
  udev_INCLUDE_DIR
  NAMES libudev.h
  HINTS ${PC_udev_INCLUDE_DIRS}
  PATHS /usr/include /usr/local/include
  DOC "udev include directory"
)

find_library(udev_LIBRARIES NAMES udev HINTS ${PC_udev_LIBRARY_DIRS} PATHS /usr/lib /usr/local/lib DOC "udev location")

set(udev_VERSION ${PC_udev_VERSION})
set(udev_ERROR_REASON "Ensure udev libraries are available in local library paths.")

find_package_handle_standard_args(
  udev
  REQUIRED_VARS udev_LIBRARIES udev_INCLUDE_DIR
  REASON_FAILURE_MESSAGE "${udev_ERROR_REASON}"
)
mark_as_advanced(udev_INCLUDE_DIR udev_LIBRARIES)
unset(udev_ERROR_REASON)

if(udev_FOUND)
  if(NOT TARGET udev::udev)
    add_library(udev::udev UNKNOWN IMPORTED)
    set_property(TARGET udev::udev PROPERTY IMPORTED_LOCATION "${udev_LIBRARIES}")

    set_target_properties(
      udev::udev
      PROPERTIES
        INTERFACE_COMPILE_OPTIONS "${PC_udev_CFLAGS_OTHER}"
        INTERFACE_INCLUDE_DIRECTORIES "${PC_udev_INCLUDE_DIR}"
        INTERFACE_LINK_LIBRARIES "${udev_LIBRARIES}"
        VERSION "${udev_VERSION}"
    )
  endif()
endif()

include(FeatureSummary)
set_package_properties(
  udev
  PROPERTIES
    URL "https://www.freedesktop.org/software/systemd/man/latest/libudev.html"
    DESCRIPTION "libudev.h provides an API to introspect and enumerate devices on the local system."
)
