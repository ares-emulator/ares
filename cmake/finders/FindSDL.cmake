#[=======================================================================[.rst:
FindSDL
-------

Finds the SDL library.

Imported Targets
^^^^^^^^^^^^^^^^

This module provides the following imported targets, if found:

``SDL::SDL``
  The SDL library

Result Variables
^^^^^^^^^^^^^^^^

This will define the following variables:

``SDL_FOUND``
  True if the system has the SDL library.
``SDL_VERSION``
  The version of the SDL library which was found.
``SDL_INCLUDE_DIRS``
  Include directories needed to use SDL.
``SDL_LIBRARIES``
  Libraries needed to link to SDL.

Cache Variables
^^^^^^^^^^^^^^^

The following cache variables may also be set:

``SDL_INCLUDE_DIR``
  The directory containing ``SDL.h``.
``SDL_LIBRARY``
  The path to the SDL library.

#]=======================================================================]

include(FindPackageHandleStandardArgs)

find_package(PkgConfig QUIET)
if(PKG_CONFIG_FOUND)
  pkg_search_module(PC_SDL QUIET sdl3)
endif()

# SDL_set_soname: Set SONAME on imported library target
macro(SDL_set_soname)
  if(CMAKE_HOST_SYSTEM_NAME MATCHES "Linux|FreeBSD")
    execute_process(
      COMMAND sh -c "objdump -p '${SDL_LIBRARY}' | grep SONAME"
      OUTPUT_VARIABLE _output
      RESULT_VARIABLE _result
    )

    if(_result EQUAL 0)
      string(REGEX REPLACE "[ \t]+SONAME[ \t]+([^ \t]+)" "\\1" _soname "${_output}")
      set_property(TARGET SDL::SDL PROPERTY IMPORTED_SONAME "${_soname}")
      unset(_soname)
    endif()
  endif()
  unset(_output)
  unset(_result)
endmacro()

find_library(
  SDL_LIBRARY
  NAMES SDL3 SDL3-3.0.0 SDL3-3.0
  HINTS ${PC_SDL_LIBRARY_DIRS}
  PATHS ${CMAKE_SOURCE_DIR}/.deps /usr/lib /usr/local/lib
  DOC "SDL location"
)

find_path(
  SDL_INCLUDE_DIR
  NAMES SDL.h SDL3/SDL.h
  HINTS ${PC_SDL_INCLUDE_DIRS} ${SDL_LIBRARY}/..
  PATHS ${CMAKE_SOURCE_DIR}/.deps /usr/include /usr/local/include
  DOC "SDL include directory"
  # "$<$<PLATFORM_ID:Darwin>:NO_DEFAULT_PATH>"
)

if(PC_SDL_VERSION VERSION_GREATER 0)
  set(SDL_VERSION ${PC_SDL_VERSION})
else()
  if(NOT SDL_FIND_QUIETLY)
    message(AUTHOR_WARNING "Failed to find SDL version.")
  endif()
  set(SDL_VERSION 0.0.0)
endif()

if(CMAKE_HOST_SYSTEM_NAME MATCHES "Darwin|Windows")
  set(SDL_ERROR_REASON "Ensure that ares-deps are provided as part of CMAKE_PREFIX_PATH.")
elseif(CMAKE_HOST_SYSTEM_NAME MATCHES "Linux|FreeBSD")
  set(SDL_ERROR_REASON "Ensure SDL libraries are available in local library paths.")
endif()

find_package_handle_standard_args(
  SDL
  REQUIRED_VARS SDL_LIBRARY SDL_INCLUDE_DIR
  VERSION_VAR SDL_VERSION
  REASON_FAILURE_MESSAGE "${SDL_ERROR_REASON}"
)
mark_as_advanced(SDL_INCLUDE_DIR SDL_LIBRARY)
unset(SDL_ERROR_REASON)

if(SDL_FOUND)
  if(NOT TARGET SDL::SDL)
    if(IS_ABSOLUTE "${SDL_LIBRARY}")
      add_library(SDL::SDL UNKNOWN IMPORTED)
      set_property(TARGET SDL::SDL PROPERTY IMPORTED_LOCATION "${SDL_LIBRARY}")
    else()
      add_library(SDL::SDL SHARED IMPORTED)
      set_property(TARGET SDL::SDL PROPERTY IMPORTED_LIBNAME "${SDL_LIBRARY}")
    endif()

    sdl_set_soname()
    set_target_properties(
      SDL::SDL
      PROPERTIES
        INTERFACE_COMPILE_OPTIONS "${PC_SDL_CFLAGS_OTHER}"
        INTERFACE_INCLUDE_DIRECTORIES "${SDL_INCLUDE_DIR}"
        VERSION ${SDL_VERSION}
    )
  endif()
endif()

include(FeatureSummary)
set_package_properties(SDL PROPERTIES URL "https://github.com/libsdl-org/SDL" DESCRIPTION "Simple Directmedia Layer")
