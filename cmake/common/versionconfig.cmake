include_guard(GLOBAL)

function(populate_ares_version_info)
  if(DEFINED ARES_VERSION_OVERRIDE)
    set(ARES_VERSION ${ARES_VERSION_OVERRIDE})
  elseif(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/.git")
    execute_process(
      COMMAND git describe --always --tags --exclude=nightly --dirty=-modified
      OUTPUT_VARIABLE ARES_VERSION
      ERROR_VARIABLE git_describe_err
      WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
      RESULT_VARIABLE ares_version_result
      OUTPUT_STRIP_TRAILING_WHITESPACE
    )

    if(git_describe_err)
      message(FATAL_ERROR "Could not fetch ares version tag from git.\n" ${git_describe_err})
    endif()
  else()
    file(READ cmake/common/.archive-version ares_tarball_version)
    string(STRIP "${ares_tarball_version}" ares_tarball_version)
    set(ARES_VERSION "${ares_tarball_version}")
  endif()
  
  string(REGEX REPLACE "^v([0-9]+(\\.[0-9]+)*)-.*" "\\1" ares_version_stripped "${ARES_VERSION}")
  string(REPLACE "." ";" ares_version_parts "${ares_version_stripped}")
  list(LENGTH ares_version_parts ares_version_parts_length)
  list(GET ares_version_parts 0 major)
  if(ares_version_parts_length GREATER 1)
    list(GET ares_version_parts 1 minor)
  else()
    set(minor 0)
  endif()
  if(ares_version_parts_length GREATER 2)
    list(GET ares_version_parts 2 patch)
  else()
    set(patch 0)
  endif()
  set(ARES_VERSION_CANONICAL "${major}.${minor}.${patch}")
  message(DEBUG "Canonical version set to ${ARES_VERSION_CANONICAL}")
  return(PROPAGATE ARES_VERSION ARES_VERSION_CANONICAL)
endfunction()

populate_ares_version_info()

if(NOT ARES_VERSION_CANONICAL MATCHES "^[0-9]+\\.[0-9]+\\.[0-9]+$")
  message(
    FATAL_ERROR
    "Available ares version information ('${ARES_VERSION}') could not be converted to a valid CMake version string.\n"
    "Make sure the repository you have cloned or archived from contains ares version tags ('git fetch --tags').\n"
    "If tags are unavailable, you may specify a custom version with ARES_VERSION_OVERRIDE, using an ares-formatted "
    "version string, e.g. 'v123'.\n"
  )
endif()

