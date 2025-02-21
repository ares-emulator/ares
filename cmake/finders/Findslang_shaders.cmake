#[=======================================================================[.rst:
Findslang_shaders
-------

Finds the slang-shaders library.

Imported Targets
^^^^^^^^^^^^^^^^

This module provides the following imported targets, if found:

``libretro::slang_shaders``

Result Variables
^^^^^^^^^^^^^^^^

This will define the following variables:

``slang_shaders_FOUND``

Cache Variables
^^^^^^^^^^^^^^^

The following cache variables may also be set:

``slang_shaders_LOCATION``
  The directory containing the libretro shader collection.

#]=======================================================================]

find_path(
  slang_shaders_LOCATION
  NAMES bilinear.slangp nearest.slangp
  PATHS
    /usr/share/libretro/shaders/shaders_slang
    /usr/local/share/libretro/shaders/shaders_slang
    ${CMAKE_PREFIX_PATH}/share/libretro/shaders/shaders_slang
  DOC "slang-shaders collection location"
)

if(CMAKE_HOST_SYSTEM_NAME MATCHES "Darwin|Windows")
  set(SLANG_ERROR_REASON "Ensure that ares-deps are provided as part of CMAKE_PREFIX_PATH.")
elseif(CMAKE_HOST_SYSTEM_NAME MATCHES "Linux|FreeBSD")
  set(
    SLANG_ERROR_REASON
    "Ensure slang-shaders are installed in local data paths or that ares-deps are provided as part of CMAKE_PREFIX_PATH."
  )
endif()

find_package_handle_standard_args(
  slang_shaders
  REQUIRED_VARS slang_shaders_LOCATION
  REASON_FAILURE_MESSAGE "${SLANG_ERROR_REASON}"
)
mark_as_advanced(slang_shaders_LOCATION)

if(NOT TARGET libretro::slang_shaders)
  if(slang_shaders_LOCATION)
    add_library(slang_shaders INTERFACE)
    set_target_properties(
      slang_shaders
      PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${slang_shaders_LOCATION}" MACOS_VERSION_REQUIRED 10.15
    )
    add_library(libretro::slang_shaders ALIAS slang_shaders)
  endif()
endif()
