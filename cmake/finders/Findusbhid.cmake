#[=======================================================================[.rst:
Findusbhid
-------

Finds the usbhid library.

Imported Targets
^^^^^^^^^^^^^^^^

This module provides the following imported targets, if found:

``usbhid::usbhid``

Result Variables
^^^^^^^^^^^^^^^^

This will define the following variables:

``usbhid_FOUND``
  True if the system has usbhid.
``usbhid_LIBRARIES``
  The location of the usbhid library.
``usbhid_INCLUDE_DIR``
  Include directories needed to use usbhid.

#]=======================================================================]

include(FindPackageHandleStandardArgs)

find_path(
  usbhid_INCLUDE_DIR
  NAMES usbhid.h /dev/usb/usbhid.h
  PATHS /usr/include /usr/local/include
  DOC "usbhid include directory"
)

find_library(usbhid_LIBRARIES NAMES usbhid PATHS /usr/lib /usr/local/lib DOC "usbhid location")

set(usbhid_VERSION ${CMAKE_HOST_SYSTEM_VERSION})

find_package_handle_standard_args(
  usbhid
  REQUIRED_VARS usbhid_INCLUDE_DIR usbhid_LIBRARIES
  VERSION_VAR usbhid_VERSION
  REASON_FAILURE_MESSAGE "Ensure that usbhid is installed on the system."
)
mark_as_advanced(usbhid_INCLUDE_DIR usbhid_LIBRARY)

if(usbhid_FOUND)
  if(NOT TARGET usbhid::usbhid)
    add_library(usbhid::usbhid MODULE IMPORTED)
    set_property(TARGET usbhid::usbhid PROPERTY IMPORTED_LOCATION "${usbhid_LIBRARIES}")

    set_target_properties(
      usbhid::usbhid
      PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${usbhid_INCLUDE_DIR}"
        INTERFACE_LINK_LIBRARIES "${usbhid_LIBRARIES}"
        VERSION ${usbhid_VERSION}
    )
  endif()
endif()
