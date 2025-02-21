#[=======================================================================[.rst:
Findlibrashader
-------

Finds the librashader library.

Imported Targets
^^^^^^^^^^^^^^^^

This module provides the following imported targets, if found:

``librashader::librashader``
  The librashader library

Result Variables
^^^^^^^^^^^^^^^^

This will define the following variables:

``librashader_FOUND``
  True if the system has the libarashader library.
``librashader_VERSION``
  The version of the SDL library which was found.
``librashader_INCLUDE_DIR``
  Include directories needed to use librashader.
``librashader_LIBRARIES``
  Libraries needed to link to librashader.

Cache Variables
^^^^^^^^^^^^^^^

The following cache variables may also be set:

``librashader_INCLUDE_DIR``
  The directory containing ``librashader_ld.h``.
``librashader_LIBRARY``
  The path to the librashader library.

#]=======================================================================]

include(FindPackageHandleStandardArgs)

find_package(PkgConfig QUIET)
if(PKG_CONFIG_FOUND)
  pkg_search_module(PC_librashader librashader)
endif()

# librashader_set_soname: Set SONAME on imported library target
macro(librashader_set_soname)
  if(CMAKE_HOST_SYSTEM_NAME MATCHES "Linux|FreeBSD")
    execute_process(
      COMMAND sh -c "objdump -p '${librashader_LIBRARY}' | grep SONAME"
      OUTPUT_VARIABLE _output
      RESULT_VARIABLE _result
    )

    if(_result EQUAL 0)
      string(REGEX REPLACE "[ \t]+SONAME[ \t]+([^ \t]+)" "\\1" _soname "${_output}")
      set_property(TARGET librashader::librashader PROPERTY IMPORTED_SONAME "${_soname}")
      unset(_soname)
    endif()
  endif()
  unset(_output)
  unset(_result)
endmacro()

if(OS_LINUX OR OS_FREEBSD OR OS_OPENBSD)
  set(_librashader_path_hint "${CMAKE_SOURCE_DIR}/thirdparty/librashader/include")
endif()

find_path(
  librashader_INCLUDE_DIR
  NAMES librashader_ld.h librashader/librashader_ld.h
  HINTS ${PC_librashader_INCLUDE_DIRS} ${_librashader_path_hint}
  PATHS /usr/include /usr/local/include
  DOC "librashader include directory"
)

if(PC_librashader_VERSION VERSION_GREATER 0)
  set(librashader_VERSION ${PC_librashader_VERSION})
else()
  if(NOT librashader_FIND_QUIETLY)
    message(AUTHOR_WARNING "Failed to find librashader version.")
  endif()
  set(librashader_VERSION 0.0.0)
endif()

find_library(
  librashader_LIBRARY
  NAMES librashader rashader
  HINTS ${PC_librashader_LIBRARY_DIRS}
  PATHS /usr/lib /usr/local/lib
  DOC "librashader location"
)

if(CMAKE_HOST_SYSTEM_NAME MATCHES "Darwin|Windows")
  set(librashader_ERROR_REASON "Ensure that ares-deps is provided as part of CMAKE_PREFIX_PATH.")
elseif(CMAKE_HOST_SYSTEM_NAME MATCHES "Linux|FreeBSD")
  set(librashader_ERROR_REASON "Ensure librashader libraries are available in local library paths.")
endif()

find_package_handle_standard_args(
  librashader
  REQUIRED_VARS librashader_LIBRARY
  VERSION_VAR librashader_VERSION
  REASON_FAILURE_MESSAGE "${librashader_ERROR_REASON}"
)
mark_as_advanced(librashader_INCLUDE_DIR librashader_LIBRARY)
unset(librashader_ERROR_REASON)

if(librashader_FOUND AND ARES_ENABLE_LIBRASHADER)
  if(NOT TARGET librashader::librashader)
    add_library(librashader::librashader UNKNOWN IMPORTED)
    set_property(TARGET librashader::librashader PROPERTY IMPORTED_LOCATION "${librashader_LIBRARY}")
    # cargo does not set the minimum version correctly in the dylib, so manually define librashader's actual system requirement
    set_property(TARGET librashader::librashader PROPERTY MACOS_VERSION_REQUIRED 10.15)

    librashader_set_soname()
    set_target_properties(
      librashader::librashader
      PROPERTIES
        INTERFACE_COMPILE_OPTIONS "${PC_librashader_CFLAGS_OTHER}"
        INTERFACE_INCLUDE_DIRECTORIES "${librashader_INCLUDE_DIR}"
        VERSION ${librashader_VERSION}
    )
  endif()
endif()

# fallback to defining a target with the header, so everything still compiles
if(NOT TARGET librashader::librashader)
  if(librashader_INCLUDE_DIR)
    add_library(librashader INTERFACE)
    set_target_properties(librashader PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${librashader_INCLUDE_DIR}")
    add_library(librashader::librashader ALIAS librashader)
    if(ARES_ENABLE_LIBRASHADER)
      message(
        AUTHOR_WARNING
        "Tried to configure with librashader, but the library was NOT found. ${librashader_ERROR_REASON}"
      )
    endif()
  endif()
endif()

include(FeatureSummary)
set_package_properties(
  librashader
  PROPERTIES
    URL "https://github.com/SnowflakePowered/librashader"
    DESCRIPTION
      "librashader (/ˈli:brəʃeɪdɚ/) is a preprocessor, compiler, and runtime for RetroArch 'slang' shaders, rewritten in pure Rust."
)
