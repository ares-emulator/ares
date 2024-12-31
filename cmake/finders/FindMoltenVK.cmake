#[=======================================================================[.rst:
FindMoltenVK
-------

Finds the MoltenVK library.

Imported Targets
^^^^^^^^^^^^^^^^

This module provides the following imported targets, if found:

``MoltenVK::MoltenVK``
  The MoltenVK library

Result Variables
^^^^^^^^^^^^^^^^

This will define the following variables:

``MoltenVK_FOUND``
  True if the system has the MoltenVK library.
``MoltenVK_VERSION``
  The version of the SDL library which was found.
``MoltenVK_LIBRARIES``
  Libraries needed to link to MoltenVK.

Cache Variables
^^^^^^^^^^^^^^^

The following cache variables may also be set:

``MoltenVK_LIBRARY``
  The path to the MoltenVK library.

#]=======================================================================]

# cmake-lint: disable=C0103

include(FindPackageHandleStandardArgs)

find_package(PkgConfig QUIET)
if(PKG_CONFIG_FOUND)
  pkg_search_module(PC_MoltenVK MoltenVK)
endif()

if(PC_MoltenVK_VERSION VERSION_GREATER 0)
  set(MoltenVK_VERSION ${PC_MoltenVK_VERSION})
else()
  if(NOT MoltenVK_FIND_QUIETLY)
    message(AUTHOR_WARNING "Failed to find MoltenVK version.")
  endif()
  set(MoltenVK_VERSION 0.0.0)
endif()

find_library(
  MoltenVK_LIBRARY
  NAMES MoltenVK
  HINTS ${PC_MoltenVK_LIBRARY_DIRS}
  DOC "MoltenVK location"
  NO_SYSTEM_ENVIRONMENT_PATH
  NO_CMAKE_SYSTEM_PATH
  NO_PACKAGE_ROOT_PATH
)

set(
  MoltenVK_ERROR_REASON
  "Ensure that ares-deps is provided as part of CMAKE_PREFIX_PATH or that MoltenVK is present in local library paths."
)

find_package_handle_standard_args(
  MoltenVK
  REQUIRED_VARS MoltenVK_LIBRARY
  VERSION_VAR MoltenVK_VERSION
  REASON_FAILURE_MESSAGE "${MoltenVK_ERROR_REASON}"
)
unset(MoltenVK_ERROR_REASON)
mark_as_advanced(MoltenVK_LIBRARY)

if(MoltenVK_FOUND)
  if(NOT TARGET MoltenVK::MoltenVK)
    if(IS_ABSOLUTE "${MoltenVK_LIBRARY}")
      add_library(MoltenVK::MoltenVK UNKNOWN IMPORTED)
      set_property(TARGET MoltenVK::MoltenVK PROPERTY IMPORTED_LOCATION "${MoltenVK_LIBRARY}")
    else()
      add_library(MoltenVK::MoltenVK SHARED IMPORTED)
      set_property(TARGET MoltenVK::MoltenVK PROPERTY IMPORTED_LIBNAME "${MoltenVK_LIBRARY}")
    endif()
    set_target_properties(
      MoltenVK::MoltenVK
      PROPERTIES INTERFACE_COMPILE_OPTIONS "${PC_MoltenVK_CFLAGS_OTHER}" VERSION ${MoltenVK_VERSION}
    )
  endif()
endif()

include(FeatureSummary)
set_package_properties(
  MoltenVK
  PROPERTIES
    URL "https://github.com/KhronosGroup/MoltenVK"
    DESCRIPTION
      "MoltenVK is a Vulkan Portability implementation. It layers a subset of the high-performance, industry-standard Vulkan graphics and compute API over Apple's Metal graphics framework, enabling Vulkan applications to run on macOS, iOS and tvOS."
)
